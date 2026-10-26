#pragma once
#include "vehicle_sensors.h"

namespace onboard {
// Hardware adapter must discard TX echo and call txComplete only after the
// final stop bit. No waits, allocation or UART dependencies in this core.
class SportBus {
public:
    enum class Action { None, Poll, Release };
    Action tick(uint32_t nowUs, bool txIdle);
    void feed(uint8_t byte, uint32_t nowMs);
    uint8_t physicalId() const { return physicalId_; }
    bool takeSample(SportSample &sample);
    uint32_t samples() const { return samples_; }
    uint32_t ignoredPrefixZeros() const { return ignoredPrefixZeros_; }
    uint32_t checksumErrors() const { return parser_.checksumErrors(); }
private:
    SportParser parser_;
    SportSample sample_{};
    uint32_t startedUs_ = 0, releasedUs_ = 0, samples_ = 0, ignoredPrefixZeros_ = 0;
    uint8_t index_ = 0, physicalId_ = 0;
    enum State { Ready, Sending, Listening } state_ = Ready;
    bool pending_ = false, prefix_ = false, completed_ = false;
};

class SportTelemetry {
public:
    explicit SportTelemetry(VehicleSensorSelection selection,
        SensorKey voltage = {0xff,0xffff}, SensorKey current = {0xff,0xffff})
        : sensors_(selection, voltage, current) {}
    void accept(const SportSample &sample, uint32_t nowMs) { sensors_.accept(sample, nowMs); }
    const VehicleSensors &sensors() const { return sensors_; }
    // Cells use 0x0e source 0; FAS battery 0x08 uses explicit ID 1, never RX supply ID 0.
    size_t nextFrame(uint32_t nowMs, uint8_t *out, size_t capacity);
private:
    VehicleSensors sensors_;
    uint32_t sentMs_ = 0;
    uint8_t next_ = 0;
    bool sent_ = false;
};
}
