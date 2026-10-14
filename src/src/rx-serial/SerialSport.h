#pragma once
#if defined(TARGET_RX) && defined(PLATFORM_ESP32)
#include "SerialIO.h"
#include "device.h"
#include "SportReceiver.h"
class SerialSport final : public SerialIO {
public:
    SerialSport(Stream &out, Stream &in, int8_t pin);
    ~SerialSport() override;
    uint32_t sendRCFrame(bool, bool, uint32_t *) override { return DURATION_IMMEDIATELY; }
    void sendQueuedData(uint32_t maxBytesToSend) override;
protected:
    void processBytes(uint8_t *bytes, uint16_t size) override;
    int getMaxSerialReadSize() override { return 32; }
private:
    Stream &input_;
    int8_t pin_;
    onboard::SportBus bus_;
    onboard::SportTelemetry telemetry_;
    void receiveMode();
};
#endif
