#if defined(SENSOR_MCP9808)
    #include "Adafruit_MCP9808.h"
    Adafruit_MCP9808 mcp;
#endif

#if defined(SENSOR_BMP280)
#include <BME280I2C.h>
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

BME280I2C bmp(settings);
#endif

#if defined(SENSOR_HTU21D)
    #include <HTU21D.h>
    HTU21D htu(HTU21D_RES_RH12_TEMP14);
#endif

float round2(float value) {
    return round(value * 100.0) / 100.0;
}

void sensorsSetup() {
    Wire.begin(SDA, SCL);
    Wire.setClock(I2C_SPEED);

#if defined(SENSOR_MCP9808)

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

#else
    sensor_state &= ~(1 << 1);
#endif

#if defined(SENSOR_BMP280)

    if (bmp.begin()) {
        sensor_state |= (1 << 0);

    } else {
        sensor_state &= ~(1 << 0);

#if defined(DEBUG)
        Serial.println("[SENSOR] BMP280 did not respond. Please check wiring.");
#endif
    }

#else
    sensor_state &= ~(1 << 0);
#endif

#if defined(SENSOR_HTU21D)

    if (htu.begin(SDA, SCL)) {
        sensor_state |= (1 << 2);

    } else {
        sensor_state &= ~(1 << 2);

#if defined(DEBUG)
        Serial.println("[SENSOR] HTU21D did not respond. Please check wiring.");
#endif
    }

#else
    sensor_state &= ~(1 << 2);
#endif
}

void sensorsLoop() {
    static uint32_t last_check = 0;
    static uint8_t counter = 0;


    if ((last_check > 0) && ((millis() - last_check) < READ_INTERVAL)) {
        return;
    }

    last_check = millis();

#if defined(SENSOR_BMP280)
    float temp_bmp = bmp.temp();
    float abs_pressure = NAN;
    float rel_pressure = NAN;

    if (!isnan(temp_bmp)) {
        if ((sensor_state & 0b1) == 0) {
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
                Serial.println("[SENSOR] BMP280 could not init.");
#endif
            }
        }

        if (sensor_state & 0b1) {
            abs_pressure = bmp.pres(BME280::PresUnit_Pa);

            if (height_above_sea <= 500) {  // relative pressure in only counted up to 500 m above sea
                rel_pressure = filter[3].add(meteoFunctions.relativePressure_c(abs_pressure, height_above_sea, temp_bmp));

            } else {
                rel_pressure = NAN;
            }

            abs_pressure = filter[4].add(abs_pressure);
            temp_bmp = filter[0].add(temp_bmp);
        }

    } else {
        if (sensor_state & 0b1) {
            sensor_state &= ~(1 << 0);  // BME not available

#if defined(DEBUG)
            Serial.println("[SENSOR] BMP280 stopped responding again. Please check wiring.");
#endif
            temp_bmp = NAN;
            abs_pressure = NAN;
            rel_pressure = NAN;
        }
    }

#endif

#if defined(SENSOR_MCP9808)
    float temp_mcp = mcp.readTempC();

    if (!isnan(temp_mcp)) {
        if ((sensor_state & 0b10) == 0) {
            if (mcp.begin()) {
                sensor_state |= (1 << 1);

                filter[1].reset();

                temp_mcp = mcp.readTempC();

#if defined(DEBUG)
                Serial.println("[SENSOR] MCP9808 is back on.");
#endif

            } else {
                sensor_state &= ~(1 << 1);
#if defined(DEBUG)
                Serial.println("[SENSOR] MCP9808 could not init again.");
#endif
            }
        }

        if (sensor_state & 0b10) {
            temp_mcp = filter[1].add(temp_mcp);
        }

    } else {
        if (sensor_state & 0b10) {
            sensor_state &= ~(1 << 1);  // MCP not available

#if defined(DEBUG)
            Serial.println("[SENSOR] MCP9808 stopped responding. Please check wiring.");
#endif
            temp_mcp = NAN;
        }
    }

#endif

