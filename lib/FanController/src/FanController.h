#pragma once

#include <stdint.h>

namespace ShedVentilation {

struct FanSetpoints {
    constexpr FanSetpoints(int16_t off = 80, int16_t on = 110)
        : offF(off), onF(on) {}

    int16_t offF;
    int16_t onF;
};

bool areSetpointsValid(const FanSetpoints& setpoints);
float celsiusToFahrenheit(float temperatureC);
float fahrenheitToCelsius(float temperatureF);

class FanController {
public:
    explicit FanController(bool initialFanState = false);

    void requestManualState(bool fanOn);
    void toggleManualState();
    void resumeAutomatic();
    void update(float indoorF, bool indoorValid, float outdoorF, bool outdoorFresh,
                const FanSetpoints& setpoints);

    bool fanOn() const;
    bool isManualOverrideActive() const;

private:
    bool automaticFanOn_ = false;
    bool fanOn_ = false;
    bool manualOverrideActive_ = false;
};

}
