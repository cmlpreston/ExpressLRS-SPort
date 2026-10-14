#pragma once
#include <stdint.h>
namespace onboard {
// ESP8266 exposes FIFO count but not a documented shift-register idle flag.
// FIFO empty can still leave one 8N1 character in flight. Wait >= 10 bits at
// 57600 baud after first observing empty, without blocking the receiver loop.
class Er4TxDrain {
public:
    void begin() { waiting_ = false; }
    bool ready(uint32_t nowUs, bool fifoEmpty) {
        if (!fifoEmpty) { waiting_ = false; return false; }
        if (!waiting_) { emptyUs_ = nowUs; waiting_ = true; return false; }
        return uint32_t(nowUs - emptyUs_) >= 180;
    }
private:
    uint32_t emptyUs_ = 0;
    bool waiting_ = false;
};
}
