#if defined(TARGET_RX) && defined(PLATFORM_ESP8266) && defined(ER4_SPORT_TRIAL)
#include "SerialSportER4.h"
#include "CRSFRouter.h"
#include "common.h"
#include "esp8266_peri.h"
#ifndef SPORT_CELLS_PHYSICAL_ID
#define SPORT_CELLS_PHYSICAL_ID 0xff
#endif
#ifndef SPORT_CELLS_APP_ID
#define SPORT_CELLS_APP_ID 0xffff
#endif
#ifndef SPORT_TEMP_PHYSICAL_ID
#define SPORT_TEMP_PHYSICAL_ID 0xff
#endif
#ifndef SPORT_TEMP_APP_ID
#define SPORT_TEMP_APP_ID 0xffff
#endif
#ifndef SPORT_GPS_PHYSICAL_ID
#define SPORT_GPS_PHYSICAL_ID 0xff
#endif
#ifndef SPORT_GPS_INSTANCE
#define SPORT_GPS_INSTANCE 0xff
#endif
onboard::SportCapture er4SportCapture;
SerialSportER4::SerialSportER4(Stream &out, Stream &in)
    : SerialIO(&out, &in), input_(in), telemetry_({
        {SPORT_CELLS_PHYSICAL_ID, SPORT_CELLS_APP_ID},
        {SPORT_TEMP_PHYSICAL_ID, SPORT_TEMP_APP_ID},
        SPORT_GPS_PHYSICAL_ID, SPORT_GPS_INSTANCE}) { receiveMode(); er4SportCapture.active = true; }
void SerialSportER4::receiveMode() {
    pinMode(1, INPUT); // Remove UART0 TX from GPIO1; no GPIO drive/pull-up.
    pinMode(3, FUNCTION_0); // UART0 RX, clear Arduino's ordinary RX pull-up.
}
SerialSportER4::~SerialSportER4() { pinMode(1, INPUT); er4SportCapture.active = false; }
void SerialSportER4::processBytes(uint8_t *bytes, uint16_t size) {
    if (connectionState >= wifiUpdate) return;
    for (uint16_t i=0; i<size; ++i) {
        bus_.feed(bytes[i], millis());
        onboard::SportSample sample;
        if (bus_.takeSample(sample)) {
            er4SportCapture.accept(sample, millis());
            telemetry_.accept(sample, millis());
        }
    }
    er4SportCapture.checksumErrors = bus_.checksumErrors();
}
void SerialSportER4::sendQueuedData(uint32_t maxBytesToSend) {
    if (connectionState >= wifiUpdate) {
        pinMode(1, INPUT);
        er4SportCapture.active = false;
        return; // Retain capture for WiFi inspection; don't drive sensors in WiFi.
    }
    // Two-byte writes cannot block on a full FIFO: dedicated UART and check
    // room before asking the core to begin a poll. Never flush or busy-wait.
    const bool fifoEmpty = ((USS(0) >> USTXC) & 0xff) == 0;
    if (!sending_ && !fifoEmpty) return;
    const bool txDone = sending_ && drain_.ready(micros(), fifoEmpty);
    const auto action = bus_.tick(micros(), txDone);
    if (action == onboard::SportBus::Action::Poll) {
        pinMode(1, FUNCTION_0); // Restore inverted hardware UART TX.
        _outputPort->write(uint8_t(0x7e));
        _outputPort->write(bus_.physicalId());
        drain_.begin(); sending_ = true; ++er4SportCapture.polls;
    } else if (action == onboard::SportBus::Action::Release) {
        for (unsigned i=0; i<32 && input_.available(); ++i) input_.read();
        receiveMode(); sending_ = false;
    }
    (void)maxBytesToSend;
    if (connectionState != connected) return;
    alignas(4) uint8_t frame[19];
    const size_t length = telemetry_.nextFrame(millis(), frame, sizeof(frame));
    if (length) crsfRouter.deliverMessageTo(CRSF_ADDRESS_RADIO_TRANSMITTER,
        reinterpret_cast<crsf_header_t *>(frame));
}
#endif
