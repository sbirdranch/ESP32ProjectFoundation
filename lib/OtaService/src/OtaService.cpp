#include <Arduino.h>
#include <ArduinoOTA.h>

#include "AppConfig.h"
#include "OtaService.h"

void OtaService::beginIfReady(bool wifiConnected) {
    if (started_ || !wifiConnected) {
        return;
    }

    if (AppConfig::OtaPassword[0] == '\0') {
        if (!warnedMissingPassword_) {
            Serial.println("OTA disabled: configure OTA_PASSWORD in include/secrets.h");
            warnedMissingPassword_ = true;
        }
        return;
    }

    ArduinoOTA.setHostname(AppConfig::DeviceId);
    ArduinoOTA.setPassword(AppConfig::OtaPassword);
    ArduinoOTA.onStart([this]() {
        if (beforeUpdate_) beforeUpdate_();
        Serial.println("OTA update started");
    });
    ArduinoOTA.onEnd([]() {
        Serial.println("OTA update complete");
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("OTA error: %u\n", static_cast<unsigned int>(error));
    });
    ArduinoOTA.begin();
    started_ = true;
    Serial.println("OTA ready");
}

void OtaService::loop() {
    if (started_) {
        ArduinoOTA.handle();
    }
}