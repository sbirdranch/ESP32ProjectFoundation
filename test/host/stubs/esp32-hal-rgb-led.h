#pragma once

#include <stdint.h>

extern uint8_t testRed;
extern uint8_t testGreen;
extern uint8_t testBlue;
inline void neopixelWrite(uint8_t, uint8_t red, uint8_t green, uint8_t blue) {
    testRed = red;
    testGreen = green;
    testBlue = blue;
}
