#pragma once
// ER6-G target: Vario 2400.json. Trial uses fitted output 5 / GPIO2 as a
// single-wire secondary-UART S.Port bus; do not take a separate PWM RX pin.
template<class Pin>
bool er6gSportPinAllowed(const Pin *outputs, unsigned count, int tx, int rx) {
    const int expected[] = {14, 1, 3, 15, 2, 4};
    if(count != 6 || tx != 2 || (rx != -1 && rx != 2)) return false;
    for(unsigned i=0;i<6;++i) if(outputs[i] != expected[i]) return false;
    return true;
}
