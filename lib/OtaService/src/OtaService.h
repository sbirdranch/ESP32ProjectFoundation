#pragma once

class OtaService {
public:
    void beginIfReady(bool wifiConnected);
    void loop();

private:
    bool started_ = false;
    bool warnedMissingPassword_ = false;
};