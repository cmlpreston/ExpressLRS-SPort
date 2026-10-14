# ER6G S.Port driver

The ESP32 `SerialSport` adapter uses Serial2/UART1 on a configured Serial2 TX
output. It routes RX internally onto the same pin with inversion and switches
TX to input after hardware UART-idle reports final stop-bit completion.

Use `ER6G_ESP32_2400_RX_SPORT_BUILD`, hardware profile
`radiomaster.rx_2400.er6g`. This environment is build-only: uploads are disabled.
No output mapping has been changed or receiver flashed. A free non-strapping
output, verified signal levels/idle behaviour, power sequencing and a bench
capture are required before approving direct wiring. The alternative tri-state
buffer proposal requires a different adapter and is not used by this driver.

See [the project build/test guide](../README.md) and [planning evidence](wiring-plan.md).
The parser, poller and telemetry encoders are shared with ER4. The host suite
feeds generated packets into a frozen transmitter compatibility parser. Source
selection remains unconfigured by default. No drive-pack battery 0x08 packets
are emitted; satellite count is unknown/zero, not invented.
