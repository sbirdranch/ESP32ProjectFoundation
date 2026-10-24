#pragma once

#include <PubSubClient.h>
#include <WiFiClient.h>

#include "WindowController.h"

class WindowMqtt {
public:
    explicit WindowMqtt(Window::Controller& controller);
    void begin();
    void loop(bool wifiConnected);
    bool isConnected();
    void publishState();
    void publishPendingState();

private:
    void connect();
    void handleMessage(const char* topic, const uint8_t* payload, unsigned int length);
    void publishDiscovery();
    static void messageCallback(char* topic, uint8_t* payload, unsigned int length);

    WiFiClient networkClient_;
    PubSubClient mqttClient_;
    Window::Controller& controller_;
    unsigned long lastAttemptAt_ = 0;
    Window::State publishedState_ = Window::State::Unknown;
    bool dirty_ = true;
    bool wasConnected_ = false;

    static WindowMqtt* activeInstance_;
};