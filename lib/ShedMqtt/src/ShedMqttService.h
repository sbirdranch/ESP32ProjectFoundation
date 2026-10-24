#pragma once

#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFiClient.h>

#include "FanController.h"
#include "FanSetpointStore.h"
#include "Sht40Sensor.h"

class ShedMqttService {
public:
    ShedMqttService(ShedVentilation::FanController& fanController,
                    ShedVentilation::FanSetpointStore& setpointStore,
                    Sht40Sensor& sensor);

    void begin();
    void loop(bool wifiConnected);
    void publishPendingState(unsigned long now);
    void markStateDirty();
    bool isConnected();
    bool hasFreshOutdoorTemperature(unsigned long now) const;
    float outdoorTemperatureC() const;

private:
    void connect();
    void handleMessage(const char* topic, const uint8_t* payload, unsigned int length);
    void publishDiscovery();
    bool publishState(unsigned long now);
    void publishStateIfNeeded(unsigned long now);
    bool publishDiscoveryConfig(const char* component, const char* objectId,
                                JsonDocument& document);
    void addDeviceMetadata(JsonDocument& document);
    static void messageCallback(char* topic, uint8_t* payload, unsigned int length);

    WiFiClient networkClient_;
    PubSubClient mqttClient_;
    ShedVentilation::FanController& fanController_;
    ShedVentilation::FanSetpointStore& setpointStore_;
    Sht40Sensor& sensor_;
    unsigned long lastAttemptAt_ = 0;
    unsigned long lastStateAt_ = 0;
    unsigned long outdoorReceivedAt_ = 0;
    float outdoorTemperatureC_ = 0.0F;
    bool wasConnected_ = false;
    bool stateDirty_ = true;
    bool hasOutdoorTemperature_ = false;

    static ShedMqttService* activeInstance_;
};