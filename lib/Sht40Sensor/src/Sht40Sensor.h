#pragma once

#include <SensirionI2cSht4x.h>

class Sht40Sensor {
public:
    bool begin();
    bool update(unsigned long now);

    bool isReadingValid() const;
    bool hasValidReading() const;
    float temperatureC() const;
    float humidityPercent() const;
    unsigned long readingAgeMs(unsigned long now) const;

private:
    bool initializeSensor();

    SensirionI2cSht4x sensor_;
    unsigned long lastPollAt_ = 0;
    unsigned long lastValidAt_ = 0;
    float temperatureC_ = 0.0F;
    float humidityPercent_ = 0.0F;
    bool wireStarted_ = false;
    bool sensorReady_ = false;
    bool hasValidReading_ = false;
    bool readingValid_ = false;
    bool hasPolled_ = false;
};