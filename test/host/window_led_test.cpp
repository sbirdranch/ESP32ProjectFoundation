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
    led.setColor(0, AppConfig::LedBrightness, 0);
    assert(testBlue == AppConfig::LedBrightness && testGreen == 0);
    testMillis = 2000; startup.update(true, true);
    testMillis = 4000; startup.update(true, true);
    assert(testGreen == AppConfig::LedBrightness && testBlue == 0);
    led.setColor(0, 0, AppConfig::LedBrightness);
    assert(testBlue == AppConfig::LedBrightness && testRed == 0);
    led.setColor(AppConfig::LedBrightness, 0, 0);
    assert(testRed == AppConfig::LedBrightness && testBlue == 0);
    puts("Window LED colors/startup handoff tests passed");
}
