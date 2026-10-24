#pragma once
#include <Arduino.h>
#include <vector>
#include <string>
#include "WiFiClient.h"
struct Publication { std::string topic, payload; bool retained; };
class PubSubClient {
public:
    inline static std::vector<Publication> publications;
    inline static std::vector<std::string> subscriptions;
    inline static bool online = false, publishOk = true;
    inline static std::string willTopic, willPayload;
    inline static bool willRetained = false;
    inline static void (*callback)(char*, uint8_t*, unsigned int) = nullptr;
    explicit PubSubClient(WiFiClient&) {}
    void setServer(const char*, unsigned int) {}
    void setCallback(void (*cb)(char*, uint8_t*, unsigned int)) { callback = cb; }
    bool setBufferSize(int) { return true; }
    void setKeepAlive(int) {}
    void setSocketTimeout(int) {}
    bool connected() { return online; }
    int state() { return 0; }
    bool connect(const char*, const char* topic, int, bool retain, const char* payload) {
        willTopic = topic; willPayload = payload; willRetained = retain;
        online = true; return true;
    }
    bool connect(const char* id, const char*, const char*, const char* topic,
                 int qos, bool retain, const char* payload) {
        return connect(id, topic, qos, retain, payload);
    }
    bool subscribe(const char* topic) { subscriptions.emplace_back(topic); return true; }
    bool publish(const char* topic, const char* payload, bool retain) {
        if (!online || !publishOk) return false;
        publications.push_back({topic, payload, retain}); return true;
    }
    void loop() {}
    static void deliver(const char* topic, const std::string& payload) {
        callback(const_cast<char*>(topic), reinterpret_cast<uint8_t*>(const_cast<char*>(payload.data())), payload.size());
    }
};
