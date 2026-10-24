#pragma once

class OtaService {
public:
    void beginIfReady(bool wifiConnected);
    void loop();
    void setBeforeUpdate(void (*callback)()) { beforeUpdate_ = callback; }

private:
    void (*beforeUpdate_)() = nullptr;
    bool started_ = false;
    bool warnedMissingPassword_ = false;
};