#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string>
using String = std::string;
extern unsigned long testMillis;
inline unsigned long millis() { return testMillis; }
constexpr int LOW = 0, HIGH = 1, OUTPUT = 1;
extern void digitalWrite(int pin, int value);
extern void pinMode(int pin, int mode);
struct TestSerial {
    void println(const char* = "") {}
    template <typename... Args> void printf(const char*, Args...) {}
    void write(const uint8_t*, size_t) {}
};
inline TestSerial Serial;
