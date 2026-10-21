#include <Arduino.h>
#include <ArduinoJson.h>
#include <string.h>

#include "AppConfig.h"
#include "LedSwitch.h"
#include "MqttSwitch.h"

namespace {
constexpr char kCommandTopic[] = "devices/" APP_DEVICE_ID "/led/set";
constexpr char kStateTopic[] = "devices/" APP_DEVICE_ID "/led/state";
constexpr char kAvailabilityTopic[] = "devices/" APP_DEVICE_ID "/led/availability";
constexpr char kDiscoveryTopic[] = "homeassistant/switch/" APP_DEVICE_ID "/led/config";
constexpr char kPayloadOn[] = "ON";
constexpr char kPayloadOff[] = "OFF";
constexpr char kPayloadOnline[] = "online";
constexpr char kPayloadOffline[] = "offline";
}

MqttSwitch* MqttSwitch::activeInstance_ = nullptr;

MqttSwitch::MqttSwitch(LedSwitch& ledSwitch)
    : mqttClient_(networkClient_), ledSwitch_(ledSwitch) {}

void MqttSwitch::begin() {
    activeInstance_ = this;
    mqttClient_.setServer(AppConfig::MqttHost, AppConfig::MqttPort);
    mqttClient_.setCallback(messageCallback);
    if (!mqttClient_.setBufferSize(1024)) {
        Serial.println("MQTT error: unable to set packet buffer size");
    }
    mqttClient_.setKeepAlive(30);
    mqttClient_.setSocketTimeout(2);
    Serial.printf("MQTT configured for %s:%u\n", AppConfig::MqttHost, AppConfig::MqttPort);
}

void MqttSwitch::loop(bool wifiConnected) {
    if (!wifiConnected) {
        if (wasConnected_) {
            Serial.println("MQTT disconnected because Wi-Fi is unavailable");
            wasConnected_ = false;
        }
        return;
    }

    if (!mqttClient_.connected()) {
        if (wasConnected_) {
            Serial.printf("MQTT disconnected; state=%d\n", mqttClient_.state());
            wasConnected_ = false;
        }
        if (millis() - lastAttemptAt_ >= AppConfig::MqttRetryMs) {
            Serial.printf("MQTT connecting to %s:%u\n", AppConfig::MqttHost, AppConfig::MqttPort);
            connect();
            lastAttemptAt_ = millis();
        }
        return;
    }

    mqttClient_.loop();
}

bool MqttSwitch::isConnected() {
    return mqttClient_.connected();
}

void MqttSwitch::connect() {
    bool connected = false;
    if (AppConfig::MqttUsername[0] != '\0') {
        connected = mqttClient_.connect(
            AppConfig::DeviceId,
            AppConfig::MqttUsername,
            AppConfig::MqttPassword,
            kAvailabilityTopic,
            0,
            true,
            kPayloadOffline);
    } else {
        connected = mqttClient_.connect(
            AppConfig::DeviceId,
            kAvailabilityTopic,
            0,
            true,
            kPayloadOffline);
    }

    if (!connected) {
        Serial.printf("MQTT connection failed, state %d\n", mqttClient_.state());
        return;
    }

    Serial.println("MQTT connected");
    wasConnected_ = true;
    if (!mqttClient_.publish(kAvailabilityTopic, kPayloadOnline, true)) {
        Serial.println("MQTT error: failed to publish online availability");
    }
    if (!mqttClient_.subscribe(kCommandTopic)) {
        Serial.println("MQTT error: failed to subscribe to LED command topic");
    }
    publishDiscovery();
    publishState();
}

void MqttSwitch::handleMessage(const char* topic, const uint8_t* payload, unsigned int length) {
    Serial.printf("MQTT message topic=%s payload=", topic);
    Serial.write(payload, length);
    Serial.println();

    if (strcmp(topic, kCommandTopic) != 0) {
        Serial.println("MQTT error: unrecognized message topic");
        return;
    }

    if (length == 2 && memcmp(payload, kPayloadOn, 2) == 0) {
        ledSwitch_.set(true);
    } else if (length == 3 && memcmp(payload, kPayloadOff, 3) == 0) {
        ledSwitch_.set(false);
    } else {
        Serial.println("MQTT error: unrecognized command; expected exactly ON or OFF");
        return;
    }

    publishState();
}

void MqttSwitch::publishDiscovery() {
    JsonDocument document;
    document["name"] = AppConfig::DeviceName;
    document["unique_id"] = String(AppConfig::DeviceId) + "_led";
    document["command_topic"] = kCommandTopic;
    document["state_topic"] = kStateTopic;
    document["availability_topic"] = kAvailabilityTopic;
    document["payload_on"] = kPayloadOn;
    document["payload_off"] = kPayloadOff;
    document["state_on"] = kPayloadOn;
    document["state_off"] = kPayloadOff;
    document["payload_available"] = kPayloadOnline;
    document["payload_not_available"] = kPayloadOffline;
    document["icon"] = "mdi:led-on";

    JsonObject device = document["device"].to<JsonObject>();
    JsonArray identifiers = device["identifiers"].to<JsonArray>();
    identifiers.add(AppConfig::DeviceId);
    device["name"] = AppConfig::DeviceName;
    device["manufacturer"] = "Lonely Binary";
    device["model"] = "ESP32-S3 N16R8";

    char payload[1024];
    const size_t payloadLength = serializeJson(document, payload, sizeof(payload));
    if (payloadLength == 0 || payloadLength >= sizeof(payload)) {
        Serial.println("MQTT error: unable to serialize Home Assistant discovery");
        return;
    }

    if (!mqttClient_.publish(kDiscoveryTopic, payload, true)) {
        Serial.println("MQTT error: failed to publish Home Assistant discovery");
    }
}

void MqttSwitch::publishState() {
    if (!mqttClient_.publish(kStateTopic, ledSwitch_.isOn() ? kPayloadOn : kPayloadOff, true)) {
        Serial.println("MQTT error: failed to publish LED state");
    }
}

void MqttSwitch::messageCallback(char* topic, uint8_t* payload, unsigned int length) {
    if (activeInstance_ != nullptr) {
        activeInstance_->handleMessage(topic, payload, length);
    }
}