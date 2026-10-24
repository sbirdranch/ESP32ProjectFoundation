#include <cassert>
#include <cstdio>
#include <algorithm>
#include <ArduinoJson.h>
#include "WindowMqtt.h"
unsigned long testMillis = 5000;
void digitalWrite(int, int) {}
void pinMode(int, int) {}
constexpr char command[] = "devices/WindowOpener_Left/window/set";
constexpr char state[] = "devices/WindowOpener_Left/window/state";
int main() {
    Window::Controller c(30, 5);
    WindowMqtt mqtt(c); mqtt.begin(); mqtt.loop(true);
    assert(PubSubClient::willTopic == "devices/WindowOpener_Left/availability");
    assert(PubSubClient::willPayload == "offline" && PubSubClient::willRetained);
    assert(PubSubClient::subscriptions.size() == 2);
    JsonDocument discovery;
    bool found = false;
    for (auto& p : PubSubClient::publications) {
        assert(p.retained);
        if (p.topic == "homeassistant/cover/WindowOpener_Left/window/config") {
            assert(!deserializeJson(discovery, p.payload)); found = true;
        }
    }
    assert(found);
    assert(discovery["command_topic"] == command);
    assert(discovery["state_topic"] == state);
    assert(discovery["unique_id"] == "WindowOpener_Left_window");
    assert(discovery["retain"] == false);
    assert(discovery["payload_stop"] == "STOP");
    assert(discovery["value_template"] == "{{ 'None' if value == 'Unknown' else value }}");
    assert(PubSubClient::publications.back().payload == "Unknown");
    auto send = [&](const char* topic, const std::string& payload) {
        PubSubClient::deliver(topic, payload); mqtt.publishPendingState();
    };
    send(command, "OPEN"); assert(c.openOutput());
    assert(PubSubClient::publications.back().payload == "Opening");
    for (const auto& invalid : {"open", "OPEN ", "", "CLOSE\n"}) {
        send(command, invalid); assert(c.openOutput());
    }
    send(command, std::string("STOP\0x", 6)); assert(c.openOutput());
    for (const auto& invalid : {"bad", "{}", "[]", "{\"event\":\"rain_started\"}garbage", "{\"event\":\"rain_started\\u0000x\"}", "{\"event\":42}", "{\"event\":\"rain_stopped\"}"}) {
        send("weather/events", invalid); assert(c.openOutput());
    }
    send("weather/events", "{\"event\":\"rain_started\"}");
    assert(!c.openOutput() && !c.closeOutput());
    assert(PubSubClient::publications.back().payload == "Closing");
    testMillis += 5; c.update(testMillis); assert(c.closeOutput());
    send(command, "STOP"); assert(c.state() == Window::State::Unknown);
    send(command, "CLOSE"); assert(c.closeOutput());
    PubSubClient::online = false;
    testMillis += 30; c.update(testMillis); mqtt.loop(false);
    assert(c.state() == Window::State::Closed && !c.closeOutput());
    testMillis += 5000; mqtt.loop(true);
    assert(PubSubClient::publications.back().payload == "Closed");
    send(command, "OPEN");
    testMillis += 30; c.update(testMillis);
    PubSubClient::publishOk = false; mqtt.publishPendingState();
    PubSubClient::publishOk = true; mqtt.publishPendingState();
    assert(PubSubClient::publications.back().payload == "Open");
    assert(PubSubClient::publications.back().retained);
    puts("Window MQTT service protocol tests passed (fake transport, real JSON)");
}
