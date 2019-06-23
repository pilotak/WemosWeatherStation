#if defined(SENSOR_MCP9808)
    #include "Adafruit_MCP9808.h"
    Adafruit_MCP9808 mcp;
#endif

#if defined(SENSOR_BMP280) || defined(SENSOR_BME280)
#include "BME280I2C.h"
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
#elif defined(SENSOR_LPS35HW)
#include "LPS35HW.h"
LPS35HW lps;
#endif

#if defined(SENSOR_HTU21D)
    #include "HTU21D.h"
    HTU21D htu(HTU21D_RES_RH12_TEMP14);
#elif defined(SENSOR_SHT31)
    #include "ClosedCube_SHT31D.h"
    ClosedCube_SHT31D sht31;
#endif

float round2(float value) {
    return round(value * 100.0) / 100.0;
}

bool setupBaro() {
#if defined(SENSOR_BMP280) || defined(SENSOR_BME280)
    return bmp.begin();
#elif defined(SENSOR_LPS35HW)

    if (lps.begin(&Wire)) {
        lps.setLowPassFilter(LPS35HW::LowPassFilter_ODR9);
        return true;
    }

#endif
    return false;
}

bool setupMCP9808() {
#if defined(SENSOR_MCP9808)

    if (mcp.begin(MCP9808_ADDR, &Wire)) {
        sensor_state |= (1 << 1);
        mcp.setResolution(3);
        mcp.shutdown_wake(0);
        return true;
    }

#endif
    return false;
}

bool setupHumidity() {
#if defined(SENSOR_HTU21D)

    if (htu.begin(SDA, SCL)) {
        return true;
    }

#elif defined(SENSOR_SHT31)

    if (sht31.begin(SHT31D_ADDR) == SHT3XD_NO_ERROR) {
        if (sht31.periodicStart(SHT3XD_REPEATABILITY_HIGH, SHT3XD_FREQUENCY_10HZ) == SHT3XD_NO_ERROR) {
            return true;
        }
    }

#endif
    return false;
}

