#pragma once
#include <cstdint>
#include <vector>
struct crsf_header_t { uint8_t address, length, type; };
constexpr uint8_t CRSF_ADDRESS_RADIO_TRANSMITTER = 0xea;
struct Router {
    std::vector<std::vector<uint8_t>> frames;
    void deliverMessageTo(uint8_t address, crsf_header_t *frame) {
        auto bytes=reinterpret_cast<uint8_t*>(frame);
        if(address==0xea) frames.emplace_back(bytes,bytes+frame->length+2);
    }
};
extern Router crsfRouter;
