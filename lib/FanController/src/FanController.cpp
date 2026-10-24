#include "FanController.h"

#include <algorithm>

#include "AppConfig.h"

namespace ShedVentilation {

bool areSetpointsValid(const FanSetpoints& setpoints) {
    return setpoints.offF >= AppConfig::MinimumSetpointF &&
           setpoints.offF <= AppConfig::MaximumSetpointF &&
           setpoints.onF >= AppConfig::MinimumSetpointF &&
           setpoints.onF <= AppConfig::MaximumSetpointF &&
           setpoints.offF + AppConfig::MinimumSetpointGapF <= setpoints.onF;
}

float celsiusToFahrenheit(float temperatureC) {
    return temperatureC * 9.0F / 5.0F + 32.0F;
}

float fahrenheitToCelsius(float temperatureF) {
    return (temperatureF - 32.0F) * 5.0F / 9.0F;
}

FanController::FanController(bool initialFanState)
    : automaticFanOn_(initialFanState), fanOn_(initialFanState) {}

void FanController::requestManualState(bool fanOn) {
    fanOn_ = fanOn;
    manualOverrideActive_ = true;
}

void FanController::toggleManualState() {
    requestManualState(!fanOn_);
}

void FanController::resumeAutomatic() {
    // Carry the current output into the thermostat's hysteresis band.
    automaticFanOn_ = fanOn_;
    manualOverrideActive_ = false;
}

void FanController::update(float indoorF, bool indoorValid, float outdoorF,
                           bool outdoorFresh, const FanSetpoints& setpoints) {
    if (!indoorValid || !areSetpointsValid(setpoints)) {
        return;
    }

    float effectiveOffF = static_cast<float>(setpoints.offF);
    if (outdoorFresh) {
        effectiveOffF = std::max(effectiveOffF,
                                  outdoorF + AppConfig::OutdoorCoolingMarginF);
    }
    const float effectiveOnF = std::max(
        static_cast<float>(setpoints.onF),
        effectiveOffF + static_cast<float>(AppConfig::FanRestartHysteresisF));

    const bool previousAutomaticState = automaticFanOn_;
    if (!automaticFanOn_ && indoorF >= effectiveOnF) {
        automaticFanOn_ = true;
    } else if (automaticFanOn_ && indoorF <= effectiveOffF) {
        automaticFanOn_ = false;
    }

    if (manualOverrideActive_ && automaticFanOn_ != previousAutomaticState) {
        manualOverrideActive_ = false;
    }
    if (!manualOverrideActive_) {
        fanOn_ = automaticFanOn_;
    }
}

bool FanController::fanOn() const {
    return fanOn_;
}

bool FanController::isManualOverrideActive() const {
    return manualOverrideActive_;
}

}
