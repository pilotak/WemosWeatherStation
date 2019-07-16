void otaSetup() {
#if defined(NOFUSS_OTA)
    NoFUSSClient.setServer(nofuss_server);
    NoFUSSClient.setDevice(DEVICE_NAME);
    NoFUSSClient.setVersion(FW_VERSION);

    NoFUSSClient.onMessage([](nofuss_t code) {
        if (code == NOFUSS_START) {
#if defined(DEBUG)
            Serial.println("[NoFUSS] Starting");
#endif
            ota_in_progess = true;
#if defined(HAS_METERS)
            attachInterrupt(digitalPinToInterrupt(ANEMOMETER_PIN), NULL, FALLING);
            attachInterrupt(digitalPinToInterrupt(RAIN_GAUGE_PIN), NULL, FALLING);
#endif
        }

#if defined(DEBUG)

        if (code == NOFUSS_UPTODATE) {
            Serial.println("[NoFUSS] Nothing for me");
        }

        if (code == NOFUSS_PARSE_ERROR) {
            Serial.println("[NoFUSS] Error parsing server response");
        }

        if (code == NOFUSS_UPDATING) {
            Serial.println("[NoFUSS] Updating");
            Serial.print("         New version: ");
            Serial.println(NoFUSSClient.getNewVersion().c_str());
            Serial.print("         Firmware: ");
            Serial.println(NoFUSSClient.getNewFirmware().c_str());
            Serial.print("         File System: ");
            Serial.println(NoFUSSClient.getNewFileSystem().c_str());
        }

        if (code == NOFUSS_FILESYSTEM_UPDATE_ERROR) {
            Serial.print("[NoFUSS] File System Update Error: ");
            Serial.println(NoFUSSClient.getErrorString().c_str());
        }

        if (code == NOFUSS_FILESYSTEM_UPDATED) {
            Serial.println("[NoFUSS] File System Updated");
        }

        if (code == NOFUSS_FIRMWARE_UPDATE_ERROR) {
            Serial.print("[NoFUSS] Firmware Update Error: ");
            Serial.println(NoFUSSClient.getErrorString().c_str());
        }

        if (code == NOFUSS_FIRMWARE_UPDATED) {
            Serial.println("[NoFUSS] Firmware Updated");
        }

        if (code == NOFUSS_RESET) {
            Serial.println("[NoFUSS] Resetting board");
        }

#endif

        if (code == NOFUSS_END) {
#if defined(DEBUG)
            Serial.println("[NoFUSS] End");
#endif

            ota_in_progess = false;
        }
    });
#endif

#if defined(ARDUINO_OTA)
    ArduinoOTA.setPort(OTA_PORT);
    ArduinoOTA.setHostname(DEVICE_NAME);

    ArduinoOTA.onStart([]() {
#if defined(DEBUG)
        Serial.println("[OTA] Starting");
#endif
        ota_in_progess = true;
    });
    ArduinoOTA.onEnd([]() {
#if defined(DEBUG)
        Serial.print("\n\e[0E");  // new line + move to the beginning of the current line
        Serial.println("[OTA] End");
        delay(200);
#endif
    });

#if defined(DEBUG)
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.print("\e[2K");  // clear line
        Serial.print("\e[0E");  // move to the beginning of the current line
        Serial.print("[OTA] Progress: ");
        Serial.print((progress / (total / 100)));
        Serial.print("%");
    });
#endif
    ArduinoOTA.onError([](ota_error_t error) {
        ota_in_progess = false;

#if defined(DEBUG)
        Serial.print("\n\e[0E");  // new line + move to the beginning of the current line
        Serial.print("[OTA] Error: ");
        Serial.println(error);

        if (error == OTA_AUTH_ERROR) {
            Serial.println("[OTA] Auth Failed");

        } else if (error == OTA_BEGIN_ERROR) {
            Serial.println("[OTA] Begin Failed");

        } else if (error == OTA_CONNECT_ERROR) {
            Serial.println("[OTA] Connect Failed");

        } else if (error == OTA_RECEIVE_ERROR) {
            Serial.println("[OTA] Receive Failed");

        } else if (error == OTA_END_ERROR) {
            Serial.println("[OTA] End Failed");
        }

#endif
    });


    ArduinoOTA.begin();
#endif
}

void otaLoop() {
#if defined(NOFUSS_OTA)
    static uint32_t last_check = 0;

    if (WiFi.status() != WL_CONNECTED) return;

    if ((last_check > 0) && ((millis() - last_check) < NOFUSS_CHECK_INTERVAL)) return;

    last_check = millis();
    NoFUSSClient.handle();
#endif

#if defined(ARDUINO_OTA)
    ArduinoOTA.handle();
#endif
}

#if defined(HTTP_OTA)
void httpUpdate() {
    char msg[127];
    uint32_t len = 0;
    ota_in_progess = true;
    WiFiClient client;

    ESPhttpUpdate.rebootOnUpdate(false);
    ESPhttpUpdate.followRedirects(true);

#if defined(DEBUG)
    Serial.print("[OTA] Starting HTTP update from: ");
    Serial.println(http_ota_url);
#endif

    t_httpUpdate_return ret = ESPhttpUpdate.update(client, http_ota_url, FW_VERSION);

    switch (ret) {
        case HTTP_UPDATE_FAILED:
            ota_in_progess = false;

            len = snprintf(msg, sizeof(msg), "%s", ESPhttpUpdate.getLastErrorString().c_str());
            mqtt.publish(MQTT_UPGRADE_STATUS_TOPIC, MQTT_QOS, false, msg, len);

#if defined(DEBUG)
            Serial.printf("[OTA] HTTP update failed: (%d): %s\r\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
#endif
            break;

        case HTTP_UPDATE_NO_UPDATES:
            ota_in_progess = false;
            mqtt.publish(MQTT_UPGRADE_STATUS_TOPIC, MQTT_QOS, false, "No updates");

#if defined(DEBUG)
            Serial.println("[OTA] HTTP update: no updates");
#endif
            break;

        case HTTP_UPDATE_OK:
            mqtt.publish(MQTT_UPGRADE_STATUS_TOPIC, MQTT_QOS, false, "OK");

#if defined(DEBUG)
            Serial.println("[OTA] HTTP update: OK");
#endif

            delay(1000);
            wifiManager.reboot();
            break;
    }
}
#endif
