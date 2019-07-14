void onWifiConnect(const WiFiEventStationModeGotIP& event) {
#if defined(DEBUG)
    Serial.println("[WIFI] Connected");
    Serial.print("[WIFI] Local IP: ");
    Serial.println(WiFi.localIP());
#endif

    connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
#if defined(DEBUG)
    Serial.println("[WIFI] Disconnected");
#endif
    mqttReconnectTimer.detach();
}

void saveConfig() {
#if defined(DEBUG)
    Serial.println("[FS] Saving config");
#endif

    // read updated parameters
    strncpy(mqtt_server, custom_mqtt_server.getValue(), sizeof(mqtt_server));
    mqtt_port = custom_mqtt_port.getValue();
    strncpy(mqtt_user, custom_mqtt_user.getValue(), sizeof(mqtt_user));
    strncpy(mqtt_password, custom_mqtt_password.getValue(), sizeof(mqtt_password));

#if defined(SENSOR_BMP280) || defined(SENSOR_BME280) || defined(SENSOR_LPS35HW)
    height_above_sea = custom_height_above_sea.getValue();
#endif

#if defined(NOFUSS_OTA)
    strncpy(nofuss_server, custom_nofuss_server.getValue(), sizeof(nofuss_server));
#endif

    DynamicJsonDocument json(1024);
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["mqtt_user"] = mqtt_user;
    json["mqtt_password"] = mqtt_password;

#if defined(SENSOR_BMP280) || defined(SENSOR_BME280) || defined(SENSOR_LPS35HW)
    json["height_above_sea"] = height_above_sea;
#endif

#if defined(NOFUSS_OTA)
    json["nofuss_server"] = nofuss_server;
#endif

    File configFile = SPIFFS.open(CONFIG_PATH, "w");

    if (!configFile) {
#if defined(DEBUG)
        Serial.println("[FS] Failed to open config file");
#endif

    } else {
#if defined(DEBUG)
        serializeJson(json, Serial);
        Serial.println();
#endif

        serializeJson(json, configFile);
    }

    configFile.close();

    wifiManager.reboot();
}


void startConfigPortal() {
#if defined(DEBUG)
    Serial.println("[WIFI] Starting configuration portal");
#endif
    wifiManager.stopWebPortal();
    wifiManager.setConfigPortalBlocking(true);
    wifiManager.startConfigPortal(CONFIG_AP_SSID, CONFIG_AP_PASSWORD);

#if defined(DEBUG)
    Serial.println("[WIFI] Ending configuration portal");
#endif

    wifiManager.setConfigPortalBlocking(false);
}

void endConfigPortal() {
#if defined(DEBUG)
    Serial.println("[WIFI] Force ending configuration portal");
#endif

    wifiManager.stopConfigPortal();
    wifiManager.startWebPortal();
}