void sensorsSetup() {
    Wire.begin(SDA, SCL);
    Wire.setClock(I2C_SPEED);

#if defined(SENSOR_BMP280) || defined(SENSOR_LPS35HW)

    if (setupBaro()) {
        sensor_state |= (1 << 0);

    } else {
        sensor_state &= ~(1 << 0);

#if defined(DEBUG)
        Serial.println("[SENSOR] Barometer did not respond. Please check wiring.");
#endif
    }

#else
    sensor_state &= ~(1 << 0);
#endif


#if defined(SENSOR_MCP9808)

    if (mcp.begin(MCP9808_ADDR, &Wire)) {
        sensor_state |= (1 << 1);
        mcp.setResolution(3);
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

#if defined(SENSOR_HTU21D) || defined(SENSOR_SHT31)

    if (setupHumidity()) {
        sensor_state |= (1 << 2);

    } else {
        sensor_state &= ~(1 << 2);

#if defined(DEBUG)
        Serial.println("[SENSOR] Humidity sensor did not respond. Please check wiring.");
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

#if defined(SENSOR_BME280) || defined(SENSOR_HTU21D) || defined(SENSOR_SHT31)
    float abs_humidity = NAN;
    float rel_humidity = NAN;
    float temp_hum = NAN;
#endif

#if defined(SENSOR_HTU21D) || defined(SENSOR_SHT31)
#if defined(SENSOR_HTU21D)
    temp_hum = htu.readTemperature();
    rel_humidity = htu.readHumidity();

    if (temp_hum >= 255.0) {
        temp_hum = NAN;
    }

    if (rel_humidity > 100.0 || rel_humidity < 0.0) {
        rel_humidity = NAN;
    }

#elif defined(SENSOR_SHT31)
    SHT31D result = sht31.periodicFetchData();

    if (result.error == SHT3XD_NO_ERROR) {
        temp_hum = result.t;
        rel_humidity = result.rh;
    }

#endif

    if (!isnan(temp_hum)) {
        if ((sensor_state & 0b100) == 0) {
            if (setupHumidity()) {
                sensor_state |= (1 << 2);

                humidity_filter[0].reset();  // rel
                humidity_filter[1].reset();  // abs
                humidity_temp_filter.reset();

#if defined(DEBUG)
                Serial.println("[SENSOR] HTU21D is back on.");
#endif

            } else {
                sensor_state &= ~(1 << 2);
#if defined(DEBUG)
                Serial.println("[SENSOR] Humidity sensor could not init again.");
#endif
            }
        }

        if (sensor_state & 0b100) {
            abs_humidity = round2(humidity_filter[1].add(meteoFunctions.absoluteHumidity_c(temp_hum, rel_humidity)));
            rel_humidity = round2(humidity_filter[0].add(rel_humidity));
            temp_hum = round2(humidity_temp_filter.add(temp_hum));
        }

    } else {
        if (sensor_state & 0b100) {
            sensor_state &= ~(1 << 2);  // Humidity not available

#if defined(DEBUG)
            Serial.println("[SENSOR] Humidity sensor stopped responding. Please check wiring.");
#endif
        }

        abs_humidity = NAN;
        rel_humidity = NAN;
        temp_hum = NAN;
    }

#endif

#if defined(SENSOR_MCP9808)
    float temp_mcp = mcp.readTempC();

    if (!isnan(temp_mcp)) {
        if ((sensor_state & 0b10) == 0) {
            if (setupMCP9808()) {
                sensor_state |= (1 << 1);

                temp_filter.reset();

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
            temp_mcp = round2(temp_filter.add(temp_mcp));
        }

    } else {
        if (sensor_state & 0b10) {
            sensor_state &= ~(1 << 1);  // MCP not available

#if defined(DEBUG)
            Serial.println("[SENSOR] MCP9808 stopped responding. Please check wiring.");
#endif
        }

        temp_mcp = NAN;
    }

#endif

#if defined(SENSOR_BMP280) || defined(SENSOR_BME280) || defined(SENSOR_LPS35HW)
    float temp_baro = NAN;
    float abs_pressure = NAN;
    float rel_pressure = NAN;

#if defined(SENSOR_BMP280) || defined(SENSOR_BME280)
    bmp.read(abs_pressure, temp_baro, rel_humidity);
#elif defined(SENSOR_LPS35HW)
    abs_pressure = lps.readPressure();
    temp_baro = lps.readTemp();
#endif

    if (!isnan(temp_baro) && !isnan(abs_pressure)) {
        if ((sensor_state & 0b1) == 0) {
            if (setupBaro()) {
                sensor_state |= (1 << 0);

                baro_filter[0].reset();  // rel
                baro_filter[1].reset();  // abs
                baro_temp_filter.reset();

#if defined(SENSOR_BME280)
                humidity_filter[0].reset();  // rel
                humidity_filter[1].reset();  // abs
#endif


#if defined(DEBUG)
                Serial.println("[SENSOR] Barometer is back on.");
#endif

            } else {
                sensor_state &= ~(1 << 0);

#if defined(DEBUG)
                Serial.println("[SENSOR] Barometer could not init.");
#endif
            }
        }

        if (sensor_state & 0b1) {
            rel_pressure = round2(baro_filter[0].add(meteoFunctions.relativePressure_c(abs_pressure, height_above_sea, temp_baro)));
            abs_pressure = round2(baro_filter[1].add(abs_pressure));
            temp_baro = round2(baro_temp_filter.add(temp_baro));

#if defined(SENSOR_BME280)
            abs_humidity = round2(humidity_filter[1].add(meteoFunctions.absoluteHumidity_c(temp_baro, rel_humidity)));
            rel_humidity = round2(humidity_filter[0].add(rel_humidity));
            temp_hum = temp_baro;
#endif
        }

    } else {
        if (sensor_state & 0b1) {
            sensor_state &= ~(1 << 0);  // Baro not available

#if defined(DEBUG)
            Serial.println("[SENSOR] Barometer stopped responding again. Please check wiring.");
#endif
        }

        rel_pressure = NAN;
        abs_pressure = NAN;
        temp_baro = NAN;

#if defined(SENSOR_BME280)
        rel_humidity = NAN;
        abs_humidity = NAN;
        temp_hum = NAN;
#endif
    }

#endif

#if defined(DEBUG)
#if defined(SENSOR_BMP280) || defined(SENSOR_BME280) || defined(SENSOR_LPS35HW)
    Serial.print("[SENSOR] Pressure: ");
    Serial.print(abs_pressure, 2);
    Serial.print(" hPa, ");
    Serial.print(rel_pressure, 2);
    Serial.println(" hPa");
    Serial.print("[SENSOR] Pressure temp: ");
    Serial.print(temp_baro);
    Serial.println(" C");
#endif

#if defined(SENSOR_MCP9808)
    Serial.print("[SENSOR] Temp MCP: ");
    Serial.print(temp_mcp, 2);
    Serial.println(" C");
#endif

#if defined(SENSOR_BME280) || defined(SENSOR_HTU21D) || defined(SENSOR_SHT31)
    Serial.print("[SENSOR] Humidity: ");
    Serial.print(abs_humidity, 2);
    Serial.print(" g/m3, ");
    Serial.print(rel_humidity, 2);
    Serial.println(" %");
#endif

#if defined(SENSOR_HTU21D) || defined(SENSOR_SHT31)
    Serial.print("[SENSOR] Humidity temp: ");
    Serial.print(temp_hum, 2);
    Serial.println(" C");
#endif

    Serial.println();
#endif

    counter++;

    if (counter > READ_SAMPLES) {  // time to send data
        counter = 0;

        if (mqtt.connected()) {
            StaticJsonDocument < (JSON_OBJECT_SIZE(8) + JSON_ARRAY_SIZE(3)) > json;

#if defined(SENSOR_BMP280) || defined(SENSOR_BME280) || defined(SENSOR_LPS35HW)

            if (!isnan(rel_pressure)) {
                json["pressure_rel"] = rel_pressure;
            }

            if (!isnan(abs_pressure)) {
                json["pressure_abs"] = abs_pressure;
            }

#endif

#if defined(SENSOR_BMP280) || defined(SENSOR_BME280) || defined(SENSOR_MCP9808) || defined(SENSOR_HTU21D) || defined(SENSOR_SHT31)

            if (sensor_state > 0) {
                JsonArray temperatures = json.createNestedArray("temp");
#if defined(SENSOR_BMP280) || defined(SENSOR_BME280) || defined(SENSOR_LPS35HW)
                temperatures.add(temp_baro);
#endif

#if defined(SENSOR_MCP9808)
                temperatures.add(temp_mcp);
#endif

#if defined(SENSOR_HTU21D) || defined(SENSOR_SHT31)
                temperatures.add(temp_hum);
#endif
            }

#endif

#if defined(SENSOR_BME280) || defined(SENSOR_HTU21D) || defined(SENSOR_SHT31)

            if (!isnan(rel_humidity)) {
                json["humidity_rel"] = rel_humidity;
            }

            if (!isnan(abs_humidity)) {
                json["humidity_abs"] = abs_humidity;
            }

            if (!isnan(temp_hum) && !isnan(rel_humidity)) {
                json["dew"] = round2(meteoFunctions.dewPoint_c(temp_hum, rel_humidity));
                json["humidex"] = round2(meteoFunctions.humidex_c(temp_hum, rel_humidity));
                json["heat"] = round2(meteoFunctions.heatIndex_c(temp_hum, rel_humidity));
                json["cloud"] = round2(meteoFunctions.cloudBase_m(temp_hum, rel_humidity));
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
