#pragma once

class LedSwitch {
public:
    void begin();
    void set(bool on);
    bool isOn() const;

private:
    bool isOn_ = false;
    bool initialized_ = false;
};