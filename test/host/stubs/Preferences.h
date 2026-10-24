#pragma once

#include <stddef.h>
#include <stdint.h>

class Preferences {
public:
    bool begin(const char*, bool) {
        return true;
    }

    bool isKey(const char*) const {
        return hasValue();
    }

    uint32_t getUInt(const char*, uint32_t defaultValue) const {
        return hasValue() ? storedValue() : defaultValue;
    }

    size_t putUInt(const char*, uint32_t value) {
        storedValue() = value;
        hasValue() = true;
        return sizeof(value);
    }

    static void clearForTest() {
        storedValue() = 0;
        hasValue() = false;
    }

private:
    static uint32_t& storedValue() {
        static uint32_t value = 0;
        return value;
    }

    static bool& hasValue() {
        static bool value = false;
        return value;
    }
};