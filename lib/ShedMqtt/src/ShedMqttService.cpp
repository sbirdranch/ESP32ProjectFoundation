#include "ShedMqttService.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "AppConfig.h"

namespace {
constexpr char kFanCommandTopic[] = "devices/" APP_DEVICE_ID "/fan/set";
constexpr char kAutomaticCommandTopic[] = "devices/" APP_DEVICE_ID "/control_mode/set";
constexpr char kOffSetpointCommandTopic[] = "devices/" APP_DEVICE_ID "/setpoint/off/set";
constexpr char kOnSetpointCommandTopic[] = "devices/" APP_DEVICE_ID "/setpoint/on/set";
constexpr char kStateTopic[] = "devices/" APP_DEVICE_ID "/state";
constexpr char kAvailabilityTopic[] = "devices/" APP_DEVICE_ID "/availability";
constexpr char kPayloadOn[] = "ON";
constexpr char kPayloadOff[] = "OFF";
constexpr char kPayloadOnline[] = "online";
constexpr char kPayloadOffline[] = "offline";
constexpr char kDiscoveryPayloadAvailable[] = "online";
constexpr char kDiscoveryPayloadNotAvailable[] = "offline";
constexpr char kFanObjectId[] = "fan";
constexpr char kOffSetpointObjectId[] = "off_setpoint";
constexpr char kOnSetpointObjectId[] = "on_setpoint";
constexpr char kIndoorTemperatureObjectId[] = "indoor_temperature";
constexpr char kIndoorHumidityObjectId[] = "indoor_humidity";
constexpr char kOutdoorTemperatureObjectId[] = "outdoor_temperature";
constexpr char kIndoorFaultObjectId[] = "indoor_sensor_fault";
constexpr char kOutdoorStaleObjectId[] = "outdoor_temperature_stale";
constexpr char kControlModeObjectId[] = "control_mode";
constexpr char kSetpointStorageFaultObjectId[] = "setpoint_storage_fault";

bool copyPayload(const uint8_t* payload, unsigned int length, char* output,
                 size_t outputSize) {
    if (length == 0 || length >= outputSize) {
        return false;
    }
    memcpy(output, payload, length);
    output[length] = '\0';
    return true;
}

bool parseSetpoint(const uint8_t* payload, unsigned int length, int16_t& value) {
    char text[16];
    if (!copyPayload(payload, length, text, sizeof(text))) {
        return false;
    }
    char* end = nullptr;
    const long parsed = strtol(text, &end, 10);
    if (end == text || *end != '\0' || parsed < INT16_MIN || parsed > INT16_MAX) {
        return false;
    }
    value = static_cast<int16_t>(parsed);
    return true;
}

bool parseTemperatureC(const uint8_t* payload, unsigned int length, float& value) {
    char text[24];
    if (!copyPayload(payload, length, text, sizeof(text))) {
        return false;
    }
    char* end = nullptr;
    const float parsed = strtof(text, &end);
    if (end == text || *end != '\0' || !isfinite(parsed) || parsed < -80.0F ||
        parsed > 80.0F) {
        return false;
    }
    value = parsed;
    return true;
}
}

ShedMqttService* ShedMqttService::activeInstance_ = nullptr;

ShedMqttService::ShedMqttService(ShedVentilation::FanController& fanController,
                                 ShedVentilation::FanSetpointStore& setpointStore,
                                 Sht40Sensor& sensor)
    : mqttClient_(networkClient_),
      fanController_(fanController),
      setpointStore_(setpointStore),
      sensor_(sensor) {}

void ShedMqttService::begin() {
    activeInstance_ = this;
    mqttClient_.setServer(AppConfig::MqttHost, AppConfig::MqttPort);
    mqttClient_.setCallback(messageCallback);
    if (!mqttClient_.setBufferSize(2048)) {
        Serial.println("MQTT error: unable to set packet buffer size");
    }
    mqttClient_.setKeepAlive(30);
    mqttClient_.setSocketTimeout(2);
    Serial.printf("MQTT configured for %s:%u\n", AppConfig::MqttHost,
                  AppConfig::MqttPort);
}

