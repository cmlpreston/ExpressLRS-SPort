#include "SportReceiver.h"
namespace onboard {
namespace {
// S.Port physical IDs include the protocol parity bits.
constexpr uint8_t ids[] = {0x00,0xa1,0x22,0x83,0xe4,0x45,0xc6,0x67,
    0x48,0xe9,0x6a,0xcb,0xac,0x0d,0x8e,0x2f,0xd0,0x71,0xf2,0x53,
    0x34,0x95,0x16,0xb7,0x98,0x39,0xba,0x1b,0x7c,0xdd,0x5e,0xff};
void be16(uint8_t *p, uint16_t v) { p[0] = v >> 8; p[1] = v; }
void be32(uint8_t *p, uint32_t v) { p[0]=v>>24; p[1]=v>>16; p[2]=v>>8; p[3]=v; }
}
SportBus::Action SportBus::tick(uint32_t nowUs, bool txIdle)
{
    if (state_ == Sending) {
        if (!txIdle) return Action::None;
        prefix_ = true;
        releasedUs_ = nowUs;
        state_ = Listening;
        return Action::Release;
    }
    if (state_ == Listening && (uint32_t(nowUs - startedUs_) < 12000 ||
        uint32_t(nowUs - releasedUs_) < 10000)) return Action::None;
    pending_ = false; completed_ = false;
    physicalId_ = ids[index_++];
    if (index_ == sizeof(ids)) index_ = 0;
    startedUs_ = nowUs;
    state_ = Sending;
    return Action::Poll;
}
void SportBus::feed(uint8_t byte, uint32_t nowMs)
{
    if (state_ != Listening || completed_) return;
    if (prefix_) {
        // The ER4 UART can report a zero when inverted TX is released, even
        // with sensors disconnected. A data reply starts with 0x10; ignore
        // only leading zero artifacts, never zeros within a started frame.
        if (byte == 0) { ++ignoredPrefixZeros_; return; }
        parser_.feed(0x7e, nowMs, sample_);
        parser_.feed(physicalId_, nowMs, sample_);
        prefix_ = false;
    }
    if (parser_.feed(byte, nowMs, sample_) && sample_.physicalId == physicalId_) {
        pending_ = true; completed_ = true; ++samples_;
    }
}
bool SportBus::takeSample(SportSample &sample)
{
    if (!pending_) return false;
    sample = sample_; pending_ = false; return true;
}
size_t SportTelemetry::nextFrame(uint32_t now, uint8_t *out, size_t capacity)
{
    if (!out || capacity < 19 || (sent_ && uint32_t(now - sentMs_) < 200)) return 0;
    for (unsigned attempt = 0; attempt < 5; ++attempt) {
        const uint8_t kind = next_;
        next_ = (next_ + 1) % 5;
        size_t payload = 0;
        if (kind == 0 && sensors_.latitudeE7.fresh(now) && sensors_.longitudeE7.fresh(now) &&
            sensors_.speedDeciKmh.fresh(now) && sensors_.headingCentiDegrees.fresh(now) && sensors_.altitudeCm.fresh(now)) {
            const int64_t altitude = int64_t(sensors_.altitudeCm.value) / 100 + 1000;
            if (altitude < 0 || altitude > 65535) continue;
            out[2] = 0x02; payload = 15;
            be32(out+3, uint32_t(sensors_.latitudeE7.value));
            be32(out+7, uint32_t(sensors_.longitudeE7.value));
            be16(out+11, sensors_.speedDeciKmh.value);
            be16(out+13, sensors_.headingCentiDegrees.value);
            be16(out+15, uint16_t(altitude));
            out[17] = 0; // S.Port GPS does not provide satellite count/fix status.
        } else if (kind == 1 && sensors_.motorTemperatureC.fresh(now)) {
            out[2] = 0x0d; payload = 3; out[3] = 0;
            be16(out+4, uint16_t(sensors_.motorTemperatureC.value * 10));
        } else if (kind == 2) {
            uint16_t total, minimum;
            if (!sensors_.pack(now, total, minimum)) continue;
            out[2] = 0x0e; payload = 9; out[3] = 0;
            for (unsigned i=0;i<4;++i) be16(out+4+i*2, sensors_.cellsMv[i].value);
        } else if (kind == 3 && sensors_.fasCurrentDeciA.fresh(now)) {
            out[2] = 0x08; payload = 9;
            be16(out+3, uint16_t(sensors_.fasVoltageCentiV.fresh(now) ? (sensors_.fasVoltageCentiV.value + 5) / 10 : 0));
            be16(out+5, uint16_t(sensors_.fasCurrentDeciA.value));
            out[7]=out[8]=out[9]=out[10]=0; // Capacity/remaining unavailable, not measured.
            out[11]=1; // Explicit battery ID; the receiver's own supply remains ID 0.
        } else if (kind == 4 && sensors_.fasVoltageCentiV.fresh(now)) {
            out[2] = 0x0e; payload = 3; out[3] = 128; // general voltage group, not cells
            be16(out+4, uint16_t(sensors_.fasVoltageCentiV.value * 10));
        } else continue;
        out[0] = 0xea; out[1] = uint8_t(payload + 2);
        uint8_t crc = 0;
        for (size_t i=2;i<payload+3;++i) {
            crc ^= out[i];
            for (unsigned bit=0;bit<8;++bit) crc = uint8_t((crc<<1)^((crc&0x80)?0xd5:0));
        }
        out[payload+3] = crc;
        sent_ = true; sentMs_ = now;
        return payload+4;
    }
    return 0;
}
}
