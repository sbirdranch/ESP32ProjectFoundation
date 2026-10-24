#pragma once

#include <Preferences.h>

#include "FanController.h"

namespace ShedVentilation {

class FanSetpointStore {
public:
    bool begin();
    bool setOffSetpoint(int16_t temperatureF);
    bool setOnSetpoint(int16_t temperatureF);

    const FanSetpoints& get() const;
    bool isAvailable() const;
    bool hasStorageError() const;

private:
    bool persist(const FanSetpoints& setpoints);
    static uint32_t encode(const FanSetpoints& setpoints);
    static FanSetpoints decode(uint32_t encoded);

    Preferences preferences_;
    FanSetpoints setpoints_;
    bool available_ = false;
    bool storageError_ = false;
};

}