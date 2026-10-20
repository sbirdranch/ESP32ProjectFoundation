#include <Arduino.h>

#include "LedSwitch.h"
#include "MqttSwitch.h"
#include "OtaService.h"
#include "WifiService.h"

LedSwitch ledSwitch;
MqttSwitch mqttSwitch(ledSwitch);
OtaService otaService;
WifiService wifiService;

void setup() {
    Serial.begin(115200);
    ledSwitch.begin();
    wifiService.begin();
    mqttSwitch.begin();
}

void loop() {
    wifiService.loop();
    otaService.beginIfReady(wifiService.isConnected());
    otaService.loop();
    mqttSwitch.loop(wifiService.isConnected());
}