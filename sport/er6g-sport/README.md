# ER6-G S.Port trial — three servo outputs plus one sensor signal

The environment `ER6G_ESP32_2400_RX_SPORT_BUILD` targets
`radiomaster.rx_2400.er6g` on ExpressLRS 4.1.0. The build-only target refuses uploads. The reviewed
`ER6G_ESP32_2400_RX_SPORT_via_WIFI` target permits ordinary guarded WiFi uploads
only after an explicit request identifying the receiver.

## Single-wire connection

Unlike the ER4's two-pin UART, ESP32 GPIO routing lets `SerialSport` transmit and
receive on one output. Arduino `Serial1` (hardware UART1, secondary serial in the
WebUI) runs at inverted 57600 baud, 8N1. The driver releases the signal after the
hardware reports the final stop bit has completed, then listens for the sensor.
No external TX/RX junction or direction-control pin is needed.

Trial mapping (connector numbers are not RF channel numbers; preserve the existing PWM settings on outputs 1–4):

| Physical output | GPIO | Mode | RF channel / purpose |
| --- | --- | --- | --- |
| 1 | 14 | 50Hz PWM | CH1 steering servo / gyro steering input |
| 2 | 1 | Existing PWM | CH2 throttle; UART0 boot output |
| 3 | 3 | 50Hz PWM | CH3 gyro gain |
| 4 | 15 | Existing PWM | CH4 spare |
| 5 | 2 | Serial2 TX (secondary serial) | S.Port shared TX/RX signal; strapping pin |
| 6 (optional header) | 4 | PWM, unused | CH6; no connector needed for this trial |

CH6 needs an optional soldered three-pin connector, as shown in the
[RadioMaster manual](https://cdn.shopify.com/s/files/1/0609/8324/7079/files/ER6G_User_Manual.pdf?v=1722224113).
This trial uses the fitted CH5 header instead. When moving from the earlier CH6
trial, change CH5 to Serial2 TX and restore CH6 to PWM, preserving their other
settings. GPIO2 is a boot-strapping pin: successful operation and power-up with
the actual sensor chain remain hardware checks.

The WebUI calls the secondary port **Serial2**; its C++ object is `Serial1`.
Select **S.Port Telemetry** for the secondary serial protocol. Leave secondary
RX unassigned: the driver routes RX internally to output 5. Leave AirPort off.
The trial refuses S.Port activation on the wrong target, another TX output or a
separately assigned secondary RX. Existing PWM mapping, binding and failsafe
settings are not changed on flashing; review and configure them explicitly.
This profile does not set new servo/failsafe defaults.

```text
ER6-G output 5 SIGNAL -------- S.Port sensor-chain SIGNAL
ER6-G GND ------------------- S.Port sensor-chain GND
Compatible sensor supply ---- S.Port sensor-chain VCC
```

Verify the actual sensor-chain logic levels are compatible with 3.3V GPIO and
its supply supports the pack's full-charge voltage. Do not join independently
powered positive rails. The ER6-G is the only bus master: disconnect the X4R or
ER4 S.Port signal. Reset/release waveforms, RF forwarding, PWM jitter and failsafe
remain hardware checks. Keep propulsion disconnected for the first trial.

## Forwarded readings

The environment selects the verified FLVS ADV source (`0xa1` / `0x0300`),
using the shared complete-fresh-4S decoder and CRSF `0x0e` encoder at up to 5Hz.
Receiver supply voltage remains separate (battery ID 0); FAS current uses
explicit battery ID 1 as described below.
GPS remains unselected. FAS300 ADV temperature/current/voltage forwarding is enabled for bench comparison (see below). Receiver
support beyond 4S is still future work even though the R-1 now accepts 1–12S.

The ESP32 adapter does not currently expose the ER4-specific `/sport/status`
discovery endpoint or its live-WiFi capture. Establish collection and delivery
using the R-1 telemetry diagnostics and a logic-analyser capture. Do not assume
that queueing a CRSF frame proves on-air reception.

## Build and test

From the receiver fork root:

```sh
make -C sport test
.venv/bin/pio run -d src -e ER6G_ESP32_2400_RX_SPORT_BUILD
```

The ESP32 WebUI headers must already be generated as described in
[the fork guide](../README.md). The WiFi target requires live ER6-G identity/pin/domain verification, a clean
reviewed branch, passing host tests, and owner-only private home WiFi defaults.
Use PlatformIO `-e ER6G_ESP32_2400_RX_SPORT_via_WIFI -t upload --upload-port HOST`;
do not use force/confirm overrides. It retains the stock unified ESP32 wire
target identity so the receiver can validate normal firmware updates. Historical alternative
wiring proposals in [wiring-plan.md](wiring-plan.md) are not this profile.

## FAS300 ADV forwarding (2026-10-26)

Both trial profiles select captured physical ID `0x22`: current `0x0200`,
voltage `0x0210`, and temperature `0x0400`. Other physical/application IDs are
ignored. Each receiver-side reading expires independently after 3 seconds; zero is retained.
At the transmitter voltage/current expire after 3 seconds, while the existing
Temperature card retains its 10-second freshness policy.

- Temperature: standard CRSF `0x0d`, source 0, tenths of a degree C.
- Current: standard CRSF battery `0x08` with explicit battery ID 1, tenths of an
  amp. ID 0 remains receiver supply; the OTA queue preserves both IDs. Capacity
  and remaining are unavailable and zero-filled; the R-1 does not display them.
  Current forwarding does not depend on a voltage reading. If voltage is absent,
  that frame's required voltage field is zero; R-1 ignores it for FAS voltage.
- Voltage: CRSF general-voltage group `0x0e`, source 128, one millivolt value.
  S.Port hundredths of a volt are multiplied by 10 without losing precision.

Captured current `11559` is deliberately forwarded (standard interpretation:
1155.9 A), despite exceeding the FAS300's 300 A range. The R-1 marks it **Check
range** so it can be compared against the FrSky display; it must not be assumed
calibrated. Capture `0x0210=0` remains a visible zero, and `0x0400=24` becomes
24.0 C. Warm the probe by hand to check temperature forwarding without a motor.
Voltage requires the sensor's voltage-sense connection; current requires its
measurement path to be wired and checked. Do not infer these from the S.Port
power connection alone.

Matching R-1 firmware adds **Data → Sensors → FAS300 ADV 3 / 3**, FAS voltage
and current widget sources, and `@FAS` output from the USB `TELEMETRY` command.
The existing Temperature sensor displays the selected FAS probe. ER4 WiFi
`/sport/status` version 10 reports raw hundredths-of-volt/tenths-of-amp values
and freshness. ER6G uses the transmitter diagnostics.

References: [OpenTX S.Port scales](https://github.com/opentx/opentx/blob/2.3/radio/src/telemetry/frsky_sport.cpp),
[TBS CRSF format](https://github.com/tbs-fpv/tbs-crsf-spec/blob/main/crsf.md).
These source changes and build outputs do not establish that receiver or
transmitter hardware has been updated; record an authorized upload separately.
