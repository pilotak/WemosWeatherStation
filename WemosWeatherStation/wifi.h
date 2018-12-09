#include <FS.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>

char mqtt_server[40];
char mqtt_port[6] = DEFAULT_MQTT_PORT;
char mqtt_user[16] = {0};
char mqtt_password[32] = {0};
char height_above_sea[8] = DEFAULT_HEIGHT_ABOVE_SEA;

#if defined(HTTP_OTA)
bool do_http_update = false;
char http_ota_url[100];
#endif

#if defined(NOFUSS_OTA)
    char nofuss_server[40];
#endif

WiFiEventHandler wifiDisconnectHandler;
bool ota_in_progess = false;

#if defined(NOFUSS_OTA)
    WiFiManagerParameter custom_nofuss_server("nofuss_server", "NoFUSS server", nofuss_server, sizeof(nofuss_server));
#endif

bool should_save_config = false;

void saveConfigCallback() {
    should_save_config = true;
}

void saveConfig() {
#if defined(DEBUG)
    Serial.println("[FS] saving config");
#endif

    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["mqtt_user"] = mqtt_user;
    json["mqtt_password"] = mqtt_password;
    json["height_above_sea"] = height_above_sea;

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
        json.printTo(Serial);
        Serial.println();
#endif

        json.printTo(configFile);
    }

    configFile.close();
}

void configPortal(bool mode) {
    WiFiManagerParameter custom_mqtt_server("mqtt_server", "MQTT server", mqtt_server, sizeof(mqtt_server));
    WiFiManagerParameter custom_mqtt_port("mqtt_port", "MQTT port", mqtt_port, sizeof(mqtt_port));
    WiFiManagerParameter custom_mqtt_user("mqtt_user", "MQTT user", mqtt_user, sizeof(mqtt_user));
    WiFiManagerParameter custom_mqtt_password("mqtt_password", "MQTT password", mqtt_password, sizeof(mqtt_password));
    WiFiManagerParameter custom_height_above_sea("height_above_sea", "Height above sea (m)", height_above_sea, sizeof(height_above_sea));

    WiFiManager wifiManager;
    wifiManager.setSaveConfigCallback(saveConfigCallback);

#if defined(DEBUG)
    wifiManager.setDebugOutput(true);
#else
    wifiManager.setDebugOutput(false);
#endif

    wifiManager.addParameter(&custom_mqtt_server);
    wifiManager.addParameter(&custom_mqtt_port);
    wifiManager.addParameter(&custom_mqtt_user);
    wifiManager.addParameter(&custom_mqtt_password);
    wifiManager.addParameter(&custom_height_above_sea);

#if defined(NOFUSS_OTA)
    wifiManager.addParameter(&custom_nofuss_server);
#endif

    if (mode) {
        if (!wifiManager.autoConnect(CONFIG_AP_SSID, CONFIG_AP_PASSWORD)) {
#if defined(DEBUG)
            Serial.println("[WIFI] Failed to connect.");
#endif
            delay(2000);
            ESP.restart();
            delay(5000);
        }

    } else {
        if (!wifiManager.startConfigPortal(CONFIG_AP_SSID, CONFIG_AP_PASSWORD)) {
#if defined(DEBUG)
            Serial.println("[WIFI] Failed create AP");
#endif
            delay(2000);
            ESP.restart();
            delay(5000);
        }
    }

    if (should_save_config) {
        // read updated parameters
        strncpy(mqtt_server, custom_mqtt_server.getValue(), sizeof(mqtt_server));
        strncpy(mqtt_port, custom_mqtt_port.getValue(), sizeof(mqtt_port));
        strncpy(mqtt_user, custom_mqtt_user.getValue(), sizeof(mqtt_user));
        strncpy(mqtt_password, custom_mqtt_password.getValue(), sizeof(mqtt_password));
        strncpy(height_above_sea, custom_height_above_sea.getValue(), sizeof(height_above_sea));

#if defined(NOFUSS_OTA)
        strncpy(nofuss_server, custom_nofuss_server.getValue(), sizeof(nofuss_server));
#endif

        saveConfig();
    }
}

void wifiSetup() {
    WiFi.hostname(DEVICE_NAME);

#if defined(DEBUG)
    Serial.println("[FS] mounting...");
#endif

    if (SPIFFS.begin()) {
#if defined(DEBUG)
        Serial.println("[FS] mounted");
#endif

        if (SPIFFS.exists(CONFIG_PATH)) {
#if defined(DEBUG)
            Serial.println("[FS] Reading config file...");
#endif

            File configFile = SPIFFS.open(CONFIG_PATH, "r");

            if (configFile) {
#if defined(DEBUG)
                Serial.println("[FS] Parsing JSON...");
#endif

                size_t size = configFile.size();
                // Allocate a buffer to store contents of the file.
                std::unique_ptr<char[]> buf(new char[size]);

                configFile.readBytes(buf.get(), size);
                DynamicJsonBuffer jsonBuffer;
                JsonObject& json = jsonBuffer.parseObject(buf.get());

#if defined(DEBUG)
                json.prettyPrintTo(Serial);
                Serial.println();
#endif

                if (json.success()) {
#if defined(DEBUG)
                    Serial.println("[FS] JSON parsed");
#endif

                    strncpy(mqtt_server, json["mqtt_server"], sizeof(mqtt_server));
                    strncpy(mqtt_port, json["mqtt_port"], sizeof(mqtt_port));
                    strncpy(mqtt_user, json["mqtt_user"], sizeof(mqtt_user));
                    strncpy(mqtt_password, json["mqtt_password"], sizeof(mqtt_password));
                    strncpy(height_above_sea, json["height_above_sea"], sizeof(height_above_sea));

#if defined(NOFUSS_OTA)
                    strncpy(nofuss_server, json["nofuss_server"], sizeof(nofuss_server));
#endif

                } else {
#if defined(DEBUG)
                    Serial.println("[FS] Parsings JSON failed");
#endif
                }
            }
        }

    } else {
#if defined(DEBUG)
        Serial.println("[FS] Failed to mount");
#endif
    }


    configPortal(true);
    should_save_config = false;

#if defined(DEBUG)
    Serial.println("[WIFI] Connected");
    Serial.print("[WIFI] Local IP: ");
    Serial.println(WiFi.localIP());
#endif
}
