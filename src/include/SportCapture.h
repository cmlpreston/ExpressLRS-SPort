#pragma once
#include "sport_crsf.h"
namespace onboard {
struct CapturedSample {
    SportSample sample{};
    uint32_t updatedMs = 0, count = 0;
    uint32_t minRawValue = 0, maxRawValue = 0;
};
struct RecentSportSample {
    SportSample sample{};
    uint32_t updatedMs = 0;
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
        recent[recentNext] = {s, now};
        recentNext = (recentNext + 1) % 16;
        if (recentUsed < 16) ++recentUsed;
        for (unsigned i=0; i<used; ++i) {
            if (entries[i].sample.physicalId == s.physicalId && entries[i].sample.appId == s.appId) {
                auto &entry = entries[i];
                entry.sample = s; entry.updatedMs = now; ++entry.count;
                if (s.value < entry.minRawValue) entry.minRawValue = s.value;
                if (s.value > entry.maxRawValue) entry.maxRawValue = s.value;
                return;
            }
        }
        if (used == 32) { ++droppedKeys; return; }
        entries[used++] = {s, now, 1, s.value, s.value};
    }
    RecentSportSample recent[16]{};
    uint8_t recentUsed = 0, recentNext = 0;
    SportWireObservation wire[32]{};
    uint32_t rxBytes = 0, txPhaseBytes = 0, replyPhaseBytes = 0, ignoredPrefixZeros = 0;
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
