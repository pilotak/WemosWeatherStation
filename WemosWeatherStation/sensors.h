#include <Wire.h>
#include <BME280I2C.h>
#include "Adafruit_MCP9808.h"
#include <HTU21D.h>
#include "MeteoFunctions.h"
#include "MovingAverageFloat.h"

BME280I2C::Settings settings(
    BME280::OSR_X2,
    BME280::OSR_X1,
    BME280::OSR_X4,
    BME280::Mode_Forced,
    BME280::StandbyTime_1000ms,
    BME280::Filter_8,
    BME280::SpiEnable_False,
    BMP280_ADDR
);

Adafruit_MCP9808 mcp;
BME280I2C bme(settings);
HTU21D htu(HTU21D_RES_RH12_TEMP14);
MeteoFunctions meteoFunctions;
MovingAverageFloat <SAMPLES> filter[3];

float round2(float value) {
    return round(value * 100.0) / 100.0;
}

void sensorsSetup() {
    Wire.begin(SDA, SCL);

    if (!bme.begin()) {
#if defined(DEBUG)
        Serial.println("[SENSOR] BMP280 did not respond. Please check wiring.");
#endif

        while (1) {}
    }

    if (!mcp.begin(MCP9808_ADDR, &Wire)) {
#if defined(DEBUG)
        Serial.println("[SENSOR] MCP9808 did not respond. Please check wiring.");
#endif

        while (1) {}
    }

    mcp.setResolution(2);
    mcp.shutdown_wake(0);

    if (!htu.begin(SDA, SCL)) {
#if defined(DEBUG)
        Serial.println("[SENSOR] HTU21D did not respond. Please check wiring.");
#endif

        while (1) {}
    }
}

void sensorsLoop() {
    static uint32_t last_check = 0;
    static uint8_t counter = 0;

    if ((last_check > 0) && ((millis() - last_check) < READ_INTERVAL)) {
        return;
    }

    last_check = millis();

    float temp_mcp = filter[0].add(mcp.readTempC());
    float temp_bmp = bme.temp();
    float temp_htu = htu.readTemperature();
    float abs_pressure = bme.pres(BME280::PresUnit_Pa);
    float rel_pressure = filter[2].add(meteoFunctions.relativePressure_c(abs_pressure, HEIGHT_ABOVE_SEA, temp_bmp));
    float humidity = filter[1].add(htu.readCompensatedHumidity());

#if defined(DEBUG)
    Serial.print("[SENSOR] Pressure: ");
    Serial.print(abs_pressure);
    Serial.print("=");
    Serial.print(rel_pressure);
    Serial.println(" Pa");

    Serial.print("[SENSOR] Temp MCP: ");
    Serial.print(temp_mcp);
    Serial.print(", BMP:");
    Serial.print(temp_bmp);
    Serial.print(", HTU:");
    Serial.print(temp_htu);
    Serial.println(" C");

    Serial.print("[SENSOR] Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");
    Serial.println();
#endif

    counter++;

    if (counter == SAMPLES) {  // time to send data
        StaticJsonBuffer < (JSON_OBJECT_SIZE(7)) > jsonBuffer;

        JsonObject& json = jsonBuffer.createObject();
        json["temp"] = round2(temp_mcp);
        json["humidity"] = round2(humidity);
        json["pressure"] = round2(rel_pressure / 100.0);
        json["dew"] = round2(meteoFunctions.dewPoint_c(temp_mcp, humidity));
        json["humidex"] = round2(meteoFunctions.humidex_c(temp_mcp, humidity));
        json["heat"] = round2(meteoFunctions.heatIndex_c(temp_mcp, humidity));
        json["cloud"] = round2(meteoFunctions.cloudBase_m(temp_mcp, humidity));

#if defined(DEBUG)
        Serial.println("[MQTT] Sending sensor data:");
        json.prettyPrintTo(Serial);
        Serial.println();
#endif

        char message[300];
        uint32_t len = json.printTo(message, sizeof(message));
        mqttClient.publish(MQTT_SENSORS_TOPIC, MQTT_QOS, MQTT_RETAIN, message, len);
        counter = 0;
    }
}
