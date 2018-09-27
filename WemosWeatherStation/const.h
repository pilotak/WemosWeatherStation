#define DEVICE_NAME "meteo"  // used for MQTT, OTA, AP
#define FW_VERSION "1.0.0"

#define CONFIG_PATH "/config.json"
#define CONFIG_AP_SSID DEVICE_NAME "Config"
#define CONFIG_AP_PASSWORD "StrongPasswordHere"

#define DEFAULT_MQTT_PORT "1883"  // important to be a string
#define MQTT_QOS 1
#define MQTT_RETAIN true
#define MQTT_STATUS_INTERVAL 30000
#define MQTT_IP_TOPIC DEVICE_NAME "/ip"  // will result ie. meteo/ip
#define MQTT_RSSI_TOPIC DEVICE_NAME "/rssi"  // will result ie. meteo/rssi
#define MQTT_STATUS_TOPIC DEVICE_NAME "/status"  // will result ie. meteo/status
#define MQTT_CMD_TOPIC DEVICE_NAME "/cmd"  // will result ie. meteo/cmd
#define MQTT_SENSORS_TOPIC DEVICE_NAME "/sensors"  // will result ie. meteo/sensors
#define MQTT_WIND_TOPIC DEVICE_NAME "/wind"  // will result ie. meteo/wind
#define MQTT_RAIN_TOPIC DEVICE_NAME "/rain"  // will result ie. meteo/rain
#define MQTT_STATUS_ALIVE "1"
#define MQTT_STATUS_DEAD "0"

#define BUTTON_PIN D3
#define LONG_PRESS 1000  // ms

#define READ_INTERVAL 3750  // ms
#define SAMPLES 16  // READ_INTERVAL*SAMPLES=sending time in ms
#define METERS_INTERVAL 60  // how ofter send data from meters in sec
#define METERS_DIR_FILTERING 20  // filter last 20 samples of wind vane
#define HEIGHT_ABOVE_SEA 415.0  // m

#define BMP280_ADDR 0x76
#define MCP9808_ADDR 0x18
#define HTU21D_ADDR 0x40
#define MCP3021_ADDRESS 0x48
#define ANEMOMETER_PIN D6
#define RAIN_GAUGE_PIN D5

#define NOFUSS_CHECK_INTERVAL 3600000  // 1h
#define OTA_PORT 8266  // for local updates

