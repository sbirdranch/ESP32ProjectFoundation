#pragma once

#include <stdint.h>

#if __has_include("secrets.h")
#include "secrets.h"
#endif

#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD ""
#endif

#ifndef OTA_PASSWORD
#define OTA_PASSWORD ""
#endif

#ifndef MQTT_USERNAME
#define MQTT_USERNAME ""
#endif

#ifndef MQTT_PASSWORD
#define MQTT_PASSWORD ""
#endif

#ifndef APP_DEVICE_ID
#define APP_DEVICE_ID "ShedVentilation_esp32s3"
#endif

namespace AppConfig {
constexpr char FirmwareVersion[] = "0.1.0";
constexpr char WifiSsid[] = WIFI_SSID;
constexpr char WifiPassword[] = WIFI_PASSWORD;
constexpr char OtaPassword[] = OTA_PASSWORD;
constexpr char MqttUsername[] = MQTT_USERNAME;
constexpr char MqttPassword[] = MQTT_PASSWORD;
constexpr char MqttHost[] = "192.168.0.179";
constexpr unsigned int MqttPort = 1883;
constexpr char DeviceId[] = APP_DEVICE_ID;
constexpr char DeviceName[] = "Shed Ventilation";
constexpr char WeatherTemperatureTopic[] = "weather/outdoor_temperature_c";
constexpr char WeatherEventsTopic[] = "weather/events";
constexpr int LedPin = 48;
constexpr uint8_t LedBrightness = 15;
constexpr int I2cSdaPin = 8;
constexpr int I2cSclPin = 9;
constexpr uint8_t Sht40Address = 0x44;
constexpr int FanRelayPin = 10;
constexpr bool FanRelayActiveHigh = true;
constexpr int ManualButtonPin = 11;
constexpr bool ManualButtonActiveHigh = false;
constexpr int16_t DefaultOffSetpointF = 80;
constexpr int16_t DefaultOnSetpointF = 110;
constexpr int16_t MinimumSetpointF = 60;
constexpr int16_t MaximumSetpointF = 120;
constexpr int16_t MinimumSetpointGapF = 2;
constexpr float OutdoorCoolingMarginF = 2.0F;
constexpr int16_t FanRestartHysteresisF = 2;
constexpr unsigned long SensorPollMs = 2000;
constexpr unsigned long ButtonDebounceMs = 40;
constexpr unsigned long OutdoorStaleMs = 600000;
constexpr unsigned long MqttStatePublishMs = 30000;
constexpr unsigned long WifiRetryMs = 10000;
constexpr unsigned long MqttRetryMs = 5000;
}
