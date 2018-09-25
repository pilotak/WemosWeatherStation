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

#if defined(NOFUSS_OTA)
char nofuss_server[40];
#endif

WiFiEventHandler wifiDisconnectHandler;
bool ota_in_progess = false;

#if defined(NOFUSS_OTA)
WiFiManagerParameter custom_nofuss_server("nofuss_server", "NoFUSS server", nofuss_server, 40);
#endif

bool should_save_config = false;

void mqttSetup();
void sendStatus();

void saveConfigCallback() {
  should_save_config = true;
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
#if defined(DEBUG)
  Serial.println("Disconnected from Wi-Fi.");
#endif
}

void configPortal(bool mode) {
  WiFiManagerParameter custom_mqtt_server("mqtt_server", "MQTT server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("mqtt_port", "MQTT port", mqtt_port, 6);
  WiFiManagerParameter custom_mqtt_user("mqtt_user", "MQTT user", mqtt_user, 16);
  WiFiManagerParameter custom_mqtt_password("mqtt_password", "MQTT password", mqtt_password, 32);

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
    //read updated parameters
    strcpy(mqtt_server, custom_mqtt_server.getValue());
    strcpy(mqtt_port, custom_mqtt_port.getValue());
    strcpy(mqtt_user, custom_mqtt_user.getValue());
    strcpy(mqtt_password, custom_mqtt_password.getValue());

#if defined(NOFUSS_OTA)
    strcpy(nofuss_server, custom_nofuss_server.getValue());
#endif

#if defined(DEBUG)
    Serial.println("[FS] saving config");
#endif

    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["mqtt_user"] = mqtt_user;
    json["mqtt_password"] = mqtt_password;

#if defined(NOFUSS_OTA)
    json["nofuss_server"] = nofuss_server;
#endif

    File configFile = SPIFFS.open(CONFIG_PATH, "w");
    if (!configFile) {
#if defined(DEBUG)
      Serial.println("[FS] Failed to open config file");
#endif
    }
#if defined(DEBUG)
    json.printTo(Serial);
#endif

    json.printTo(configFile);
    configFile.close();
  }
}

void wifiSetup() {
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);
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

          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(mqtt_user, json["mqtt_user"]);
          strcpy(mqtt_password, json["mqtt_password"]);

#if defined(NOFUSS_OTA)
          strcpy(nofuss_server, json["nofuss_server"]);
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