#if defined(SENSOR_HTU21D)
    float temp_htu = htu.readTemperature();
    float humidity = NAN;

    if (temp_htu < 255.0) {
        if ((sensor_state & 0b100) == 0) {
            if (htu.begin(SDA, SCL)) {
                sensor_state |= (1 << 2);

                filter[2].reset();
                filter[5].reset();

                temp_htu = htu.readTemperature();

#if defined(DEBUG)
                Serial.println("[SENSOR] HTU21D is back on.");
#endif

            } else {
                sensor_state &= ~(1 << 2);
#if defined(DEBUG)
                Serial.println("[SENSOR] HTU21D could not init again.");
#endif
            }
        }

        if (sensor_state & 0b100) {
            temp_htu = filter[2].add(temp_htu);
            humidity = htu.readHumidity();

            if (humidity > 100.0 || humidity < 0.0) {
                humidity = NAN;

            } else {
                humidity = filter[5].add(humidity);
            }
        }

    } else {
        if (sensor_state & 0b100) {
            sensor_state &= ~(1 << 2);  // HTU not available

#if defined(DEBUG)
            Serial.println("[SENSOR] HTU21D stopped responding. Please check wiring.");
#endif
        }

        humidity = NAN;
        temp_htu = NAN;
    }

#endif

#if defined(DEBUG)
#if defined(SENSOR_BMP280)
    Serial.print("[SENSOR] Pressure: ");
    Serial.print(abs_pressure);
    Serial.print("=");
    Serial.print(rel_pressure);
    Serial.println(" Pa");
#endif

#if defined(SENSOR_MCP9808)
    Serial.print("[SENSOR] Temp MCP: ");
    Serial.print(temp_mcp);
#endif

#if defined(SENSOR_BMP280)
    Serial.print(", BMP:");
    Serial.print(temp_bmp);
#endif

#if defined(SENSOR_HTU21D)
    Serial.print(", HTU:");
    Serial.print(temp_htu);
    Serial.println(" C");
    Serial.print("[SENSOR] Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");
    Serial.println();
#endif
#endif

    counter++;

    if (counter > READ_SAMPLES) {  // time to send data
        counter = 0;

        if (mqtt.connected()) {
            StaticJsonDocument < (JSON_OBJECT_SIZE(8) + JSON_ARRAY_SIZE(3)) > json;

#if defined(SENSOR_HTU21D)

            if (!isnan(humidity)) {
                json["humidity"] = round2(humidity);
            }

#endif

#if defined(SENSOR_BMP280)

            if (!isnan(rel_pressure)) {
                json["pressure_rel"] = round2(rel_pressure / 100.0);
            }

            if (!isnan(abs_pressure)) {
                json["pressure_abs"] = round2(abs_pressure / 100.0);
            }

#endif
#if defined(SENSOR_HTU21D)

            if (!isnan(temp_htu) && !isnan(humidity)) {
                json["dew"] = round2(meteoFunctions.dewPoint_c(temp_mcp, humidity));
                json["humidex"] = round2(meteoFunctions.humidex_c(temp_mcp, humidity));
                json["heat"] = round2(meteoFunctions.heatIndex_c(temp_mcp, humidity));
                json["cloud"] = round2(meteoFunctions.cloudBase_m(temp_mcp, humidity));
            }

#endif
#if defined(SENSOR_MCP9808) || defined(SENSOR_BMP280) || defined(SENSOR_HTU21D)

            if (sensor_state > 0) {
                JsonArray temperatures = json.createNestedArray("temp");
#if defined(SENSOR_MCP9808)
                temperatures.add(round2(temp_mcp));
#endif
#if defined(SENSOR_BMP280)
                temperatures.add(round2(temp_bmp));
#endif

#if defined(SENSOR_HTU21D)
                temperatures.add(round2(temp_htu));
#endif
            }

#endif

            if (json.size() > 0) {
#if defined(DEBUG)
                Serial.println("[MQTT] Sending sensor data:");
                serializeJsonPretty(json, Serial);
                Serial.println();
#endif

                char message[350];
                uint32_t len = serializeJson(json, message, sizeof(message));
                mqtt.publish(MQTT_SENSORS_TOPIC, MQTT_QOS, MQTT_RETAIN, message, len);

            } else {
#if defined(DEBUG)
                Serial.println("[MQTT] No valid data to send");
#endif
            }

        } else {
#if defined(DEBUG)
            // Serial.println("[MQTT] Could not send sensors data");
#endif
        }
    }
}
