#include "FanSetpointStore.h"

#include "AppConfig.h"

namespace ShedVentilation {
namespace {
constexpr char kPreferencesNamespace[] = "shed-vent";
constexpr char kSetpointsKey[] = "setpoints";
}

bool FanSetpointStore::begin() {
    setpoints_ = {AppConfig::DefaultOffSetpointF, AppConfig::DefaultOnSetpointF};
    available_ = preferences_.begin(kPreferencesNamespace, false);
    storageError_ = !available_;
    if (!available_) {
        return false;
    }

    if (!preferences_.isKey(kSetpointsKey)) {
        return persist(setpoints_);
    }

    const FanSetpoints stored = decode(preferences_.getUInt(kSetpointsKey, 0));
    if (!areSetpointsValid(stored) || stored.offF < AppConfig::MinimumSetpointF ||
        stored.onF > AppConfig::MaximumSetpointF) {
        return persist(setpoints_);
    }

    setpoints_ = stored;
    return true;
}

bool FanSetpointStore::setOffSetpoint(int16_t temperatureF) {
    FanSetpoints candidate = setpoints_;
    candidate.offF = temperatureF;
    return persist(candidate);
}

bool FanSetpointStore::setOnSetpoint(int16_t temperatureF) {
    FanSetpoints candidate = setpoints_;
    candidate.onF = temperatureF;
    return persist(candidate);
}

const FanSetpoints& FanSetpointStore::get() const {
    return setpoints_;
}

bool FanSetpointStore::isAvailable() const {
    return available_;
}

bool FanSetpointStore::hasStorageError() const {
    return storageError_;
}

bool FanSetpointStore::persist(const FanSetpoints& setpoints) {
    if (!available_ || !areSetpointsValid(setpoints) ||
        setpoints.offF < AppConfig::MinimumSetpointF ||
        setpoints.offF > AppConfig::MaximumSetpointF ||
        setpoints.onF < AppConfig::MinimumSetpointF ||
        setpoints.onF > AppConfig::MaximumSetpointF) {
        return false;
    }
    const uint32_t encoded = encode(setpoints);
    if (preferences_.isKey(kSetpointsKey) &&
        preferences_.getUInt(kSetpointsKey, 0) == encoded) {
        setpoints_ = setpoints;
        return true;
    }
    if (preferences_.putUInt(kSetpointsKey, encoded) != sizeof(encoded)) {
        storageError_ = true;
        return false;
    }

    setpoints_ = setpoints;
    storageError_ = false;
    return true;
}

uint32_t FanSetpointStore::encode(const FanSetpoints& setpoints) {
    return static_cast<uint16_t>(setpoints.offF) |
           (static_cast<uint32_t>(static_cast<uint16_t>(setpoints.onF)) << 16);
}

FanSetpoints FanSetpointStore::decode(uint32_t encoded) {
    return {
        static_cast<int16_t>(encoded & 0xFFFFU),
        static_cast<int16_t>((encoded >> 16) & 0xFFFFU)};
}

}