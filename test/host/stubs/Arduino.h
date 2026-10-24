#pragma once

extern unsigned long testMillis;
inline unsigned long millis() { return testMillis; }

constexpr int LOW = 0;
constexpr int HIGH = 1;
constexpr int INPUT_PULLUP = 2;
constexpr int INPUT_PULLDOWN = 3;
constexpr int OUTPUT = 4;

extern int testDigitalInputLevel;
extern int testPinModeValue;
extern int testDigitalOutputPin;
extern int testDigitalOutputLevel;

inline int digitalRead(int) { return testDigitalInputLevel; }
inline void pinMode(int, int mode) { testPinModeValue = mode; }
inline void digitalWrite(int pin, int level) {
    testDigitalOutputPin = pin;
    testDigitalOutputLevel = level;
}

struct TestSerial {
    void println(const char*) {}
};
inline TestSerial Serial;