bool loadDefaultConfig() {
#if defined(DEBUG)
    Serial.println("[FS] mounting...");
#endif

    if (SPIFFS.begin()) {
#if defined(DEBUG)
        Serial.println("[FS] mounted");
#endif

        DynamicJsonDocument json(JSON_OBJECT_SIZE(5) + 130);
        File configFile;

        if (SPIFFS.exists(CONFIG_PATH)) {
#if defined(DEBUG)
            Serial.println("[FS] Reading config file...");
#endif

            configFile = SPIFFS.open(CONFIG_PATH, "r");

            if (configFile) {
#if defined(DEBUG)
                Serial.println("[FS] Parsing JSON...");
#endif

                size_t size = configFile.size();
                // Allocate a buffer to store contents of the file.
                std::unique_ptr<char[]> buf(new char[size]);

                configFile.readBytes(buf.get(), size);
                DeserializationError error = deserializeJson(json, buf.get());


#if defined(DEBUG)
                serializeJsonPretty(json, Serial);
                Serial.println();
#endif
                configFile.close();

                if (error) {
#if defined(DEBUG)
                    Serial.println("[FS] Parsings JSON failed");
#endif

                } else {
#if defined(DEBUG)
                    Serial.println("[FS] JSON parsed");
#endif
                    const char * server = json["mqtt_server"];
                    const char * user = json["mqtt_user"];
                    const char * password = json["mqtt_password"];

                    if (server) {
                        strncpy(mqtt_server, json["mqtt_server"], sizeof(mqtt_server));
                    }

                    mqtt_port = json["mqtt_port"];

                    if (mqtt_port == 0) {
                        mqtt_port = DEFAULT_MQTT_PORT;
                    }

                    if (user) {
                        strncpy(mqtt_user, json["mqtt_user"], sizeof(mqtt_user));
                    }

                    if (password) {
                        strncpy(mqtt_password, json["mqtt_password"], sizeof(mqtt_password));
                    }

#if defined(SENSOR_BMP280) || defined(SENSOR_BME280) || defined(SENSOR_LPS35HW)

                    height_above_sea = json["height_above_sea"];

                    if (height_above_sea <= 0) {
                        height_above_sea = DEFAULT_HEIGHT_ABOVE_SEA;
                    }

#endif

#if defined(NOFUSS_OTA)
                    const char * nofuss = json["nofuss_server"];

                    if (nofuss) {
                        strncpy(nofuss_server, json["nofuss_server"], sizeof(nofuss_server));
                    }

#endif

                    return true;
                }
            }

        } else {
#if defined(DEBUG)
            Serial.println("[FS] Formatting");
#endif
            SPIFFS.format();
            configFile = SPIFFS.open(CONFIG_PATH, "w");

            if (!configFile) {
                Serial.println("Failed to create file");
                return false;
            }

            json["mqtt_server"] = "192.168.1.100";
            json["mqtt_port"] = DEFAULT_MQTT_PORT;
            json["mqtt_user"] = "";
            json["mqtt_password"] = "";

#if defined(SENSOR_BMP280) || defined(SENSOR_BME280) || defined(SENSOR_LPS35HW)

            json["height_above_sea"] = DEFAULT_HEIGHT_ABOVE_SEA;
#endif

#if defined(NOFUSS_OTA)
            json["nofuss_server"] = "";
#endif

            if (serializeJson(json, configFile) == 0) {
#if defined(DEBUG)
                Serial.println("Failed to write to file");
#endif

            } else {
#if defined(DEBUG)
                Serial.println("Saved");
#endif
            }

            configFile.close();
        }

    } else {
#if defined(DEBUG)
        Serial.println("[FS] Failed to mount");
#endif
    }

    return false;
}

void wifiSetup() {
    wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
    wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

    wifiManager.setHostname(DEVICE_NAME);
    wifiManager.setWiFiAutoReconnect(true);
    wifiManager.setConfigPortalTimeout(CONFIG_PORTAL_TIMEOUT);
    wifiManager.setConfigPortalBlocking(false);
    wifiManager.setEnableConfigPortal(false);
    wifiManager.setSaveParamsCallback(saveConfig);
    wifiManager.setBreakAfterConfig(true);

    std::vector<const char *> menu = {"wifi", "param", "restart", "exit"};
    wifiManager.setMenu(menu);

#if defined(DEBUG)
    wifiManager.setDebugOutput(true);
#else
    wifiManager.setDebugOutput(false);
#endif

    custom_mqtt_server.setValue(mqtt_server, sizeof(mqtt_server));
    custom_mqtt_port.setValue(mqtt_port, 4);
    custom_mqtt_user.setValue(mqtt_user, sizeof(mqtt_user));
    custom_mqtt_password.setValue(mqtt_password, sizeof(mqtt_password));

    wifiManager.addParameter(&custom_mqtt_server);
    wifiManager.addParameter(&custom_mqtt_port);
    wifiManager.addParameter(&custom_mqtt_user);
    wifiManager.addParameter(&custom_mqtt_password);

#if defined(SENSOR_BMP280) || defined(SENSOR_BME280) || defined(SENSOR_LPS35HW)
    custom_height_above_sea.setValue(height_above_sea, 4);
    wifiManager.addParameter(&custom_height_above_sea);
#endif

#if defined(NOFUSS_OTA)
    custom_nofuss_server.setValue(nofuss_server, sizeof(nofuss_server));
    wifiManager.addParameter(&custom_nofuss_server);
#endif

#if defined(DEBUG)
    Serial.print("[WIFI] Connecting to: ");
    Serial.println(WiFi.SSID());
#endif

    wifiManager.autoConnect(CONFIG_AP_SSID, CONFIG_AP_PASSWORD);
}
