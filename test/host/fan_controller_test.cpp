#include <cassert>
#include <cstdio>

#include "FanController.h"

using ShedVentilation::FanController;
using ShedVentilation::FanSetpoints;

void checkBaseHysteresis() {
    FanController controller;
    const FanSetpoints setpoints{};

    controller.update(109.9F, true, 0.0F, false, setpoints);
    assert(!controller.fanOn());
    controller.update(110.0F, true, 0.0F, false, setpoints);
    assert(controller.fanOn());
    controller.update(100.0F, true, 0.0F, false, setpoints);
    assert(controller.fanOn());
    controller.update(80.0F, true, 0.0F, false, setpoints);
    assert(!controller.fanOn());
}

void checkOutdoorAdjustment() {
    FanController controller;
    const FanSetpoints setpoints{};

    controller.update(110.0F, true, 100.0F, true, setpoints);
    assert(controller.fanOn());
    controller.update(102.1F, true, 100.0F, true, setpoints);
    assert(controller.fanOn());
    controller.update(102.0F, true, 100.0F, true, setpoints);
    assert(!controller.fanOn());

    controller.update(119.0F, true, 115.0F, true, setpoints);
    assert(controller.fanOn());
    controller.update(117.0F, true, 115.0F, true, setpoints);
    assert(!controller.fanOn());

    controller.update(110.0F, true, 108.0F, true, setpoints);
    assert(!controller.fanOn());
    controller.update(112.0F, true, 108.0F, true, setpoints);
    assert(controller.fanOn());

    controller.update(112.0F, true, 113.0F, true, setpoints);
    assert(!controller.fanOn());
}

void checkTemperatureConversions() {
    assert(ShedVentilation::celsiusToFahrenheit(0.0F) == 32.0F);
    assert(ShedVentilation::celsiusToFahrenheit(100.0F) == 212.0F);
    assert(ShedVentilation::fahrenheitToCelsius(32.0F) == 0.0F);
    assert(ShedVentilation::fahrenheitToCelsius(212.0F) == 100.0F);
}

void checkSetpointValidationAndFallback() {
    assert(ShedVentilation::areSetpointsValid({60, 62}));
    assert(ShedVentilation::areSetpointsValid({118, 120}));
    assert(!ShedVentilation::areSetpointsValid({59, 110}));
    assert(!ShedVentilation::areSetpointsValid({80, 121}));
    assert(!ShedVentilation::areSetpointsValid({110, 110}));

    FanController controller;
    const FanSetpoints setpoints{80, 110};
    controller.update(109.9F, true, 115.0F, false, setpoints);
    assert(!controller.fanOn());
    controller.update(110.0F, true, 115.0F, false, setpoints);
    assert(controller.fanOn());
}

void checkManualOverrideAndSensorFault() {
    FanController controller;
    const FanSetpoints setpoints{};

    controller.requestManualState(true);
    assert(controller.fanOn());
    controller.update(109.0F, true, 0.0F, false, setpoints);
    assert(controller.fanOn());
    assert(controller.isManualOverrideActive());
    controller.update(110.0F, true, 0.0F, false, setpoints);
    assert(controller.fanOn());
    assert(!controller.isManualOverrideActive());

    controller.requestManualState(false);
    controller.update(100.0F, false, 0.0F, false, setpoints);
    assert(!controller.fanOn());
    assert(controller.isManualOverrideActive());
    controller.update(80.0F, true, 0.0F, false, setpoints);
    assert(!controller.fanOn());
    assert(!controller.isManualOverrideActive());
}

void checkResumeAutomatic() {
    FanController controller;
    const FanSetpoints setpoints{};
    controller.update(100.0F, true, 0.0F, false, setpoints);
    controller.requestManualState(true);
    controller.resumeAutomatic();
    assert(!controller.isManualOverrideActive());
    controller.update(100.0F, true, 0.0F, false, setpoints);
    assert(controller.fanOn());
    controller.resumeAutomatic(); // Repeated presses preserve automatic operation.
    controller.update(80.1F, true, 0.0F, false, setpoints);
    assert(controller.fanOn());
    controller.update(80.0F, true, 0.0F, false, setpoints);
    assert(!controller.fanOn());
    controller.update(110.0F, true, 0.0F, false, setpoints);
    assert(controller.fanOn());

    controller.requestManualState(false);
    controller.resumeAutomatic();
    controller.update(100.0F, true, 0.0F, false, setpoints);
    assert(!controller.fanOn());
    controller.update(110.0F, true, 0.0F, false, setpoints);
    assert(controller.fanOn());

    controller.requestManualState(true);
    controller.resumeAutomatic();
    controller.update(70.0F, false, 0.0F, false, setpoints);
    assert(controller.fanOn()); // Invalid sensor still holds output.
    controller.update(70.0F, true, 0.0F, false, setpoints);
    assert(!controller.fanOn());

    controller.requestManualState(true);
    controller.resumeAutomatic();
    controller.update(102.0F, true, 100.0F, true, setpoints);
    assert(!controller.fanOn()); // Outdoor-adjusted OFF threshold still applies.
    controller.requestManualState(false);
    controller.resumeAutomatic();
    controller.update(120.0F, true, 0.0F, false, setpoints);
    assert(controller.fanOn()); // Already above ON acts on the next update.
}

int main() {
    checkResumeAutomatic();
    checkBaseHysteresis();
    checkOutdoorAdjustment();
    checkTemperatureConversions();
    checkSetpointValidationAndFallback();
    checkManualOverrideAndSensorFault();
    std::puts("Fan controller tests passed");
}
