#include <ESP8266WiFi.h>
#include <AsyncMqttClient.h>

AsyncMqttClient mqtt;
char will[40];

#if defined(HTTP_OTA)
    void httpUpdate(const char* url);
#endif

void sendStatus() {
    if (mqtt.connected()) {
        StaticJsonBuffer < JSON_OBJECT_SIZE(5) > jsonBuffer;
        JsonObject& json = jsonBuffer.createObject();
        char message[140];

        json["status"] = MQTT_STATUS_ALIVE;
        json["ip"] = WiFi.localIP().toString();
        json["rssi"] = WiFi.RSSI();
        json["sensors"] = sensor_state;

        uint32_t len = json.printTo(message, sizeof(message));

#if defined(DEBUG)
        Serial.println("[MQTT] Sending status data:");
        json.prettyPrintTo(Serial);
        Serial.println();
#endif

        mqtt.publish(MQTT_STATUS_TOPIC, MQTT_QOS, MQTT_RETAIN, message, len);
    }
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
#if defined(DEBUG)
    Serial.print("[MQTT] Message arrived [");
    Serial.print(topic);
    Serial.print("]: ");

    for (uint16_t i = 0; i < len; i++) {
        Serial.print(static_cast<char>(payload[i]));
    }

    Serial.println();
#endif

    if (strcmp(topic, MQTT_HEIGHT_UPDATE_TOPIC) == 0 && len > 0) {
#if defined(SENSOR_BMP280)

        if (atoi(payload) > 0) {
            memcpy(height_above_sea, payload, len);
            saveConfig();

            // confirm
            mqtt.publish(MQTT_HEIGHT_NEW_TOPIC, MQTT_QOS, false, payload, len);
        }

#endif

    } else if (strcmp(topic, MQTT_UPGRADE_TOPIC) == 0 && len >= 4) {
#if defined(HTTP_OTA)

        if (len > sizeof(http_ota_url)) {
            mqtt.publish(MQTT_UPGRADE_STATUS_TOPIC, MQTT_QOS, false, "URL too long");

        } else {
            memset(http_ota_url, 0, sizeof(http_ota_url));
            memcpy(http_ota_url, payload, len);
            do_http_update = true;
        }

#endif

    } else if (strcmp(topic, MQTT_RESTART_TOPIC) == 0) {
        delay(2000);
        ESP.restart();
        delay(5000);
    }
}

void onMqttConnect(bool sessionPresent) {
#if defined(DEBUG)
    Serial.print("[MQTT] Connected, rc=");
    Serial.println(sessionPresent);
#endif

#if defined(SENSOR_BMP280)
    mqtt.subscribe(MQTT_HEIGHT_UPDATE_TOPIC, MQTT_QOS);
#endif
    mqtt.subscribe(MQTT_RESTART_TOPIC, MQTT_QOS);

#if defined(HTTP_OTA)
    mqtt.subscribe(MQTT_UPGRADE_TOPIC, MQTT_QOS);
#endif

    sendStatus();
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
    if (!ota_in_progess) {
#if defined(DEBUG)

        if ((int8_t)reason != 0) {
            Serial.print("[MQTT] Not connected, rc=");
            Serial.println((int8_t)reason);
        }

#endif

        if (WiFi.isConnected()) {
            mqtt.connect();
        }
    }
}

void mqttSetup() {
#if defined(DEBUG)
    Serial.println("[MQTT] Setup OK");
#endif

    IPAddress ip;

    if (ip.fromString(mqtt_server)) {  // check if server is IP address or hostname
        mqtt.setServer(ip, atoi(mqtt_port));

    } else {
        mqtt.setServer(mqtt_server, atoi(mqtt_port));
    }

    snprintf(will, sizeof(will), "{\"status\": %i}", MQTT_STATUS_DEAD);

    mqtt.setCredentials(mqtt_user, mqtt_password);
    mqtt.setWill(MQTT_STATUS_TOPIC, MQTT_QOS, MQTT_RETAIN, will, strlen(will));
    mqtt.setKeepAlive((MQTT_STATUS_INTERVAL / 1000) + 2);  // converts ms->s + 2 sec extra, in case we have a delay
    mqtt.onConnect(onMqttConnect);
    mqtt.onDisconnect(onMqttDisconnect);
    mqtt.onMessage(onMqttMessage);

    mqtt.connect();
}

void mqttLoop() {
    static uint32_t last_check = 0;
    static uint32_t last_status = 0;

    if (!ota_in_progess) {
        if ((millis() - last_check) >= MQTT_CHECK_INTERVAL) {
            last_check = millis();

            if (!mqtt.connected()) {
                mqtt.connect();
            }
        }

        if ((millis() - last_status) >= MQTT_STATUS_INTERVAL) {
            last_status = millis();

            sendStatus();
        }
    }
}
