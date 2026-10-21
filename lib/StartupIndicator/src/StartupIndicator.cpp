#include <Arduino.h>

#include "LedSwitch.h"
#include "StartupIndicator.h"

namespace {
constexpr unsigned long kFastHalfPeriodMs = 250;
constexpr unsigned long kSlowHalfPeriodMs = 500;
constexpr unsigned long kMinimumPhaseDurationMs = 2000;
}

StartupIndicator::StartupIndicator(LedSwitch& ledSwitch)
    : ledSwitch_(ledSwitch) {}

void StartupIndicator::begin() {
    const unsigned long now = millis();
    phase_ = Phase::SlowWifi;
    phaseStartedAt_ = now;
    lastToggleAt_ = now;
    ledOn_ = true;
    ledSwitch_.showStartupPulse(ledOn_);
    Serial.println("Startup LED: slow Wi-Fi pattern (minimum 2 seconds)");
}

void StartupIndicator::update(bool wifiConnected, bool mqttConnected) {
    if (phase_ == Phase::Complete) {
        return;
    }

    const unsigned long now = millis();
    // Each phase requires both its connection and its own minimum display time.
    // An already-connected MQTT client must not skip either visible pattern.
    if (now - phaseStartedAt_ >= kMinimumPhaseDurationMs) {
        if (phase_ == Phase::SlowWifi && wifiConnected) {
            beginMqttPhase(now);
            return;
        }
        if (phase_ == Phase::FastMqtt && wifiConnected && mqttConnected) {
            Serial.println("Startup LED: connections ready; minimum patterns complete");
            complete();
            return;
        }
    }

    const unsigned long halfPeriodMs =
        phase_ == Phase::SlowWifi ? kSlowHalfPeriodMs : kFastHalfPeriodMs;
    if (now - lastToggleAt_ < halfPeriodMs) {
        return;
    }
    lastToggleAt_ = now;
    ledOn_ = !ledOn_;
    ledSwitch_.showStartupPulse(ledOn_);
}

void StartupIndicator::beginMqttPhase(unsigned long now) {
    phase_ = Phase::FastMqtt;
    phaseStartedAt_ = now;
    lastToggleAt_ = now;
    ledOn_ = true;
    ledSwitch_.showStartupPulse(ledOn_);
    Serial.println("Startup LED: fast MQTT pattern (minimum 2 seconds)");
}

void StartupIndicator::complete() {
    phase_ = Phase::Complete;
    ledSwitch_.endStartup();
}
