// wifi.h
#include <FS.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266httpUpdate.h>
#include <DNSServer.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>

WiFiManager wifiManager;
WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;


// mqtt.h
#include <ESP8266WiFi.h>
#include <AsyncMqttClient.h>
#include <Ticker.h>

AsyncMqttClient mqtt;
Ticker mqttReconnectTimer;

char mqtt_server[40];
char mqtt_port[6] = DEFAULT_MQTT_PORT;
char mqtt_user[16] = {0};
char mqtt_password[32] = {0};
char will[40];

void connectToMqtt();

WiFiManagerParameter custom_mqtt_server("mqtt_server", "MQTT server", mqtt_server, sizeof(mqtt_server), "required");
WiFiManagerParameter custom_mqtt_port("mqtt_port", "MQTT port", mqtt_port, sizeof(mqtt_port), "required");
WiFiManagerParameter custom_mqtt_user("mqtt_user", "MQTT user", mqtt_user, sizeof(mqtt_user),
                                      "placeholder=\"Leave blank if not aplicable\"");
WiFiManagerParameter custom_mqtt_password("mqtt_password", "MQTT password", mqtt_password, sizeof(mqtt_password),
        "placeholder=\"Leave blank if not aplicable\" type=\"password\"");


// ota.h
#if defined(NOFUSS_OTA)
    #include "NoFUSSClient.h"
    char nofuss_server[40];
    WiFiManagerParameter custom_nofuss_server("nofuss_server", "NoFUSS server", nofuss_server, sizeof(nofuss_server), "required");
#endif

#if defined(HTTP_OTA)
    #include <ESP8266HTTPClient.h>
    bool do_http_update = false;
    char http_ota_url[100];

    void httpUpdate(const char* url);
#endif

#if defined(ARDUINO_OTA)
    #include <ArduinoOTA.h>
#endif

bool ota_in_progess = false;


// sensors.h
#include <Wire.h>
#include "MeteoFunctions.h"
#include "MovingAverageFloat.h"

MeteoFunctions meteoFunctions;
MovingAverageFloat <READ_SAMPLES> filter[6];  // BMP temp, MCP temp, HTU temp, rel pressure, abs pressure, humidity

#if defined(SENSOR_BMP280)
    char height_above_sea[8] = DEFAULT_HEIGHT_ABOVE_SEA;
    WiFiManagerParameter custom_height_above_sea("height_above_sea", "Height above sea (m)", height_above_sea, sizeof(height_above_sea),
    "required");
#endif


// button.h
#include <JC_Button.h>
Button button(BUTTON_PIN);
