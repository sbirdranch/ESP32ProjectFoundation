#include <Arduino.h>

#include "AppConfig.h"
#include "FanController.h"
#include "FanRelay.h"
#include "FanSetpointStore.h"
#include "LedSwitch.h"
#include "ManualButton.h"
#include "OtaService.h"
#include "ShedMqttService.h"
#include "Sht40Sensor.h"
#include "StartupIndicator.h"
#include "WifiService.h"

LedSwitch ledSwitch;
ShedVentilation::FanController fanController;
ShedVentilation::FanSetpointStore fanSetpointStore;
FanRelay fanRelay;
ManualButton manualButton;
Sht40Sensor sht40Sensor;
ShedMqttService shedMqttService(fanController, fanSetpointStore, sht40Sensor);
OtaService otaService;
StartupIndicator startupIndicator(ledSwitch);
WifiService wifiService;

void setup() {
    Serial.begin(115200);
    Serial.printf("Firmware version: %s\n", AppConfig::FirmwareVersion);
    Serial.println("Shed Ventilation: Wi-Fi, MQTT, OTA and temperature control enabled");
    Serial.println("LED driver: ESP32 Arduino core neopixelWrite; LED mirrors fan output");
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
    fanRelay.begin();
    ledSwitch.begin();
    startupIndicator.begin();
    manualButton.begin(millis());
    if (!fanSetpointStore.begin()) {
        Serial.println("Setpoint storage error: NVS unavailable or defaults could not be saved");
    }
    if (!sht40Sensor.begin()) {
        Serial.println("SHT40 unavailable; retaining fan output until readings recover");
    }
    wifiService.begin();
    shedMqttService.begin();
}

void loop() {
    wifiService.loop();
    otaService.beginIfReady(wifiService.isConnected());
    otaService.loop();

    const unsigned long now = millis();
    const bool sensorUpdated = sht40Sensor.update(now);
    if (manualButton.update(now)) {
        fanController.toggleManualState();
        shedMqttService.markStateDirty();
    }
    shedMqttService.loop(wifiService.isConnected());

    const bool previousFanState = fanRelay.isOn();
    const float indoorF = ShedVentilation::celsiusToFahrenheit(sht40Sensor.temperatureC());
    const bool outdoorFresh = shedMqttService.hasFreshOutdoorTemperature(now);
    const float outdoorF = ShedVentilation::celsiusToFahrenheit(
        shedMqttService.outdoorTemperatureC());
    fanController.update(indoorF, sht40Sensor.isReadingValid(), outdoorF,
                         outdoorFresh, fanSetpointStore.get());
    fanRelay.set(fanController.fanOn());
    ledSwitch.set(fanRelay.isOn());

    if (sensorUpdated || previousFanState != fanRelay.isOn()) {
        shedMqttService.markStateDirty();
    }
    shedMqttService.publishPendingState(now);
    startupIndicator.update(wifiService.isConnected(), shedMqttService.isConnected());
}
