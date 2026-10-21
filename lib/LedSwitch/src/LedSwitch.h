#pragma once

#include <FastLED.h>

class LedSwitch {
public:
    void begin();
    void set(bool on);
    bool isOn() const;
    void showStartupPulse(bool on);
    void endStartup();

private:
    void showSwitchState();

    CRGB pixels_[1];
    bool isOn_ = false;
    bool initialized_ = false;
    bool startupActive_ = false;
};