#include <Arduino.h>
#include <ArduinoJson.h>
#include <string.h>

#include "AppConfig.h"
#include "WindowController.h"
#include "WindowMqtt.h"

namespace {
constexpr char kCommandTopic[] = "devices/" APP_DEVICE_ID "/window/set";
constexpr char kStateTopic[] = "devices/" APP_DEVICE_ID "/window/state";
constexpr char kAvailabilityTopic[] = "devices/" APP_DEVICE_ID "/availability";
constexpr char kDiscoveryTopic[] = "homeassistant/cover/" APP_DEVICE_ID "/window/config";
constexpr char kWeatherTopic[] = "weather/events";
struct PayloadReader {
    const uint8_t* data;
    size_t length, position = 0;
    PayloadReader(const uint8_t* bytes, size_t size) : data(bytes), length(size) {}
    int read() { return position < length ? data[position++] : -1; }
    size_t readBytes(char* buffer, size_t count) {
        size_t copied = 0;
        while (copied < count && position < length) buffer[copied++] = data[position++];
        return copied;
    }
};
constexpr char kPayloadOnline[] = "online";
constexpr char kPayloadOffline[] = "offline";
}

WindowMqtt* WindowMqtt::activeInstance_ = nullptr;

WindowMqtt::WindowMqtt(Window::Controller& controller)
    : mqttClient_(networkClient_), controller_(controller) {}

void WindowMqtt::begin() {
    activeInstance_ = this;
    mqttClient_.setServer(AppConfig::MqttHost, AppConfig::MqttPort);
    mqttClient_.setCallback(messageCallback);
    if (!mqttClient_.setBufferSize(2048)) {
        Serial.println("MQTT error: unable to set packet buffer size");
    }
    mqttClient_.setKeepAlive(30);
    mqttClient_.setSocketTimeout(2);
    Serial.printf("MQTT configured for %s:%u\n", AppConfig::MqttHost, AppConfig::MqttPort);
}

void WindowMqtt::loop(bool wifiConnected) {
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

bool WindowMqtt::isConnected() {
    return mqttClient_.connected();
}

void WindowMqtt::connect() {
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
        Serial.println("MQTT error: failed to subscribe to window command topic");
    }
    mqttClient_.subscribe(kWeatherTopic);
    publishDiscovery();
    publishState();
}

void WindowMqtt::handleMessage(const char* topic, const uint8_t* payload, unsigned int length) {
    if (strcmp(topic, kCommandTopic) == 0) {
        if (!controller_.command(payload, length, millis())) return;
    } else if (strcmp(topic, kWeatherTopic) == 0) {
        if (length == 0 || length > 256) return;
        JsonDocument event;
        PayloadReader reader{payload, length};
        if (deserializeJson(event, reader) || !event.is<JsonObject>() ||
            !event["event"].is<const char*>()) return;
        const JsonString name = event["event"].as<JsonString>();
        if (name.size() != 12 || memcmp(name.c_str(), "rain_started", 12) != 0) return;
        while (reader.position < length) {
            const char c = payload[reader.position++];
            if (c != ' ' && c != '\t' && c != '\r' && c != '\n') return;
        }
        controller_.move(Window::Direction::Close, millis());
    } else return;
    dirty_ = true;
}

void WindowMqtt::publishDiscovery() {
    JsonDocument document;
    document["name"] = AppConfig::DeviceName;
    document["unique_id"] = String(AppConfig::DeviceId) + "_window";
    document["command_topic"] = kCommandTopic;
    document["state_topic"] = kStateTopic;
    document["availability_topic"] = kAvailabilityTopic;
    document["payload_open"] = "OPEN";
    document["payload_close"] = "CLOSE";
    document["payload_stop"] = "STOP";
    document["state_open"] = "Open";
    document["state_closed"] = "Closed";
    document["state_opening"] = "Opening";
    document["state_closing"] = "Closing";
    // MQTT cover resets to unknown on the special None payload after templating.
    document["value_template"] = "{{ 'None' if value == 'Unknown' else value }}";
    document["device_class"] = "window";
    document["optimistic"] = false;
    document["retain"] = false;
    document["payload_available"] = kPayloadOnline;
    document["payload_not_available"] = kPayloadOffline;

    JsonObject device = document["device"].to<JsonObject>();
    JsonArray identifiers = device["identifiers"].to<JsonArray>();
    identifiers.add(AppConfig::DeviceId);
    device["name"] = AppConfig::DeviceName;
    device["manufacturer"] = "Lonely Binary";
    device["model"] = "ESP32-S3 N16R8";
    device["sw_version"] = AppConfig::FirmwareVersion;

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

void WindowMqtt::publishState() {
    dirty_ = !mqttClient_.publish(kStateTopic, controller_.stateText(), true);
    if (!dirty_) publishedState_ = controller_.state();
}

void WindowMqtt::publishPendingState() {
    if (mqttClient_.connected() && (dirty_ || publishedState_ != controller_.state())) publishState();
}

void WindowMqtt::messageCallback(char* topic, uint8_t* payload, unsigned int length) {
    if (activeInstance_ != nullptr) {
        activeInstance_->handleMessage(topic, payload, length);
    }
}
