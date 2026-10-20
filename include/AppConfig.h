#pragma once

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

#ifndef APP_LED_PIN
#define APP_LED_PIN 2
#endif

#ifndef APP_DEVICE_ID
#define APP_DEVICE_ID "esp32-foundation-01"
#endif

#ifndef APP_LED_ACTIVE_LOW
#define APP_LED_ACTIVE_LOW 0
#endif

namespace AppConfig {
constexpr char WifiSsid[] = WIFI_SSID;
constexpr char WifiPassword[] = WIFI_PASSWORD;
constexpr char OtaPassword[] = OTA_PASSWORD;
constexpr char MqttUsername[] = MQTT_USERNAME;
constexpr char MqttPassword[] = MQTT_PASSWORD;
constexpr char MqttHost[] = "192.168.0.179";
constexpr unsigned int MqttPort = 1883;
constexpr char DeviceId[] = APP_DEVICE_ID;
constexpr char DeviceName[] = "ESP32 Foundation LED";
constexpr int LedPin = APP_LED_PIN;
constexpr bool LedActiveLow = APP_LED_ACTIVE_LOW != 0;
constexpr unsigned long WifiRetryMs = 10000;
constexpr unsigned long MqttRetryMs = 5000;
}