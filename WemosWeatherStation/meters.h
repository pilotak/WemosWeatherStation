#include <Ticker.h>
#include "WeatherMeters.h"
#include "MCP3X21.h"

Ticker ticker;
WeatherMeters <0> meters(-1);
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
    read_adc = true;
    meters.timer();
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

#if defined(ENABLE_DEBUG)
    meters.init(&Serial);
#endif
}

void metersLoop() {
    if (read_adc) {
        meters.adcToDir(mcp3021.read());
        read_adc = false;
    }

    if (rain_data || meters_data) {
        StaticJsonBuffer < JSON_OBJECT_SIZE(2) > jsonBuffer;

        JsonObject& json = jsonBuffer.createObject();

        if (rain_data) {
            rain_data = false;

            json["rain"] = RAIN_GAUGE_RES;
        }

        if (meters_data) {
            meters_data = false;

            json["wind_dir"] = round2(meters.getDir());
            json["wind_speed"] = round2(meters.getSpeed());
        }

#if defined(DEBUG)
        Serial.println("[MQTT] Sending meters data:");
        json.prettyPrintTo(Serial);
        Serial.println();
#endif
        char message[150];
        uint32_t len = json.printTo(message, sizeof(message));
        mqttClient.publish(MQTT_WIND_TOPIC, MQTT_QOS, MQTT_RETAIN, message, len);
    }
}
