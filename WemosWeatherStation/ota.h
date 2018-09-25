#if defined(NOFUSS_OTA)
#include <ESP8266httpUpdate.h>
#include "NoFUSSClient.h"

void otaSetup() {
  NoFUSSClient.setServer(nofuss_server);
  NoFUSSClient.setDevice(DEVICE_NAME);
  NoFUSSClient.setVersion(FW_VERSION);


  NoFUSSClient.onMessage([](nofuss_t code) {
    if (code == NOFUSS_START) {
#ifdef DEBUG
      Serial.println("[NoFUSS] Starting");
#endif
      ota_in_progess = true;
    }
#ifdef DEBUG
    if (code == NOFUSS_UPTODATE) {
      Serial.println("[NoFUSS] Nothing for me");
    }

    if (code == NOFUSS_PARSE_ERROR) {
      Serial.println("[NoFUSS] Error parsing server response");
    }

    if (code == NOFUSS_UPDATING) {
      Serial.println("[NoFUSS] Updating");
      Serial.print("         New version: ");
      Serial.println(NoFUSSClient.getNewVersion().c_str());
      Serial.print("         Firmware: ");
      Serial.println(NoFUSSClient.getNewFirmware().c_str());
      Serial.print("         File System: ");
      Serial.println(NoFUSSClient.getNewFileSystem().c_str());
    }

    if (code == NOFUSS_FILESYSTEM_UPDATE_ERROR) {
      Serial.print("[NoFUSS] File System Update Error: ");
      Serial.println(NoFUSSClient.getErrorString().c_str());
    }

    if (code == NOFUSS_FILESYSTEM_UPDATED) {
      Serial.println("[NoFUSS] File System Updated");
    }

    if (code == NOFUSS_FIRMWARE_UPDATE_ERROR) {
      Serial.print("[NoFUSS] Firmware Update Error: ");
      Serial.println(NoFUSSClient.getErrorString().c_str());
    }

    if (code == NOFUSS_FIRMWARE_UPDATED) {
      Serial.println("[NoFUSS] Firmware Updated");
    }

    if (code == NOFUSS_RESET) {
      Serial.println("[NoFUSS] Resetting board");
    }
#endif

    if (code == NOFUSS_END) {
#ifdef DEBUG
      Serial.println("[NoFUSS] End");
#endif

      ota_in_progess = false;
    }
  });
}

void otaLoop() {
  static unsigned long last_check = 0;
  if (WiFi.status() != WL_CONNECTED) return;
  if ((last_check > 0) && ((millis() - last_check) < NOFUSS_CHECK_INTERVAL)) return;
  last_check = millis();
  NoFUSSClient.handle();
}
#elif defined(OTA)
#include <ArduinoOTA.h>

void otaSetup() {
  ArduinoOTA.setPort(OTA_PORT);
  ArduinoOTA.setHostname(DEVICE_NAME);


  ArduinoOTA.onStart([]() {
#if defined(DEBUG)
    Serial.println("[OTA] Starting");
#endif
    ota_in_progess = true;
  });
  ArduinoOTA.onEnd([]() {
#if defined(DEBUG)
    Serial.println("\n[OTA] End");
#endif
    ota_in_progess = false;
  });

#if defined(DEBUG)
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.print("[OTA] Progress:");
    Serial.println((progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.print("[OTA] Error: ");
    Serial.println(error);

    if (error == OTA_AUTH_ERROR) {
      Serial.println("[OTA] Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("[OTA] Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("[OTA] Connect Failed");

    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("[OTA] Receive Failed");

    } else if (error == OTA_END_ERROR) {
      Serial.println("[OTA] End Failed");
    }

  });
#endif

  ArduinoOTA.begin();
}

void otaLoop() {

}
#else
void otaSetup() {}
void otaLoop() {}
#endif