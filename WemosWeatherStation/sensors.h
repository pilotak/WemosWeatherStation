#include <Wire.h>
#include <BME280I2C.h>
#include "Adafruit_MCP9808.h"
#include <HTU21D.h>
#include "MeteoFunctions.h"
#include "MovingAverageFloat.h"

BME280I2C::Settings settings(
    BME280::OSR_X1,
    BME280::OSR_X1,
    BME280::OSR_X1,
    BME280::Mode_Forced,
    BME280::StandbyTime_1000ms,
    BME280::Filter_8,
    BME280::SpiEnable_False,
    BMP280_ADDR
);

Adafruit_MCP9808 mcp;
BME280I2C bmp(settings);
HTU21D htu(HTU21D_RES_RH12_TEMP14);
MeteoFunctions meteoFunctions;
MovingAverageFloat <SAMPLES> filter[6];  // BMP temp, MCP temp, HTU temp, rel pressure, abs pressure, humidity

float round2(float value) {
    return round(value * 100.0) / 100.0;
}

void sensorsSetup() {
    Wire.begin(SDA, SCL);

    if (bmp.begin()) {
        sensor_state |= (1 << 0);

    } else {
        sensor_state &= ~(1 << 0);

#if defined(DEBUG)
        Serial.println("[SENSOR] BMP280 did not respond. Please check wiring.");
#endif
    }

    if (mcp.begin(MCP9808_ADDR, &Wire)) {
        sensor_state |= (1 << 1);
        mcp.setResolution(2);
        mcp.shutdown_wake(0);

    } else {
        sensor_state &= ~(1 << 1);

#if defined(DEBUG)
        Serial.println("[SENSOR] MCP9808 did not respond. Please check wiring.");
#endif
    }

    if (htu.begin(SDA, SCL)) {
        sensor_state |= (1 << 2);

    } else {
        sensor_state &= ~(1 << 2);

#if defined(DEBUG)
        Serial.println("[SENSOR] HTU21D did not respond. Please check wiring.");
#endif
    }
}

void sensorsLoop() {
    static uint32_t last_check = 0;
    static uint8_t counter = 0;


    if ((last_check > 0) && ((millis() - last_check) < READ_INTERVAL)) {
        return;
    }

    last_check = millis();

    float temp_bmp = NAN;
    float abs_pressure = NAN;
    float rel_pressure = NAN;
    float temp_mcp = NAN;
    float temp_htu = NAN;
    float humidity = NAN;

    temp_bmp = bmp.temp();

    if (!isnan(temp_bmp)) {
        if ((sensor_state & 0b001) == 0) {
            if (bmp.begin()) {
                sensor_state |= (1 << 0);

                filter[0].reset();
                filter[3].reset();
                filter[4].reset();

                temp_bmp = bmp.temp();

#if defined(DEBUG)
                Serial.println("[SENSOR] BMP280 is back on.");
#endif

            } else {
                sensor_state &= ~(1 << 0);

#if defined(DEBUG)
                Serial.println("[SENSOR] BMP280 did not respond. Please check wiring.");
#endif
            }
        }

        abs_pressure = bmp.pres(BME280::PresUnit_Pa);

        rel_pressure = filter[3].add(meteoFunctions.relativePressure_c(abs_pressure, HEIGHT_ABOVE_SEA, temp_bmp));
        abs_pressure = filter[4].add(abs_pressure);
        temp_bmp = filter[0].add(temp_bmp);

    } else {
        if (sensor_state & 0b001) {
            sensor_state &= ~(1 << 0);  // BME not available

#if defined(DEBUG)
            Serial.println("[SENSOR] BMP280 did not respond. Please check wiring.");
#endif
        }
    }


    temp_mcp = mcp.readTempC();

    if (!isnan(temp_mcp)) {
        if (((sensor_state & 0b010) >> 1) == 0) {
            sensor_state |= (1 << 1);

            filter[1].reset();

#if defined(DEBUG)
            Serial.println("[SENSOR] MCP9808 is back on.");
#endif
        }

        temp_mcp = filter[1].add(temp_mcp);

    } else {
        if (sensor_state & 0b100) {
            sensor_state &= ~(1 << 1);  // MCP not available

#if defined(DEBUG)
            Serial.println("[SENSOR] MCP9808 did not respond. Please check wiring.");
#endif
        }
    }

    temp_htu = htu.readTemperature();

    if (temp_htu < 255.0) {
        if (((sensor_state & 0b100) >> 2) == 0) {
            sensor_state |= (1 << 2);

            filter[2].reset();
            filter[5].reset();

#if defined(DEBUG)
            Serial.println("[SENSOR] HTU21D is back on.");
#endif
        }

        temp_htu = filter[2].add(temp_htu);
        humidity = filter[5].add(htu.readCompensatedHumidity());

    } else {
        if (sensor_state & 0b100) {
            sensor_state &= ~(1 << 2);  // HTU not available

#if defined(DEBUG)
            Serial.println("[SENSOR] HTU21D did not respond. Please check wiring.");
#endif
        }

        humidity = NAN;
        temp_htu = NAN;
    }

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
        if (mqtt.connected()) {
            StaticJsonBuffer < (JSON_OBJECT_SIZE(8) + JSON_ARRAY_SIZE(3)) > jsonBuffer;

            JsonObject& json = jsonBuffer.createObject();

            if (!isnan(humidity)) {
                json["humidity"] = round2(humidity);
            }

            if (!isnan(rel_pressure)) {
                json["pressure_rel"] = round2(rel_pressure / 100.0);
            }

            if (!isnan(abs_pressure)) {
                json["pressure_abs"] = round2(abs_pressure / 100.0);
            }

            if (!isnan(temp_mcp) && !isnan(humidity)) {
                json["dew"] = round2(meteoFunctions.dewPoint_c(temp_mcp, humidity));
                json["humidex"] = round2(meteoFunctions.humidex_c(temp_mcp, humidity));
                json["heat"] = round2(meteoFunctions.heatIndex_c(temp_mcp, humidity));
                json["cloud"] = round2(meteoFunctions.cloudBase_m(temp_mcp, humidity));
            }

            if (!isnan(temp_mcp) && !isnan(temp_bmp) && !isnan(temp_htu)) {
                JsonArray& temperatures = json.createNestedArray("temp");

                if (!isnan(temp_mcp)) {
                    temperatures.add(round2(temp_mcp));
                }

                if (!isnan(temp_bmp)) {
                    temperatures.add(round2(temp_bmp));
                }

                if (!isnan(temp_htu)) {
                    temperatures.add(round2(temp_htu));
                }
            }

            if (json.size() > 0) {
#if defined(DEBUG)
                Serial.println("[MQTT] Sending sensor data:");
                json.prettyPrintTo(Serial);
                Serial.println();
#endif

                char message[350];
                uint32_t len = json.printTo(message, sizeof(message));
                mqtt.publish(MQTT_SENSORS_TOPIC, MQTT_QOS, MQTT_RETAIN, message, len);
                counter = 0;

            } else {
#if defined(DEBUG)
                Serial.println("[MQTT] No valid data to send");
#endif
            }

        } else {
#if defined(DEBUG)
            Serial.println("[MQTT] Could not send sensors data");
#endif
        }
    }
}
