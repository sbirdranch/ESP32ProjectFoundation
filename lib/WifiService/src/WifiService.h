#pragma once

class WifiService {
public:
    void begin();
    void loop();
    bool isConnected() const;

private:
    bool started_ = false;
    bool warnedMissingSsid_ = false;
    bool wasConnected_ = false;
    unsigned long lastAttemptAt_ = 0;
};