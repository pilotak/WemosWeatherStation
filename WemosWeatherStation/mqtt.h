#include <AsyncMqttClient.h>

AsyncMqttClient mqttClient;

void onMqttConnect(bool sessionPresent) {
#if defined(DEBUG)
    Serial.println("[MQTT] Connected");
#endif

    mqttClient.subscribe(MQTT_CMD_TOPIC, MQTT_QOS);
    sendStatus();
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
#if defined(DEBUG)
    Serial.println("[MQTT] Disconnected");
#endif
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
#if defined(DEBUG)
    Serial.println("[MQTT] Message received:");
    Serial.print("       topic: ");
    Serial.println(topic);
    Serial.print("       qos: ");
    Serial.println(properties.qos);
    Serial.print("       dup: ");
    Serial.println(properties.dup);
    Serial.print("       retain: ");
    Serial.println(properties.retain);
    Serial.print("       len: ");
    Serial.println(len);
    Serial.print("       index: ");
    Serial.println(index);
    Serial.print("       total: ");
    Serial.println(total);
#endif
}

void mqttConnect() {
#if defined(DEBUG)
    Serial.println("[MQTT] Connecting...");
#endif
    mqttClient.connect();
}

void mqttSetup() {
#if defined(DEBUG)
    Serial.println("[MQTT] Setup");
#endif
    mqttClient.onConnect(onMqttConnect);
    mqttClient.onMessage(onMqttMessage);
    mqttClient.setServer(mqtt_server, atoi(mqtt_port));
    mqttClient.setClientId(DEVICE_NAME);
    mqttClient.setKeepAlive(MQTT_STATUS_INTERVAL);
    mqttClient.setWill(MQTT_STATUS_TOPIC, MQTT_QOS, MQTT_RETAIN, MQTT_STATUS_DEAD, strlen(MQTT_STATUS_ALIVE));
    mqttConnect();
}

void sendStatus() {
    // alive message
    mqttClient.publish(MQTT_STATUS_TOPIC, MQTT_QOS, MQTT_RETAIN, MQTT_STATUS_ALIVE);

    // IP address
    mqttClient.publish(MQTT_IP_TOPIC, MQTT_QOS, MQTT_RETAIN, WiFi.localIP().toString().c_str());

    // RSSI
    char buf[5];
    snprintf(buf, sizeof(buf), "%i", WiFi.RSSI());
    mqttClient.publish(MQTT_RSSI_TOPIC, MQTT_QOS, MQTT_RETAIN, buf);
}

void mqttLoop() {
    static unsigned long last_check = 0;

    if ((last_check > 0) && ((millis() - last_check) < MQTT_STATUS_INTERVAL)) return;

    last_check = millis();
    sendStatus();
}
