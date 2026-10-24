#pragma once

class LedSwitch {
public:
    void begin();
    void set(bool on);
    void setColor(unsigned char red, unsigned char green, unsigned char blue);
    bool isOn() const;
    void showStartupPulse(bool on);
    void endStartup();

private:
    void showSwitchState();

    unsigned char red_ = 0, green_ = 0, blue_ = 0;
    bool isOn_ = false;
    bool initialized_ = false;
    bool startupActive_ = false;
};
