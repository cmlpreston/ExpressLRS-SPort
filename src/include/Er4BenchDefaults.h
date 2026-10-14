#pragma once
// Only invoked when upstream initializes/resets a configuration. Existing
// EEPROM mapping and binding data must not be silently changed after flashing.
template<class Config>
void applyEr4BenchDefaults(Config &config) {
    constexpr uint16_t centre = US_CHANNEL_VALUE_CENTER - US_CHANNEL_VALUE_MIN;
    config.SetSerialProtocol(PROTOCOL_SPORT);
    config.SetPwmChannel(0, centre, 0, false, som50Hz, false);
    config.SetPwmChannel(1, centre, 1, false, somSerial, false);
    config.SetPwmChannel(2, centre, 2, false, somSerial, false);
    config.SetPwmChannel(3, centre, 1, false, som50Hz, false);
}
