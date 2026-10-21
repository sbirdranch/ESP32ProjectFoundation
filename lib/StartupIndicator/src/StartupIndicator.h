#pragma once

class LedSwitch;

class StartupIndicator {
public:
    explicit StartupIndicator(LedSwitch& ledSwitch);
    void begin();
    void update(bool wifiConnected, bool mqttConnected);

private:
    enum class Phase {
        SlowWifi,
        FastMqtt,
        Complete
    };

    void beginMqttPhase(unsigned long now);
    void complete();

    LedSwitch& ledSwitch_;
    Phase phase_ = Phase::Complete;
    unsigned long phaseStartedAt_ = 0;
    unsigned long lastToggleAt_ = 0;
    bool ledOn_ = false;
};
