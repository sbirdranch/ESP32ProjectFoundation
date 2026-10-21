#include <cassert>
#include <cstdio>
#include <limits>

#include "AppConfig.h"
#include "LedSwitch.h"
#include "StartupIndicator.h"

unsigned long testMillis = 0;
uint8_t testRed = 0;
uint8_t testGreen = 0;
uint8_t testBlue = 0;

void expectColor(uint8_t green, uint8_t blue) {
    assert(testRed == 0 && testGreen == green && testBlue == blue);
}

void checkImmediateConnections(unsigned long start) {
    LedSwitch led;
    StartupIndicator indicator(led);
    testMillis = start;
    led.begin();
    indicator.begin();
    led.set(true); // MQTT ON must wait for both display phases to finish.
    for (unsigned long elapsed = 0; elapsed < 4000; elapsed += 125) {
        testMillis = start + elapsed;
        indicator.update(true, true);
        const unsigned long phaseElapsed = elapsed < 2000 ? elapsed : elapsed - 2000;
        const unsigned long halfPeriod = elapsed < 2000 ? 500 : 250;
        expectColor(0, (phaseElapsed / halfPeriod) % 2 == 0 ? AppConfig::LedBrightness : 0);
    }
    testMillis = start + 4000;
    indicator.update(true, true);
    expectColor(AppConfig::LedBrightness, 0);
    led.set(false);
    expectColor(0, 0);
    testMillis += 500;
    indicator.update(false, false); // Startup sequence stays finished.
    expectColor(0, 0);
}

void checkDelayedConnections() {
    LedSwitch led;
    StartupIndicator indicator(led);
    testMillis = 0;
    led.begin();
    indicator.begin();
    for (testMillis = 0; testMillis < 5000; testMillis += 250) {
        indicator.update(false, false);
        expectColor(0, (testMillis / 500) % 2 == 0 ? AppConfig::LedBrightness : 0);
    }
    // Wi-Fi only becomes ready after five seconds; MQTT gets its own full phase.
    indicator.update(true, false);
    for (testMillis = 5250; testMillis < 8000; testMillis += 250) {
        indicator.update(true, false);
        expectColor(0, ((testMillis - 5000) / 250) % 2 == 0 ? AppConfig::LedBrightness : 0);
    }
    indicator.update(true, true);
    expectColor(0, 0);
}

int main() {
    checkImmediateConnections(0);
    checkImmediateConnections(std::numeric_limits<unsigned long>::max() - 1000);
    checkDelayedConnections();
    std::puts("Startup indicator tests passed");
}
