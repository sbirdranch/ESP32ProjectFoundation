#pragma once

#include <PubSubClient.h>
#include <WiFiClient.h>

class LedSwitch;

class MqttSwitch {
public:
    explicit MqttSwitch(LedSwitch& ledSwitch);
    void begin();
    void loop(bool wifiConnected);

private:
    void connect();
    void handleMessage(const char* topic, const uint8_t* payload, unsigned int length);
    void publishDiscovery();
    void publishState();
    static void messageCallback(char* topic, uint8_t* payload, unsigned int length);

    WiFiClient networkClient_;
    PubSubClient mqttClient_;
    LedSwitch& ledSwitch_;
    unsigned long lastAttemptAt_ = 0;

    static MqttSwitch* activeInstance_;
};