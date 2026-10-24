#include <Arduino.h>

#include "AppConfig.h"
#include "LedSwitch.h"
#include "WindowMqtt.h"
#include "WindowOutputs.h"
#include "OtaService.h"
#include "StartupIndicator.h"
#include "WifiService.h"

LedSwitch ledSwitch;
Window::Controller windowController(AppConfig::WindowRunMs, AppConfig::ReversalPauseMs);
WindowOutputs outputs(AppConfig::OpenRelayPin, AppConfig::CloseRelayPin);
WindowMqtt windowMqtt(windowController);
OtaService otaService;
StartupIndicator startupIndicator(ledSwitch);
WifiService wifiService;

void setup() {
    Serial.begin(115200);
    Serial.printf("Firmware version: %s\n", AppConfig::FirmwareVersion);
    Serial.println("Left Window Opener: timed actuator control, MQTT, Wi-Fi and OTA enabled");
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
    outputs.begin();
    ledSwitch.begin();
    startupIndicator.begin();
    wifiService.begin();
    windowMqtt.begin();
    otaService.setBeforeUpdate([]() {
        windowController.stop(millis());
        outputs.apply(windowController);
        ledSwitch.setColor(AppConfig::LedBrightness, 0, 0);
        if (windowMqtt.isConnected()) windowMqtt.publishState();
    });
}

void loop() {
    windowController.update(millis());
    outputs.apply(windowController);
    wifiService.loop();
    otaService.beginIfReady(wifiService.isConnected());
    otaService.loop();
    windowMqtt.loop(wifiService.isConnected());
    windowController.update(millis());
    outputs.apply(windowController);
    const auto endpoint = windowController.endpoint();
    const auto brightness = AppConfig::LedBrightness;
    ledSwitch.setColor(endpoint == Window::State::Unknown ? brightness : 0,
                      endpoint == Window::State::Closed ? brightness : 0,
                      endpoint == Window::State::Open ? brightness : 0);
    windowMqtt.publishPendingState();
    startupIndicator.update(wifiService.isConnected(), windowMqtt.isConnected());
}
