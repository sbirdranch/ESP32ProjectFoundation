#include <Arduino.h>

#include "LedSwitch.h"
#include "StartupIndicator.h"

namespace {
constexpr unsigned long kFastHalfPeriodMs = 250;
constexpr unsigned long kSlowHalfPeriodMs = 500;
constexpr unsigned long kSlowPhaseDurationMs = 2000;
constexpr unsigned int kFastFlashLimit = 4;
}

StartupIndicator::StartupIndicator(LedSwitch& ledSwitch)
    : ledSwitch_(ledSwitch) {}

void StartupIndicator::begin() {
    const unsigned long now = millis();
    phase_ = Phase::FastWifi;
    fastFlashCount_ = 0;
    ledOn_ = true;
    nextToggleAt_ = now + kFastHalfPeriodMs;
    ledSwitch_.showStartupPulse(ledOn_);
    Serial.println("Startup LED: fast Wi-Fi search pattern");
}

void StartupIndicator::update(bool wifiConnected, bool mqttConnected) {
    if (phase_ == Phase::Complete) {
        return;
    }

    const unsigned long now = millis();
    if (mqttConnected) {
        Serial.println("Startup LED: MQTT connected");
        complete();
        return;
    }

    if (phase_ == Phase::FastWifi &&
        (wifiConnected || fastFlashCount_ >= kFastFlashLimit)) {
        beginSlowPhase(now);
    }

    if (phase_ == Phase::SlowMqtt && now - slowPhaseStartedAt_ >= kSlowPhaseDurationMs) {
        Serial.println("Startup LED: connection pattern complete");
        complete();
        return;
    }

    if (static_cast<long>(now - nextToggleAt_) < 0) {
        return;
    }

    ledOn_ = !ledOn_;
    ledSwitch_.showStartupPulse(ledOn_);

    if (phase_ == Phase::FastWifi) {
        if (!ledOn_) {
            ++fastFlashCount_;
        }
        nextToggleAt_ = now + kFastHalfPeriodMs;
        if (fastFlashCount_ >= kFastFlashLimit) {
            beginSlowPhase(now);
        }
    } else {
        nextToggleAt_ = now + kSlowHalfPeriodMs;
    }
}

void StartupIndicator::beginSlowPhase(unsigned long now) {
    phase_ = Phase::SlowMqtt;
    slowPhaseStartedAt_ = now;
    ledOn_ = true;
    nextToggleAt_ = now + kSlowHalfPeriodMs;
    ledSwitch_.showStartupPulse(ledOn_);
    Serial.println("Startup LED: slow MQTT search pattern");
}

void StartupIndicator::complete() {
    phase_ = Phase::Complete;
    ledSwitch_.endStartup();
}