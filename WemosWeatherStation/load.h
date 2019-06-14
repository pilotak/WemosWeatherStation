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

class IntParameter : public WiFiManagerParameter {
  public:
    IntParameter(const char *id, const char *placeholder, int32_t value, const uint8_t length = 10, const char *custom = NULL)
        : WiFiManagerParameter("") {
        const char * type_number = "type=\"number\" ";
        uint16_t custom_len = 0;

        if (custom) {
            custom_len = strlen(custom);
        }

        uint16_t custom_buffer_len = custom_len + strlen(type_number) + 1;

        custom_buffer = new char[custom_buffer_len];

        if (custom_buffer) {
            int offset = snprintf(custom_buffer, custom_buffer_len, type_number);

            if (custom_len > 0 && offset > 0) {
                snprintf(custom_buffer + offset, custom_buffer_len - offset, custom);
            }

            init(id, placeholder, String(value).c_str(), length, custom_buffer, WFM_LABEL_BEFORE);
        }
    }

    ~IntParameter() {
        if (custom_buffer) {
            delete[] custom_buffer;
        }
    }

    int32_t getValue() {
        return String(WiFiManagerParameter::getValue()).toInt();
    }

    void setValue(int32_t value, uint8_t max_size) {
        char *buffer = new char[max_size];

        if (buffer) {
            itoa(value, buffer, 10);
            WiFiManagerParameter::setValue(buffer, max_size);
            delete[] buffer;
        }
    }

    char *custom_buffer;
};

// mqtt.h
#include <ESP8266WiFi.h>
#include <AsyncMqttClient.h>
#include <Ticker.h>

AsyncMqttClient mqtt;
Ticker mqttReconnectTimer;

char mqtt_server[40];
uint16_t mqtt_port = DEFAULT_MQTT_PORT;
char mqtt_user[16] = {0};
char mqtt_password[32] = {0};
char will[40];

void connectToMqtt();

WiFiManagerParameter custom_mqtt_server("mqtt_server", "MQTT server", mqtt_server, sizeof(mqtt_server), "required");
IntParameter custom_mqtt_port("mqtt_port", "MQTT port", mqtt_port, 4, "required");
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

#if defined(SENSOR_BMP280) || defined(SENSOR_BME280) || defined(SENSOR_LPS35HW)
    MovingAverageFloat <READ_SAMPLES> baro_filter[2];  // rel pressure, abs pressure
    MovingAverageFloat <READ_SAMPLES> baro_temp_filter;

    uint16_t height_above_sea = DEFAULT_HEIGHT_ABOVE_SEA;

    IntParameter custom_height_above_sea("height_above_sea", "Height above sea (m)", height_above_sea, 4, "required");
#endif

#if defined(SENSOR_BME280) || defined(SENSOR_HTU21D) || defined(SENSOR_SHT31)
    MovingAverageFloat <READ_SAMPLES> humidity_filter[2];  // rel humidity, abs humidity
    MovingAverageFloat <READ_SAMPLES> humidity_temp_filter;
#endif

#if defined(SENSOR_MCP9808)
    MovingAverageFloat <READ_SAMPLES> temp_filter;
#endif

// button.h
#include <JC_Button.h>
Button button(BUTTON_PIN);
