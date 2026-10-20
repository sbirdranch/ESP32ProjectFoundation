#include <Arduino.h>
#include <WiFi.h>

#include "AppConfig.h"
#include "WifiService.h"

void WifiService::begin() {
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    started_ = true;

    if (AppConfig::WifiSsid[0] == '\0') {
        Serial.println("Wi-Fi not started: configure include/secrets.h");
        warnedMissingSsid_ = true;
        return;
    }

    WiFi.begin(AppConfig::WifiSsid, AppConfig::WifiPassword);
    lastAttemptAt_ = millis();
}

void WifiService::loop() {
    if (!started_) {
        return;
    }

    if (AppConfig::WifiSsid[0] == '\0') {
        if (!warnedMissingSsid_) {
            Serial.println("Wi-Fi not started: configure include/secrets.h");
            warnedMissingSsid_ = true;
        }
        return;
    }

    const bool connected = isConnected();
    if (connected != wasConnected_) {
        Serial.println(connected ? "Wi-Fi connected" : "Wi-Fi disconnected");
        wasConnected_ = connected;
    }

    if (connected || millis() - lastAttemptAt_ < AppConfig::WifiRetryMs) {
        return;
    }

    WiFi.reconnect();
    lastAttemptAt_ = millis();
}

bool WifiService::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}