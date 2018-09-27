#include "const.h"
#include "wifi.h"
#include "mqtt.h"
#include "ota.h"
#include "button.h"
#include "sensors.h"
#include "meters.h"

void setup() {
#if defined(DEBUG)
    Serial.begin(115200);
    Serial.println();
#endif

    wifiSetup();
    otaSetup();
    mqttSetup();
    sensorsSetup();
    metersSetup();
}

void loop() {
    otaLoop();

    if (!ota_in_progess) {
        buttonLoop();
        mqttLoop();
        sensorsLoop();
        metersLoop();
    }
}
