#pragma once
#if defined(TARGET_RX) && defined(PLATFORM_ESP8266) && defined(ER4_SPORT_TRIAL)
#include "SerialIO.h"
#include "device.h"
#include "SportReceiver.h"
#include "Er4TxDrain.h"
#include "SportCapture.h"
#include "Er4SportSources.h"
extern onboard::SportCapture er4SportCapture;
extern bool er4SportUploadInProgress;
extern uint32_t er4SportCellsQueued;
bool er4SportCellSnapshot(uint32_t now, uint16_t (&cells)[4], uint16_t &total, uint16_t &minimum);
void er4SportBeginUpload();
extern bool er4SportSniffMode;
extern uint32_t er4SportObservedPolls;
bool er4SportEnableSniffer();
class SerialSportER4 final : public SerialIO {
public:
    SerialSportER4(Stream &out, Stream &in);
    ~SerialSportER4() override;
    uint32_t sendRCFrame(bool, bool, uint32_t *) override { return DURATION_IMMEDIATELY; }
    void sendQueuedData(uint32_t maxBytesToSend) override;
    void enableSniffer();
    const onboard::VehicleSensors &sensors() const { return telemetry_.sensors(); }
protected:
    int getMaxSerialReadSize() override { return 32; }
    void processBytes(uint8_t *bytes, uint16_t size) override;
private:
    Stream &input_;
    onboard::SportBus bus_;
    onboard::Er4TxDrain drain_;
    onboard::SportParser sniffer_;
    bool awaitingId_ = false;
    onboard::SportTelemetry telemetry_;
    bool sending_ = false;
    void receiveMode();
};
#endif
