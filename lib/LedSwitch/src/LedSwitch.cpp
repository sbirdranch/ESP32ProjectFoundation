#include "AppConfig.h"
#include "LedSwitch.h"

void LedSwitch::begin() {
    FastLED.addLeds<WS2812, AppConfig::LedPin, GRB>(pixels_, 1);
    FastLED.setBrightness(AppConfig::LedBrightness);
    isOn_ = false;
    pixels_[0] = CRGB::Black;
    FastLED.show();
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
    pixels_[0] = on ? CRGB::Blue : CRGB::Black;
    FastLED.show();
}

void LedSwitch::endStartup() {
    startupActive_ = false;
    showSwitchState();
}

void LedSwitch::showSwitchState() {
    pixels_[0] = isOn_ ? CRGB::Green : CRGB::Black;
    FastLED.show();
}