void ShedMqttService::loop(bool wifiConnected) {
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
        const unsigned long now = millis();
        if (now - lastAttemptAt_ >= AppConfig::MqttRetryMs) {
            connect();
            lastAttemptAt_ = now;
        }
        return;
    }

    mqttClient_.loop();
}

void ShedMqttService::publishPendingState(unsigned long now) {
    if (mqttClient_.connected()) {
        publishStateIfNeeded(now);
    }
}

void ShedMqttService::markStateDirty() {
    stateDirty_ = true;
}

bool ShedMqttService::isConnected() {
    return mqttClient_.connected();
}

bool ShedMqttService::hasFreshOutdoorTemperature(unsigned long now) const {
    return hasOutdoorTemperature_ && now - outdoorReceivedAt_ <= AppConfig::OutdoorStaleMs;
}

float ShedMqttService::outdoorTemperatureC() const {
    return outdoorTemperatureC_;
}

void ShedMqttService::connect() {
    bool connected = false;
    if (AppConfig::MqttUsername[0] != '\0') {
        connected = mqttClient_.connect(
            AppConfig::DeviceId, AppConfig::MqttUsername, AppConfig::MqttPassword,
            kAvailabilityTopic, 0, true, kPayloadOffline);
    } else {
        connected = mqttClient_.connect(AppConfig::DeviceId, kAvailabilityTopic, 0,
                                        true, kPayloadOffline);
    }

    if (!connected) {
        Serial.printf("MQTT connection failed, state %d\n", mqttClient_.state());
        return;
    }

    Serial.println("MQTT connected");
    wasConnected_ = true;
    mqttClient_.publish(kAvailabilityTopic, kPayloadOnline, true);
    mqttClient_.subscribe(kFanCommandTopic);
    mqttClient_.subscribe(kAutomaticCommandTopic);
    mqttClient_.subscribe(kOffSetpointCommandTopic);
    mqttClient_.subscribe(kOnSetpointCommandTopic);
    mqttClient_.subscribe(AppConfig::WeatherTemperatureTopic);
    publishDiscovery();
    stateDirty_ = true;
}

void ShedMqttService::handleMessage(const char* topic, const uint8_t* payload,
                                   unsigned int length) {
    if (strcmp(topic, kAutomaticCommandTopic) == 0) {
        if (length == 4 && memcmp(payload, "AUTO", 4) == 0) {
            fanController_.resumeAutomatic();
        } else {
            Serial.println("MQTT error: control mode command must be exactly AUTO");
        }
        stateDirty_ = true;
        return;
    }
    if (strcmp(topic, kFanCommandTopic) == 0) {
        if (length == 2 && memcmp(payload, kPayloadOn, 2) == 0) {
            fanController_.requestManualState(true);
        } else if (length == 3 && memcmp(payload, kPayloadOff, 3) == 0) {
            fanController_.requestManualState(false);
        } else {
            Serial.println("MQTT error: fan command must be exactly ON or OFF");
        }
        stateDirty_ = true;
        return;
    }

    if (strcmp(topic, kOffSetpointCommandTopic) == 0 ||
        strcmp(topic, kOnSetpointCommandTopic) == 0) {
        int16_t temperatureF = 0;
        bool accepted = false;
        if (parseSetpoint(payload, length, temperatureF)) {
            accepted = strcmp(topic, kOffSetpointCommandTopic) == 0
                           ? setpointStore_.setOffSetpoint(temperatureF)
                           : setpointStore_.setOnSetpoint(temperatureF);
        }
        if (!accepted) {
            Serial.println("MQTT error: rejected invalid or unpersisted setpoint");
        }
        stateDirty_ = true;
        return;
    }

    if (strcmp(topic, AppConfig::WeatherTemperatureTopic) == 0) {
        float temperatureC = 0.0F;
        if (parseTemperatureC(payload, length, temperatureC)) {
            outdoorTemperatureC_ = temperatureC;
            outdoorReceivedAt_ = millis();
            hasOutdoorTemperature_ = true;
        } else {
            hasOutdoorTemperature_ = false;
            Serial.println("MQTT error: rejected invalid outdoor temperature");
        }
        stateDirty_ = true;
    }
}

