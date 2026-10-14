#pragma once
#include "sport_crsf.h"
namespace onboard {
struct CapturedSample {
    SportSample sample{};
    uint32_t updatedMs = 0, count = 0;
};
class SportCapture {
public:
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
    CapturedSample entries[32]{};
    uint32_t samples = 0, checksumErrors = 0, polls = 0, droppedKeys = 0;
    uint8_t used = 0;
    bool active = false;
};
}
