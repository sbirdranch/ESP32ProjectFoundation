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

    Serial.printf("Wi-Fi connecting to SSID '%s'\n", AppConfig::WifiSsid);
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
        if (connected) {
            const String ipAddress = WiFi.localIP().toString();
            Serial.printf("Wi-Fi connected; IP=%s\n", ipAddress.c_str());
        } else {
            Serial.printf("Wi-Fi disconnected; status=%d\n", static_cast<int>(WiFi.status()));
        }
        wasConnected_ = connected;
    }

    if (connected || millis() - lastAttemptAt_ < AppConfig::WifiRetryMs) {
        return;
    }

    Serial.printf("Wi-Fi reconnect attempt; status=%d\n", static_cast<int>(WiFi.status()));
    WiFi.reconnect();
    lastAttemptAt_ = millis();
}

bool WifiService::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}