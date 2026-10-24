#include <Arduino.h>

#include "AppConfig.h"
#include "FanRelay.h"

void FanRelay::begin() {
    isOn_ = false;
    digitalWrite(AppConfig::FanRelayPin, AppConfig::FanRelayActiveHigh ? LOW : HIGH);
    pinMode(AppConfig::FanRelayPin, OUTPUT);
}

void FanRelay::set(bool on) {
    if (isOn_ == on) {
        return;
    }
    isOn_ = on;
    const bool pinHigh = on == AppConfig::FanRelayActiveHigh;
    digitalWrite(AppConfig::FanRelayPin, pinHigh ? HIGH : LOW);
}

bool FanRelay::isOn() const {
    return isOn_;
}