void ShedMqttService::publishDiscovery() {
    JsonDocument document;
    document["name"] = "Ventilation Fan";
    document["unique_id"] = String(AppConfig::DeviceId) + "_fan";
    document["command_topic"] = kFanCommandTopic;
    document["state_topic"] = kStateTopic;
    document["state_value_template"] = "{{ 'ON' if value_json.fan_on else 'OFF' }}";
    document["payload_on"] = kPayloadOn;
    document["payload_off"] = kPayloadOff;
    document["retain"] = false;
    document["availability_topic"] = kAvailabilityTopic;
    document["payload_available"] = kDiscoveryPayloadAvailable;
    document["payload_not_available"] = kDiscoveryPayloadNotAvailable;
    document["icon"] = "mdi:fan";
    addDeviceMetadata(document);
    publishDiscoveryConfig("fan", kFanObjectId, document);

    document.clear();
    document["name"] = "Resume automatic control";
    document["unique_id"] = String(AppConfig::DeviceId) + "_resume_automatic";
    document["command_topic"] = kAutomaticCommandTopic;
    document["payload_press"] = "AUTO";
    document["retain"] = false;
    document["icon"] = "mdi:autorenew";
    document["availability_topic"] = kAvailabilityTopic;
    document["payload_available"] = kDiscoveryPayloadAvailable;
    document["payload_not_available"] = kDiscoveryPayloadNotAvailable;
    addDeviceMetadata(document);
    publishDiscoveryConfig("button", "resume_automatic", document);

    document.clear();
    document["name"] = "Fan OFF temperature";
    document["unique_id"] = String(AppConfig::DeviceId) + "_off_setpoint";
    document["command_topic"] = kOffSetpointCommandTopic;
    document["state_topic"] = kStateTopic;
    document["value_template"] = "{{ value_json.off_setpoint_f }}";
    document["min"] = AppConfig::MinimumSetpointF;
    document["max"] = AppConfig::MaximumSetpointF;
    document["step"] = 1;
    document["mode"] = "box";
    document["unit_of_measurement"] = "°F";
    document["icon"] = "mdi:thermometer-chevron-down";
    document["availability_topic"] = kAvailabilityTopic;
    document["payload_available"] = kDiscoveryPayloadAvailable;
    document["payload_not_available"] = kDiscoveryPayloadNotAvailable;
    addDeviceMetadata(document);
    publishDiscoveryConfig("number", kOffSetpointObjectId, document);

    document.clear();
    document["name"] = "Fan ON temperature";
    document["unique_id"] = String(AppConfig::DeviceId) + "_on_setpoint";
    document["command_topic"] = kOnSetpointCommandTopic;
    document["state_topic"] = kStateTopic;
    document["value_template"] = "{{ value_json.on_setpoint_f }}";
    document["min"] = AppConfig::MinimumSetpointF;
    document["max"] = AppConfig::MaximumSetpointF;
    document["step"] = 1;
    document["mode"] = "box";
    document["unit_of_measurement"] = "°F";
    document["icon"] = "mdi:thermometer-chevron-up";
    document["availability_topic"] = kAvailabilityTopic;
    document["payload_available"] = kDiscoveryPayloadAvailable;
    document["payload_not_available"] = kDiscoveryPayloadNotAvailable;
    addDeviceMetadata(document);
    publishDiscoveryConfig("number", kOnSetpointObjectId, document);

    document.clear();
    document["name"] = "Shed temperature";
    document["unique_id"] = String(AppConfig::DeviceId) + "_indoor_temperature";
    document["state_topic"] = kStateTopic;
    document["value_template"] = "{{ value_json.indoor_temperature_c if value_json.indoor_valid else none }}";
    document["unit_of_measurement"] = "°C";
    document["device_class"] = "temperature";
    document["state_class"] = "measurement";
    document["availability_topic"] = kAvailabilityTopic;
    document["payload_available"] = kDiscoveryPayloadAvailable;
    document["payload_not_available"] = kDiscoveryPayloadNotAvailable;
    addDeviceMetadata(document);
    publishDiscoveryConfig("sensor", kIndoorTemperatureObjectId, document);

    document.clear();
    document["name"] = "Shed humidity";
    document["unique_id"] = String(AppConfig::DeviceId) + "_indoor_humidity";
    document["state_topic"] = kStateTopic;
    document["value_template"] = "{{ value_json.indoor_humidity_percent if value_json.indoor_valid else none }}";
    document["unit_of_measurement"] = "%";
    document["device_class"] = "humidity";
    document["state_class"] = "measurement";
    document["suggested_display_precision"] = 0;
    document["availability_topic"] = kAvailabilityTopic;
    document["payload_available"] = kDiscoveryPayloadAvailable;
    document["payload_not_available"] = kDiscoveryPayloadNotAvailable;
    addDeviceMetadata(document);
    publishDiscoveryConfig("sensor", kIndoorHumidityObjectId, document);

    document.clear();
    document["name"] = "Outdoor temperature";
    document["unique_id"] = String(AppConfig::DeviceId) + "_outdoor_temperature";
    document["state_topic"] = kStateTopic;
    document["value_template"] = "{{ value_json.outdoor_temperature_c if value_json.outdoor_fresh else none }}";
    document["unit_of_measurement"] = "°C";
    document["device_class"] = "temperature";
    document["state_class"] = "measurement";
    document["availability_topic"] = kAvailabilityTopic;
    document["payload_available"] = kDiscoveryPayloadAvailable;
    document["payload_not_available"] = kDiscoveryPayloadNotAvailable;
    addDeviceMetadata(document);
    publishDiscoveryConfig("sensor", kOutdoorTemperatureObjectId, document);

    document.clear();
    document["name"] = "Indoor sensor fault";
    document["unique_id"] = String(AppConfig::DeviceId) + "_indoor_sensor_fault";
    document["state_topic"] = kStateTopic;
    document["value_template"] = "{{ 'OFF' if value_json.indoor_valid else 'ON' }}";
    document["payload_on"] = kPayloadOn;
    document["payload_off"] = kPayloadOff;
    document["device_class"] = "problem";
    document["availability_topic"] = kAvailabilityTopic;
    document["payload_available"] = kDiscoveryPayloadAvailable;
    document["payload_not_available"] = kDiscoveryPayloadNotAvailable;
    addDeviceMetadata(document);
    publishDiscoveryConfig("binary_sensor", kIndoorFaultObjectId, document);

    document.clear();
    document["name"] = "Outdoor temperature stale";
    document["unique_id"] = String(AppConfig::DeviceId) + "_outdoor_temperature_stale";
    document["state_topic"] = kStateTopic;
    document["value_template"] = "{{ 'OFF' if value_json.outdoor_fresh else 'ON' }}";
    document["payload_on"] = kPayloadOn;
    document["payload_off"] = kPayloadOff;
    document["device_class"] = "problem";
    document["availability_topic"] = kAvailabilityTopic;
    document["payload_available"] = kDiscoveryPayloadAvailable;
    document["payload_not_available"] = kDiscoveryPayloadNotAvailable;
    addDeviceMetadata(document);
    publishDiscoveryConfig("binary_sensor", kOutdoorStaleObjectId, document);

    document.clear();
    document["name"] = "Fan control mode";
    document["unique_id"] = String(AppConfig::DeviceId) + "_control_mode";
    document["state_topic"] = kStateTopic;
    document["value_template"] = "{{ 'Manual' if value_json.manual_override else 'Automatic' }}";
    document["availability_topic"] = kAvailabilityTopic;
    document["payload_available"] = kDiscoveryPayloadAvailable;
    document["payload_not_available"] = kDiscoveryPayloadNotAvailable;
    addDeviceMetadata(document);
    publishDiscoveryConfig("sensor", kControlModeObjectId, document);

    document.clear();
    document["name"] = "Setpoint storage fault";
    document["unique_id"] = String(AppConfig::DeviceId) + "_setpoint_storage_fault";
    document["state_topic"] = kStateTopic;
    document["value_template"] = "{{ 'ON' if value_json.setpoint_storage_fault else 'OFF' }}";
    document["payload_on"] = kPayloadOn;
    document["payload_off"] = kPayloadOff;
    document["device_class"] = "problem";
    document["availability_topic"] = kAvailabilityTopic;
    document["payload_available"] = kDiscoveryPayloadAvailable;
    document["payload_not_available"] = kDiscoveryPayloadNotAvailable;
    addDeviceMetadata(document);
    publishDiscoveryConfig("binary_sensor", kSetpointStorageFaultObjectId, document);
}

