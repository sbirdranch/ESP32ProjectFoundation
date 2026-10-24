#include <esp32-hal-rgb-led.h>

#include "AppConfig.h"
#include "LedSwitch.h"

void LedSwitch::begin() {
    isOn_ = false;
    red_ = green_ = blue_ = 0;
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
    red_ = blue_ = 0;
    green_ = on ? AppConfig::LedBrightness : 0;
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

void LedSwitch::setColor(unsigned char red, unsigned char green, unsigned char blue) {
    if (red_ == red && green_ == green && blue_ == blue) return;
    red_ = red; green_ = green; blue_ = blue;
    if (initialized_ && !startupActive_) showSwitchState();
}

void LedSwitch::showSwitchState() {
    neopixelWrite(AppConfig::LedPin, red_, green_, blue_);
}
