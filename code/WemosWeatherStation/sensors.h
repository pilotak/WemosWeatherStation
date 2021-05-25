#if defined(SENSOR_MCP9808)
    #include "sensors/mcp9808.h"
#endif

#if defined(SENSOR_BMP280) || defined(SENSOR_BME280)
    #include "sensors/bmp280.h"
#elif defined(SENSOR_LPS35HW)
    #include "sensors/lps35hw.h"
#endif

#if defined(SENSOR_HTU21D)
    #include "sensors/htu21d.h"
#elif defined(SENSOR_SHT31)
    #include "sensors/sht31d.h"
#endif

void sensorsSetup() {
    Wire.begin(SDA, SCL);
    Wire.setClock(I2C_SPEED);

#if defined(HAS_BARO)
    setupBaro();
#endif

#if defined(HAS_TEMP)
    setupTemp();
#endif

#if defined(HAS_HUMIDITY)
    setupHumidity();
#endif
}

void sensorsLoop() {
    static uint32_t last_check = 0;
    static uint8_t counter = 0;

    if ((last_check > 0) && ((millis() - last_check) < READ_INTERVAL)) {
        return;
    }

    last_check = millis();

    sensorData data;

    data.humidity[0] = NAN;
    data.humidity[1] = NAN;
    data.temp[0] = NAN;
    data.temp[1] = NAN;
    data.temp[2] = NAN;
    data.pressure[0] = NAN;
    data.pressure[1] = NAN;

#if defined(HAS_BARO)

    if ((sensor_state & 0b1) == 0) {
        setupBaro();
    }

    if (sensor_state & 0b1) {
        readBaro(&data);
    }

#endif

#if defined(HAS_TEMP)

    if ((sensor_state & 0b10) == 0) {
        setupTemp();
    }

    if (sensor_state & 0b10) {
        readTemp(&data);
    }

#endif

#if defined(HAS_HUMIDITY)

    if ((sensor_state & 0b100) == 0) {
        setupHumidity();
    }

    if (sensor_state & 0b100) {
        readHumidity(&data);
    }

#endif

#if defined(DEBUG)
#if defined(HAS_BARO)
    Serial.print("[SENSOR] Pressure: ");
    Serial.print(data.pressure[1], 2);
    Serial.print("hPa (abs), ");
    Serial.print(data.pressure[0], 2);
    Serial.println("hPa (rel)");
    Serial.print("[SENSOR] Pressure temp: ");
    Serial.print(data.temp[0]);
    Serial.println(" C");
#endif

#if defined(HAS_TEMP)
    Serial.print("[SENSOR] Temp: ");
    Serial.print(data.temp[1], 2);
    Serial.println(" C");
#endif

#if defined(SENSOR_BME280) || defined(HAS_HUMIDITY)
    Serial.print("[SENSOR] Humidity: ");
    Serial.print(data.humidity[1], 2);
    Serial.print(" g/m3, ");
    Serial.print(data.humidity[0], 2);
    Serial.println(" %");
#endif

#if defined(HAS_HUMIDITY)
    Serial.print("[SENSOR] Humidity temp: ");
    Serial.print(data.temp[2], 2);
    Serial.println(" C");
#endif

    Serial.println();
#endif

    counter++;

    if (counter > READ_SAMPLES) {  // time to send data
        counter = 0;

        if (!mqtt.connected()) {
            return;
        }

        StaticJsonDocument < (JSON_OBJECT_SIZE(9) + JSON_ARRAY_SIZE(3)) > json;

#if defined(HAS_BARO)

        if (!isnan(data.pressure[0])) {
            json["pressure_rel"] = data.pressure[0];
        }

        if (!isnan(data.pressure[1])) {
            json["pressure_abs"] = data.pressure[1];
        }

#endif

#if defined(HAS_BARO) || defined(HAS_TEMP) || defined(HAS_HUMIDITY)

        if (sensor_state > 0) {
            JsonArray temperatures = json.createNestedArray("temp");
#if defined(HAS_BARO)
            temperatures.add(data.temp[0]);
#endif

#if defined(HAS_TEMP)
            temperatures.add(data.temp[1]);
#endif

#if defined(HAS_HUMIDITY)
            temperatures.add(data.temp[2]);
#endif
        }

#endif

#if defined(SENSOR_BME280) || defined(HAS_HUMIDITY)

        if (!isnan(data.humidity[0])) {
            json["humidity_rel"] = data.humidity[0];
        }

        if (!isnan(data.humidity[1])) {
            json["humidity_abs"] = data.humidity[1];
        }

        if (!isnan(data.temp[2]) && !isnan(data.humidity[0])) {
            json["dew"] = round2(meteoFunctions.dewPoint_c(data.temp[2], data.humidity[0]));
            json["humidex"] = round2(meteoFunctions.humidex_c(data.temp[2], data.humidity[0]));
            json["heat"] = round2(meteoFunctions.heatIndex_c(data.temp[2], data.humidity[0]));
            json["cloud"] = round2(meteoFunctions.cloudBase_m(data.temp[2], data.humidity[0]));
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
    }
}
