#include <Ticker.h>
#include "WeatherMeters.h"
#include "MCP3X21.h"

Ticker ticker;
WeatherMeters <METERS_DIR_FILTERING> meters(-1, METERS_INTERVAL);
MCP3021 mcp3021(MCP3021_ADDRESS);

volatile bool meters_data = false;
volatile bool rain_data = false;
volatile bool read_adc = false;

void IRAM_ATTR intAnemometer() {
    meters.intAnemometer();
}

void IRAM_ATTR intRaingauge() {
    meters.intRaingauge();
}

void secondTimer() {
    if (!ota_in_progess) {
        meters.timer();
        read_adc = true;
    }
}

void metersData(void) {
    meters_data = true;
}

void rainData(void) {
    rain_data = true;
}

void metersSetup() {
    attachInterrupt(digitalPinToInterrupt(ANEMOMETER_PIN), intAnemometer, FALLING);
    attachInterrupt(digitalPinToInterrupt(RAIN_GAUGE_PIN), intRaingauge, FALLING);

    mcp3021.init(&Wire);

    meters.attach(metersData);
    meters.attachRain(rainData);
    ticker.attach(1.0, secondTimer);
    meters.reset();  // in case we got already some interrupts

#if defined(DEBUG)
    // meters.debug(&Serial);
#endif
}

void metersLoop() {
    if (read_adc) {
        uint16_t adc;
        adc = mcp3021.read();

        if (adc != 0xFFFF) {
            meters.adcToDir(adc);

        } else {
            meters.reset();
        }

        read_adc = false;
    }

    if (rain_data || meters_data) {
        if (mqtt.connected()) {
            StaticJsonDocument <JSON_OBJECT_SIZE(5)> json;
            char message[180];

            if (rain_data) {
                rain_data = false;
                json["rain"] = RAIN_GAUGE_RES;

#if defined(DEBUG)
                Serial.println("[MQTT] Sending rain:");
                serializeJsonPretty(json, Serial);
                Serial.println();
#endif

                uint32_t len = serializeJson(json, message, sizeof(message));
                mqtt.publish(MQTT_RAIN_TOPIC, MQTT_QOS, false, message, len);
            }

            if (meters_data) {
                meters_data = false;
                float wind_speed_kmh = meters.getSpeed();
                float wind_speed_ms = meteoFunctions.kmhToMs(wind_speed_kmh);

                json["wind_dir"] = meters.getDir();
                json["wind_speed"] = round2(wind_speed_kmh);
                json["beaufort"] = meteoFunctions.beaufort(wind_speed_ms);

                float temp = NAN;
                float humidity = NAN;

#if defined(HAS_HUMIDITY)

                if (sensor_state & 0b100) {
                    temp = humidity_temp_filter.get();
                    humidity = humidity_filter[0].get();
                }

#elif defined(HAS_TEMP)

                if (sensor_state & 0b010) {
                    temp = temp_filter.get();
                }

#elif defined(HAS_BARO)

                if (sensor_state & 0b001) {
                    temp = baro_temp_filter.get();
                }

#endif

                if (!isnan(temp)) {
                    json["wind_chill"] = round2(meteoFunctions.windChill_c(temp, wind_speed_ms));

                    if (!isnan(humidity)) {
                        json["apparent"] = round2(meteoFunctions.apparentTemp_c(temp, humidity, wind_speed_ms));
                    }
                }

#if defined(DEBUG)
                Serial.println("[MQTT] Sending meters data:");
                serializeJsonPretty(json, Serial);
                Serial.println();
#endif

                uint32_t len = serializeJson(json, message, sizeof(message));
                mqtt.publish(MQTT_WIND_TOPIC, MQTT_QOS, MQTT_RETAIN, message, len);
            }

        } else {
            rain_data = false;
            meters_data = false;

#if defined(DEBUG)
            Serial.println("[MQTT] Could not send meters data");
#endif
        }
    }
}
