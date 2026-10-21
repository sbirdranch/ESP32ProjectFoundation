#pragma once

#include <PubSubClient.h>
#include <WiFiClient.h>

class LedSwitch;

class MqttSwitch {
public:
    explicit MqttSwitch(LedSwitch& ledSwitch);
    void begin();
    void loop(bool wifiConnected);
    bool isConnected();

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
    bool wasConnected_ = false;

    static MqttSwitch* activeInstance_;
};