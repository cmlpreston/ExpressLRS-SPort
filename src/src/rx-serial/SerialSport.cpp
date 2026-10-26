#if defined(TARGET_RX) && defined(PLATFORM_ESP32)
#include "SerialSport.h"
#include "CRSFRouter.h"
#include "common.h"
#include "hal/uart_ll.h"
#include "soc/uart_struct.h"
#include "SportSources.h"
SerialSport::SerialSport(Stream &out, Stream &in, int8_t pin)
    : SerialIO(&out, &in), input_(in), pin_(pin), telemetry_({
        {SPORT_CELLS_PHYSICAL_ID, SPORT_CELLS_APP_ID},
        {SPORT_TEMP_PHYSICAL_ID, SPORT_TEMP_APP_ID},
        SPORT_GPS_PHYSICAL_ID, SPORT_GPS_INSTANCE},
        {SPORT_FAS_PHYSICAL_ID,0x0210}, {SPORT_FAS_PHYSICAL_ID,0x0200}) { receiveMode(); }
void SerialSport::receiveMode() {
    if (pin_ == UNDEF_PIN) return;
    pinMatrixOutDetach(pin_, false, false);
    pinMode(pin_, INPUT); // External interface establishes idle-low bias.
    pinMatrixInAttach(pin_, U1RXD_IN_IDX, true);
}
SerialSport::~SerialSport() { receiveMode(); }
void SerialSport::processBytes(uint8_t *bytes, uint16_t size) {
    if (connectionState >= MODE_STATES) return;
    for (uint16_t i=0; i<size; ++i) {
        bus_.feed(bytes[i], millis());
        onboard::SportSample sample;
        if (bus_.takeSample(sample)) telemetry_.accept(sample, millis());
    }
}
void SerialSport::sendQueuedData(uint32_t maxBytesToSend) {
    if (pin_ == UNDEF_PIN) return;
    if (connectionState >= MODE_STATES) { receiveMode(); return; }
    // Only UART1/Serial2 is supported. Inspect the hardware FSM, never flush or wait.
    const auto action = bus_.tick(micros(), uart_ll_is_tx_idle(&UART1));
    if (action == onboard::SportBus::Action::Poll) {
        pinMode(pin_, OUTPUT);
        digitalWrite(pin_, LOW);
        pinMatrixOutAttach(pin_, U1TXD_OUT_IDX, true, false);
        _outputPort->write(uint8_t(0x7e));
        _outputPort->write(bus_.physicalId());
    } else if (action == onboard::SportBus::Action::Release) {
        // RX can observe local TX echo. Drain it before releasing the bus;
        // sensor replies cannot start while we still drive the line.
        for (unsigned i=0; i<32 && input_.available(); ++i) input_.read();
        receiveMode();
    }
    (void)maxBytesToSend;
    // Keep sensor collection running without a link; don't fill RF queues.
    if (connectionState != connected) return;
    alignas(4) uint8_t frame[19];
    const size_t length = telemetry_.nextFrame(millis(), frame, sizeof(frame));
    if (length) crsfRouter.deliverMessageTo(CRSF_ADDRESS_RADIO_TRANSMITTER,
        reinterpret_cast<crsf_header_t *>(frame));
}
#endif
