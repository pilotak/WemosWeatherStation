#define DEVICE_NAME "meteo"  // used for MQTT, OTA, AP
#define FW_VERSION "1.0.0"

#define CONFIG_PATH "/config.json"
#define CONFIG_AP_SSID DEVICE_NAME "Config"
#define CONFIG_AP_PASSWORD "StrongPasswordHere"

#define DEFAULT_MQTT_PORT "1883"  // important to be a string
#define MQTT_QOS 1
#define MQTT_RETAIN true
#define MQTT_CHECK_INTERVAL 5000
#define MQTT_STATUS_INTERVAL 30000
#define MQTT_STATUS_TOPIC DEVICE_NAME "/status"  // will result ie. meteo/status
#define MQTT_UPGRADE_TOPIC DEVICE_NAME "/upgrade"  // will result ie. meteo/upgrade
#define MQTT_UPGRADE_STATUS_TOPIC DEVICE_NAME "/upgrade/status"  // will result ie. meteo/upgrade/status
#define MQTT_RESTART_TOPIC DEVICE_NAME "/restart"  // will result ie. meteo/restart
#define MQTT_HEIGHT_UPDATE_TOPIC DEVICE_NAME "/height"  // will result ie. meteo/height
#define MQTT_HEIGHT_NEW_TOPIC DEVICE_NAME "/height/new"  // will result ie. meteo/height/new
#define MQTT_SENSORS_TOPIC DEVICE_NAME "/sensors"  // will result ie. meteo/sensors
#define MQTT_WIND_TOPIC DEVICE_NAME "/wind"  // will result ie. meteo/wind
#define MQTT_RAIN_TOPIC DEVICE_NAME "/rain"  // will result ie. meteo/rain
#define MQTT_STATUS_ALIVE 1
#define MQTT_STATUS_DEAD 0

#define BUTTON_PIN D3
#define LONG_PRESS 1000  // ms

#define READ_INTERVAL 3750  // ms
#define READ_SAMPLES 16  // READ_INTERVAL*SAMPLES=sending time in ms
#define METERS_INTERVAL 60  // how ofter send data from meters in sec
#define METERS_DIR_FILTERING 20  // filter last 20 samples of wind vane
#define DEFAULT_HEIGHT_ABOVE_SEA "415"  // m; important to be a string

#define BMP280_ADDR BME280I2C::I2CAddr_0x76  // or BME280I2C::I2CAddr_0x77
#define MCP9808_ADDR 0x18
#define HTU21D_ADDR 0x40
#define MCP3021_ADDRESS 0x48
#define ANEMOMETER_PIN D6
#define RAIN_GAUGE_PIN D5

#define NOFUSS_CHECK_INTERVAL 3600000  // 1h
#define OTA_PORT 8266  // for local updates

uint8_t sensor_state = 0b000;  // BMP, MCP, HTU