bool ShedMqttService::publishState(unsigned long now) {
    JsonDocument document;
    document["fan_on"] = fanController_.fanOn();
    document["manual_override"] = fanController_.isManualOverrideActive();
    document["indoor_valid"] = sensor_.isReadingValid();
    document["indoor_age_ms"] = sensor_.readingAgeMs(now);
    if (sensor_.hasValidReading()) {
        document["indoor_temperature_c"] = sensor_.temperatureC();
        document["indoor_humidity_percent"] = static_cast<int>(lroundf(sensor_.humidityPercent()));
    } else {
        document["indoor_temperature_c"] = nullptr;
        document["indoor_humidity_percent"] = nullptr;
    }
    const bool outdoorFresh = hasFreshOutdoorTemperature(now);
    document["outdoor_fresh"] = outdoorFresh;
    document["outdoor_age_ms"] = hasOutdoorTemperature_
                                     ? now - outdoorReceivedAt_
                                     : 0;
    if (hasOutdoorTemperature_) {
        document["outdoor_temperature_c"] = outdoorTemperatureC_;
    } else {
        document["outdoor_temperature_c"] = nullptr;
    }
    document["off_setpoint_f"] = setpointStore_.get().offF;
    document["on_setpoint_f"] = setpointStore_.get().onF;
    document["setpoint_storage_available"] = setpointStore_.isAvailable();
    document["setpoint_storage_fault"] = setpointStore_.hasStorageError();

    char payload[768];
    const size_t payloadLength = serializeJson(document, payload, sizeof(payload));
    if (payloadLength == 0 || payloadLength >= sizeof(payload) ||
        !mqttClient_.publish(kStateTopic, payload, true)) {
        Serial.println("MQTT error: failed to publish retained shed state");
        return false;
    }

    stateDirty_ = false;
    lastStateAt_ = now;
    return true;
}

