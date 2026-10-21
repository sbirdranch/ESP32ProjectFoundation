#include <Arduino.h>

#include "AppConfig.h"
#include "LedSwitch.h"
#include "MqttSwitch.h"
#include "OtaService.h"
#include "StartupIndicator.h"
#include "WifiService.h"

LedSwitch ledSwitch;
MqttSwitch mqttSwitch(ledSwitch);
OtaService otaService;
StartupIndicator startupIndicator(ledSwitch);
WifiService wifiService;

void setup() {
    Serial.begin(115200);
    Serial.printf("Firmware version: %s\n", AppConfig::FirmwareVersion);
    Serial.println("ESP32 Foundation: Wi-Fi, MQTT, OTA and startup indicator enabled");
    Serial.println("LED driver: ESP32 Arduino core neopixelWrite");
    Serial.printf("LED configuration: WS2812, GPIO%d, GRB, brightness=%u\n",
                  AppConfig::LedPin,
                  static_cast<unsigned int>(AppConfig::LedBrightness));
    Serial.printf(
        "ESP32-S3 memory: flash=%lu bytes, PSRAM=%lu bytes\n",
        static_cast<unsigned long>(ESP.getFlashChipSize()),
        static_cast<unsigned long>(ESP.getPsramSize()));
    if (!psramFound()) {
        Serial.println("Hardware error: PSRAM was not detected");
    }
    ledSwitch.begin();
    startupIndicator.begin();
    wifiService.begin();
    mqttSwitch.begin();
}

void loop() {
    wifiService.loop();
    otaService.beginIfReady(wifiService.isConnected());
    otaService.loop();
    mqttSwitch.loop(wifiService.isConnected());
    startupIndicator.update(wifiService.isConnected(), mqttSwitch.isConnected());
}
