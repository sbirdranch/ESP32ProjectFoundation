#include <cassert>
#include <cstdio>
#include "AppConfig.h"
#include "LedSwitch.h"
#include "StartupIndicator.h"
unsigned long testMillis = 0;
uint8_t testRed = 0, testGreen = 0, testBlue = 0;
void digitalWrite(int, int) {}
void pinMode(int, int) {}
int main() {
    LedSwitch led;
    StartupIndicator startup(led);
    led.begin(); startup.begin();
    led.setColor(AppConfig::LedBrightness, 0, 0); // Unknown
    assert(testBlue == AppConfig::LedBrightness && testGreen == 0);
    testMillis = 2000; startup.update(true, true);
    testMillis = 4000; startup.update(true, true);
    assert(testRed == AppConfig::LedBrightness && testGreen == 0 && testBlue == 0);
    led.setColor(0, 0, AppConfig::LedBrightness);
    assert(testBlue == AppConfig::LedBrightness && testRed == 0);
    led.setColor(0, AppConfig::LedBrightness, 0); // Closed
    assert(testGreen == AppConfig::LedBrightness && testRed == 0 && testBlue == 0);
    puts("Window LED colors/startup handoff tests passed");
}
