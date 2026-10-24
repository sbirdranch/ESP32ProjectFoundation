#include <Arduino.h>

#include "AppConfig.h"
#include "ManualButton.h"

void ManualButton::begin(unsigned long now) {
    pinMode(AppConfig::ManualButtonPin,
            AppConfig::ManualButtonActiveHigh ? INPUT_PULLDOWN : INPUT_PULLUP);
    lastRawLevel_ = digitalRead(AppConfig::ManualButtonPin);
    stablePressed_ = isPressedLevel(lastRawLevel_);
    lastChangeAt_ = now;
}

bool ManualButton::update(unsigned long now) {
    const int rawLevel = digitalRead(AppConfig::ManualButtonPin);
    if (rawLevel != lastRawLevel_) {
        lastRawLevel_ = rawLevel;
        lastChangeAt_ = now;
    }
    if (now - lastChangeAt_ < AppConfig::ButtonDebounceMs) {
        return false;
    }

    const bool pressed = isPressedLevel(rawLevel);
    if (pressed == stablePressed_) {
        return false;
    }

    stablePressed_ = pressed;
    return pressed;
}

bool ManualButton::isPressedLevel(int level) const {
    return (level == HIGH) == AppConfig::ManualButtonActiveHigh;
}