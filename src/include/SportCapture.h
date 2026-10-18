#pragma once
#include "sport_crsf.h"
namespace onboard {
struct CapturedSample {
    SportSample sample{};
    uint32_t updatedMs = 0, count = 0;
};
// Retain the last nonempty byte observation for each polled physical ID.
// These are UART observations, not decoded sensor data; TX includes local echo.
struct SportWireObservation {
    uint8_t physicalId = 0, txLength = 0, rxLength = 0;
    uint8_t tx[16]{}, rx[16]{};
    uint32_t txBytes = 0, rxBytes = 0, firstRxUs = 0;
};
class SportCapture {
public:
    void beginPoll(uint8_t id, uint32_t nowUs) {
        currentId_ = id & 31;
        wire[currentId_].physicalId = id;
        pollUs_ = nowUs;
        txCount_ = rxCount_ = 0;
    }
    void observeByte(uint8_t byte, bool transmitting, bool discarded, uint32_t nowUs) {
        ++rxBytes;
        auto &row = wire[currentId_];
        if (transmitting) {
            ++txPhaseBytes; ++row.txBytes;
            if (discarded) ++releaseDiscardedBytes;
            if (txCount_ < sizeof(row.tx)) row.tx[txCount_] = byte;
            if (txCount_ < sizeof(row.tx)) ++txCount_;
            row.txLength = txCount_;
        } else {
            ++replyPhaseBytes; ++row.rxBytes;
            if (!rxCount_) row.firstRxUs = uint32_t(nowUs - pollUs_);
            if (rxCount_ < sizeof(row.rx)) row.rx[rxCount_] = byte;
            if (rxCount_ < sizeof(row.rx)) ++rxCount_;
            row.rxLength = rxCount_;
        }
    }
    void released(uint32_t nowUs) {
        const uint32_t elapsed = uint32_t(nowUs - pollUs_);
        if (elapsed > maxReleaseUs) maxReleaseUs = elapsed;
    }
    void accept(SportSample s, uint32_t now) {
        ++samples;
        for (unsigned i=0; i<used; ++i) {
            if (entries[i].sample.physicalId == s.physicalId && entries[i].sample.appId == s.appId) {
                entries[i].sample = s; entries[i].updatedMs = now; ++entries[i].count; return;
            }
        }
        if (used == 32) { ++droppedKeys; return; }
        entries[used++] = {s, now, 1};
    }
    SportWireObservation wire[32]{};
    uint32_t rxBytes = 0, txPhaseBytes = 0, replyPhaseBytes = 0;
    uint32_t releaseDiscardedBytes = 0, maxReleaseUs = 0;
    CapturedSample entries[32]{};
    uint32_t samples = 0, checksumErrors = 0, polls = 0, droppedKeys = 0;
    uint8_t used = 0;
    bool active = false;
private:
    uint32_t pollUs_ = 0;
    uint8_t currentId_ = 0, txCount_ = 0, rxCount_ = 0;
};
}
