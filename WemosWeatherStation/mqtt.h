#include <ESP8266WiFi.h>
#include <AsyncMqttClient.h>

AsyncMqttClient mqtt;

void sendStatus() {
    if (mqtt.connected()) {
#if defined(DEBUG)
        Serial.println("[MQTT] Sending status data");
#endif

        // alive message
        mqtt.publish(MQTT_STATUS_TOPIC, MQTT_QOS, MQTT_RETAIN, MQTT_STATUS_ALIVE, strlen(MQTT_STATUS_ALIVE));

        // IP address
        mqtt.publish(MQTT_IP_TOPIC, MQTT_QOS, MQTT_RETAIN, WiFi.localIP().toString().c_str(), 16);

        // RSSI
        char buf[5];
        snprintf(buf, sizeof(buf), "%i", WiFi.RSSI());
        mqtt.publish(MQTT_RSSI_TOPIC, MQTT_QOS, MQTT_RETAIN, buf, strlen(buf));
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
}

void onMqttConnect(bool sessionPresent) {
#if defined(DEBUG)
    Serial.print("[MQTT] Connected, rc=");
    Serial.println(sessionPresent);
#endif

    sendStatus();
    mqtt.subscribe(MQTT_CMD_TOPIC, MQTT_QOS);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
    if (!ota_in_progess) {
#if defined(DEBUG)
        Serial.print("[MQTT] Not connected, rc=");
        Serial.println((int8_t)reason);
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

    mqtt.setCredentials(mqtt_user, mqtt_password);
    mqtt.setWill(MQTT_STATUS_TOPIC, MQTT_QOS, MQTT_RETAIN, MQTT_STATUS_DEAD, strlen(MQTT_STATUS_DEAD));
    mqtt.setKeepAlive((MQTT_STATUS_INTERVAL / 1000) + 2);  // converts ms->s + 2 sec extra, in case we have a delay
    mqtt.onConnect(onMqttConnect);
    mqtt.onDisconnect(onMqttDisconnect);
    mqtt.onMessage(onMqttMessage);

    mqtt.connect();
}

void mqttLoop() {
    static uint32_t last_check = 0;
    static uint32_t last_status = 0;

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
