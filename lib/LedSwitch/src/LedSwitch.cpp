#include <esp32-hal-rgb-led.h>

#include "AppConfig.h"
#include "LedSwitch.h"

void LedSwitch::begin() {
    isOn_ = false;
    startupActive_ = false;
    // The Arduino core sends GRB data using its own RMT implementation.
    neopixelWrite(AppConfig::LedPin, 0, 0, 0);
    initialized_ = true;
}

void LedSwitch::set(bool on) {
    if (!initialized_ || isOn_ == on) {
        return;
    }

    isOn_ = on;
    if (!startupActive_) {
        showSwitchState();
    }
}

bool LedSwitch::isOn() const {
    return isOn_;
}

void LedSwitch::showStartupPulse(bool on) {
    if (!initialized_) {
        return;
    }

    startupActive_ = true;
    neopixelWrite(AppConfig::LedPin, 0, 0, on ? AppConfig::LedBrightness : 0);
}

void LedSwitch::endStartup() {
    startupActive_ = false;
    showSwitchState();
}

void LedSwitch::showSwitchState() {
    neopixelWrite(AppConfig::LedPin, 0, isOn_ ? AppConfig::LedBrightness : 0, 0);
}