void ShedMqttService::publishStateIfNeeded(unsigned long now) {
    if (stateDirty_ || now - lastStateAt_ >= AppConfig::MqttStatePublishMs) {
        publishState(now);
    }
}

bool ShedMqttService::publishDiscoveryConfig(const char* component,
                                            const char* objectId,
                                            JsonDocument& document) {
    char topic[160];
    snprintf(topic, sizeof(topic), "homeassistant/%s/%s/%s/config", component,
             AppConfig::DeviceId, objectId);
    char payload[1024];
    const size_t payloadLength = serializeJson(document, payload, sizeof(payload));
    if (payloadLength == 0 || payloadLength >= sizeof(payload) ||
        !mqttClient_.publish(topic, payload, true)) {
        Serial.printf("MQTT error: failed HA discovery for %s\n", objectId);
        return false;
    }
    return true;
}

void ShedMqttService::addDeviceMetadata(JsonDocument& document) {
    JsonObject device = document["device"].to<JsonObject>();
    JsonArray identifiers = device["identifiers"].to<JsonArray>();
    identifiers.add(AppConfig::DeviceId);
    device["name"] = AppConfig::DeviceName;
    device["manufacturer"] = "Lonely Binary";
    device["model"] = "ESP32-S3 N16R8 Shed Ventilation";
    device["sw_version"] = AppConfig::FirmwareVersion;
}

void ShedMqttService::messageCallback(char* topic, uint8_t* payload,
                                     unsigned int length) {
    if (activeInstance_ != nullptr) {
        activeInstance_->handleMessage(topic, payload, length);
    }
}
