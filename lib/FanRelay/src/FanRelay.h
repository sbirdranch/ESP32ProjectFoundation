#pragma once

class FanRelay {
public:
    void begin();
    void set(bool on);
    bool isOn() const;

private:
    bool isOn_ = false;
};