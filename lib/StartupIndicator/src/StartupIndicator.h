#pragma once

class LedSwitch;

class StartupIndicator {
public:
    explicit StartupIndicator(LedSwitch& ledSwitch);
    void begin();
    void update(bool wifiConnected, bool mqttConnected);

private:
    enum class Phase {
        FastWifi,
        SlowMqtt,
        Complete
    };

    void beginSlowPhase(unsigned long now);
    void complete();

    LedSwitch& ledSwitch_;
    Phase phase_ = Phase::Complete;
    unsigned long nextToggleAt_ = 0;
    unsigned long slowPhaseStartedAt_ = 0;
    unsigned int fastFlashCount_ = 0;
    bool ledOn_ = false;
};