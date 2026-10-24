#include <cassert>
#include <cstdio>

#include <Arduino.h>

#include "AppConfig.h"
#include "FanRelay.h"
#include "ManualButton.h"

unsigned long testMillis = 0;
int testDigitalInputLevel = HIGH;
int testPinModeValue = 0;
int testDigitalOutputPin = -1;
int testDigitalOutputLevel = HIGH;

void checkButtonDebounce() {
    ManualButton button;
    testDigitalInputLevel = HIGH;
    button.begin(0);
    assert(testPinModeValue == INPUT_PULLUP);

    testDigitalInputLevel = LOW;
    assert(!button.update(10));
    assert(!button.update(49));
    assert(button.update(50));
    assert(!button.update(100));

    testDigitalInputLevel = HIGH;
    assert(!button.update(110));
    assert(!button.update(149));
    assert(!button.update(150));
}

void checkRelayOutput() {
    FanRelay relay;
    relay.begin();
    assert(!relay.isOn());
    assert(testDigitalOutputPin == AppConfig::FanRelayPin);
    assert(testDigitalOutputLevel == LOW);

    relay.set(true);
    assert(relay.isOn());
    assert(testDigitalOutputLevel == HIGH);
    relay.set(false);
    assert(!relay.isOn());
    assert(testDigitalOutputLevel == LOW);
}

int main() {
    checkButtonDebounce();
    checkRelayOutput();
    std::puts("GPIO control tests passed");
}