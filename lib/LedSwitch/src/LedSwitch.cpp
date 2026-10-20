#include <Arduino.h>

#include "AppConfig.h"
#include "LedSwitch.h"

void LedSwitch::begin() {
    pinMode(AppConfig::LedPin, OUTPUT);
    isOn_ = false;
    digitalWrite(AppConfig::LedPin, AppConfig::LedActiveLow ? HIGH : LOW);
    initialized_ = true;
}

void LedSwitch::set(bool on) {
    if (!initialized_ || isOn_ == on) {
        return;
    }

    isOn_ = on;
    const uint8_t output = (on != AppConfig::LedActiveLow) ? HIGH : LOW;
    digitalWrite(AppConfig::LedPin, output);
}

bool LedSwitch::isOn() const {
    return isOn_;
}