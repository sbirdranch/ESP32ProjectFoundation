#include <cassert>
#include <cstdio>

#include "FanSetpointStore.h"
#include "Preferences.h"

using ShedVentilation::FanSetpointStore;

int main() {
    Preferences::clearForTest();

    FanSetpointStore firstBoot;
    assert(firstBoot.begin());
    assert(firstBoot.isAvailable());
    assert(firstBoot.get().offF == 80);
    assert(firstBoot.get().onF == 110);

    FanSetpointStore defaultsAfterReboot;
    assert(defaultsAfterReboot.begin());
    assert(defaultsAfterReboot.get().offF == 80);
    assert(defaultsAfterReboot.get().onF == 110);

    assert(!firstBoot.setOffSetpoint(119));
    assert(firstBoot.get().offF == 80);
    assert(firstBoot.setOnSetpoint(120));
    assert(firstBoot.setOffSetpoint(118));

    FanSetpointStore afterReboot;
    assert(afterReboot.begin());
    assert(afterReboot.get().offF == 118);
    assert(afterReboot.get().onF == 120);
    assert(!afterReboot.setOnSetpoint(121));
    assert(afterReboot.get().onF == 120);

    std::puts("Setpoint store tests passed");
}