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

#ifdef APP_DEVICE_ID
#undef APP_DEVICE_ID
#endif
#define APP_DEVICE_ID "WindowOpener_Right"

namespace AppConfig {
constexpr char FirmwareVersion[] = "0.3.0";
constexpr char WifiSsid[] = WIFI_SSID;
constexpr char WifiPassword[] = WIFI_PASSWORD;
constexpr char OtaPassword[] = OTA_PASSWORD;
constexpr char MqttUsername[] = MQTT_USERNAME;
constexpr char MqttPassword[] = MQTT_PASSWORD;
constexpr char MqttHost[] = "192.168.0.179";
constexpr unsigned int MqttPort = 1883;
constexpr char DeviceId[] = APP_DEVICE_ID;
constexpr char DeviceName[] = "Right Window Opener";
constexpr int LedPin = 48;
constexpr int OpenRelayPin = 10;
constexpr int CloseRelayPin = 11;
constexpr uint32_t WindowRunMs = 30000;
constexpr uint32_t ReversalPauseMs = 500;
static_assert(OpenRelayPin != CloseRelayPin, "Relay pins must differ");
constexpr uint8_t LedBrightness = 15;
constexpr unsigned long WifiRetryMs = 10000;
constexpr unsigned long MqttRetryMs = 5000;
}
