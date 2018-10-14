#include <Ticker.h>
#include "WeatherMeters.h"
#include "MCP3X21.h"

Ticker ticker;
WeatherMeters <METERS_DIR_FILTERING> meters(-1, METERS_INTERVAL);
MCP3021 mcp3021(MCP3021_ADDRESS);

volatile bool meters_data = false;
volatile bool rain_data = false;
volatile bool read_adc = false;

void intAnemometer() {
    meters.intAnemometer();
}

void intRaingauge() {
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
        meters.adcToDir(mcp3021.read());
        read_adc = false;
    }

    if (rain_data || meters_data) {
        if (mqtt.connected()) {
            StaticJsonBuffer < JSON_OBJECT_SIZE(5) > jsonBuffer;
            JsonObject& json = jsonBuffer.createObject();
            char message[180];

            if (rain_data) {
                rain_data = false;
                json["rain"] = RAIN_GAUGE_RES;

#if defined(DEBUG)
                Serial.println("[MQTT] Sending rain:");
                json.prettyPrintTo(Serial);
                Serial.println();
#endif

                uint32_t len = json.printTo(message, sizeof(message));
                mqtt.publish(MQTT_RAIN_TOPIC, MQTT_QOS, false, message, len);
            }

            if (meters_data) {
                meters_data = false;
                float wind_speed = round2(meters.getSpeed());

                json["wind_dir"] = meters.getDir();
                json["wind_speed"] = wind_speed;
                json["wind_chill"] = round2(meteoFunctions.windChill_c(filter[0].get(), wind_speed));
                json["apparent"] = round2(meteoFunctions.apparentTemp_c(filter[0].get(), filter[1].get(), wind_speed));
                json["beaufort"] = meteoFunctions.beaufort(wind_speed);

#if defined(DEBUG)
                Serial.println("[MQTT] Sending meters data:");
                json.prettyPrintTo(Serial);
                Serial.println();
#endif

                uint32_t len = json.printTo(message, sizeof(message));
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
