#include "Sht40Sensor.h"

#include <math.h>
#include <Wire.h>

#include "AppConfig.h"

bool Sht40Sensor::begin() {
    if (!wireStarted_) {
        Wire.begin(AppConfig::I2cSdaPin, AppConfig::I2cSclPin);
        wireStarted_ = true;
    }
    return initializeSensor();
}

bool Sht40Sensor::update(unsigned long now) {
    if (hasPolled_ && now - lastPollAt_ < AppConfig::SensorPollMs) {
        return false;
    }

    hasPolled_ = true;
    lastPollAt_ = now;
    if (!sensorReady_ && !initializeSensor()) {
        readingValid_ = false;
        return true;
    }

    float temperatureC = 0.0F;
    float humidityPercent = 0.0F;
    if (sensor_.measureHighPrecision(temperatureC, humidityPercent) != 0 ||
        !isfinite(temperatureC) || !isfinite(humidityPercent) ||
        humidityPercent < 0.0F || humidityPercent > 100.0F) {
        readingValid_ = false;
        return true;
    }

    temperatureC_ = temperatureC;
    humidityPercent_ = humidityPercent;
    lastValidAt_ = now;
    hasValidReading_ = true;
    readingValid_ = true;
    return true;
}

bool Sht40Sensor::isReadingValid() const {
    return readingValid_;
}

bool Sht40Sensor::hasValidReading() const {
    return hasValidReading_;
}

float Sht40Sensor::temperatureC() const {
    return temperatureC_;
}

float Sht40Sensor::humidityPercent() const {
    return humidityPercent_;
}

unsigned long Sht40Sensor::readingAgeMs(unsigned long now) const {
    return hasValidReading_ ? now - lastValidAt_ : 0;
}

bool Sht40Sensor::initializeSensor() {
    sensor_.begin(Wire, AppConfig::Sht40Address);
    sensorReady_ = sensor_.softReset() == 0;
    if (sensorReady_) {
        Serial.printf("SHT40 ready at I2C address 0x%02X\n", AppConfig::Sht40Address);
    }
    return sensorReady_;
}