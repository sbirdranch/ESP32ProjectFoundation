#pragma once

class ManualButton {
public:
    void begin(unsigned long now);
    bool update(unsigned long now);

private:
    bool isPressedLevel(int level) const;

    unsigned long lastChangeAt_ = 0;
    int lastRawLevel_ = 0;
    bool stablePressed_ = false;
};