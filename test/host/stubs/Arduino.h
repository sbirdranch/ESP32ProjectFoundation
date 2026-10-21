#pragma once

extern unsigned long testMillis;
inline unsigned long millis() { return testMillis; }

struct TestSerial {
    void println(const char*) {}
};
inline TestSerial Serial;
