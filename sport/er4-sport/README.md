# ER4 S.Port bench firmware — ExpressLRS 4.1.0

Prepared for the spare RadioMaster ER4 (ESP8285), two bench servos and a 2S
LiFe receiver pack. This is experimental bench firmware, not vehicle-ready firmware.
The ER4 has completed active-poll and passive-X4R comparison trials.

For the next stage see [the flash/configuration/wiring procedure](first-trial.md).
The dedicated WiFi environment runs receiver-local preflight; build-only
environments refuse uploads. Uploads require explicit authorization for the receiver.

## Prepared behaviour

- Adds primary `S.Port Telemetry` protocol number 10 without renumbering the
  existing serial protocols. The WebUI keeps AirPort as its last option.
- Uses UART0 at inverted 57600 8N1 on output 2/GPIO1 TX and output 3/GPIO3 RX.
- Between polls GPIO1 is an input/high impedance. RX remains hardware UART on
  GPIO3 with its ordinary pull-up removed. Both signals join the same S.Port wire.
- Uses the shared S.Port parser/poller and CRSF GPS/temperature/cell encoders.
- ESP8285 FIFO-empty does not prove the final stop bit has left the UART. The
  driver waits another 180 microseconds after observing empty without spinning,
  then releases TX. This conservative scheduling strategy must be measured on
  hardware for sensor reply timing and contention before any vehicle use.
- Records up to 32 distinct physical/application-ID pairs with raw values,
  timestamps, counts, poll count and checksum errors. The read-only WiFi endpoint
  `/sport/status` exposes that current-boot capture.
- WiFi mode continues live polling/capture but stops the RC radio link. Upload
  start immediately releases TX and latches polling off until reboot.
- The ER4 environments select the verified FLVS ADV source: physical ID `0xa1`,
  application ID `0x0300`, exactly four cells. Complete fresh scans are encoded
  as CRSF `0x0e`, source 0, four big-endian millivolt readings, at most 5 Hz.
  Frames enter the upstream telemetry router only while RF is connected. No
  drive-pack `0x08` frame replaces the receiver's own supply-voltage telemetry.
  GPS and temperature remain unselected pending their separate verification.
- `/sport/status` version 9 reports `cell_forwarding_enabled`, selected IDs,
  `cells_valid`, `cells_mv`, `pack_mv`, `minimum_cell_mv`, `crsf_cells_queued`
  and `rf_forwarding_active`. Queue count is not an RF delivery acknowledgement.
  Partial scans and readings older than 3 seconds are not presented as valid.
- Passive `/sport/sniff` remains receive-only for that boot. Reboot restores
  ER4 polling. The X4R signal must be disconnected before ER4 polling resumes.
- Retains upstream servo control, binding and RF/failsafe machinery. The serial
  adapter emits no RC channel frames. Firmware rejects S.Port activation if the
  receiver pin layout is not ER4 or outputs 2/3 are still owned by PWM.

## Channel arrangement

| Physical output | GPIO | Mode | RF input channel |
| --- | --- | --- | --- |
| 1 | 0 | 50Hz servo PWM, steering | 1 |
| 2 | 1 | Primary Serial TX, S.Port | — |
| 3 | 3 | Primary Serial RX, S.Port | — |
| 4 | 9 | 50Hz servo PWM, throttle bench servo | 2 |

These are defaults **only when upstream initializes or resets configuration**.
An existing receiver configuration and binding data are not silently overwritten
by this firmware. For a later authorized trial, inspect the receiver Model page:
set outputs 2/3 to primary Serial, choose S.Port Telemetry, and map output 4 to
input channel 2. Keep output 1 on input channel 1. Reboot after configuration.
Do not reset the receiver merely to get these defaults; a reset also affects
binding/settings. The defaults centre the two bench servos at 1500us on failsafe.
Review the actual throttle-neutral/failsafe requirement separately before an ESC
replaces the second servo; a centre pulse is not appropriate for every ESC.

## Proposed trial connection, not yet electrically validated

After verifying signal levels of the working FrSky sensor chain and ensuring
the custom firmware's serial modes own outputs 2/3:

```text
ER4 output 2 SIGNAL (GPIO1 TX) --+
                                +--> sensor-chain S.Port SIGNAL
ER4 output 3 SIGNAL (GPIO3 RX) --+
ER4 GND ----------------------------> sensor-chain GND
Verified compatible supply ---------> sensor-chain V+
```

Use only the signal/ground pins for the UART harness; do not mistake the
receiver-positive pins for 3.3V. Sensor supply may be the existing LiFe rail if
all actual sensor models support its full-charge voltage. Servo supply must
match the servo ratings. The BSS138 module and three-pin ER6G buffer circuit
are not used in this proposed direct arrangement.

Only one receiver polls the bus: disconnect the X4R signal from the chain before
any eventual ER4 trial. Do not tie the two receivers' power supplies together.
Disconnect the sensor harness during eventual flashing/recovery: GPIO1 is also
ROM UART TX and can emit bootloader traffic before application code releases it.
With stock firmware or incorrect PWM settings, outputs 2/3 must not be joined
to a sensor bus. Actual signal voltage, idle bias, reset behaviour and reply
turnaround remain bench checks. The first trial uses servos only, no propulsion.

## First trial's observable result

No binding is needed to collect sensor replies: normal disconnected operation
polls both before and during WiFi. After an authorized upload and configuration,
let it collect with sensors connected, then enter receiver WiFi using the normal
ELRS workflow. If no RF link is established the upstream default auto-WiFi is
60 seconds. Open `/sport/status` on the receiver's normal WebUI address.

Inspect `polls`, `samples`, `checksum_errors`, `dropped_keys`, and `readings`.
IDs/raw values are decimal in JSON. Zero samples does not establish a software
bug: check the selected protocol/output modes, waveform, signal cable and supply.
The cell source is now selected from captures. Other sources require separate
verification before enabling them. Do not use guessed factory IDs.
The snapshot is RAM-only and disappears after a power cycle. `driver_active`
remains true in WiFi while the driver is collecting; RF forwarding is inactive.

Build-time selectors for subsequent builds:
`SPORT_CELLS_PHYSICAL_ID`, `SPORT_CELLS_APP_ID`,
`SPORT_TEMP_PHYSICAL_ID`, `SPORT_TEMP_APP_ID`,
`SPORT_GPS_PHYSICAL_ID`, `SPORT_GPS_INSTANCE`.
Physical ID is the parity-encoded byte observed on the wire.

## Reproduction and validation

Use [the fork build guide](../README.md). All production sources are canonical
in `src/src/rx-serial` and `src/include`; no overlay installation is necessary.
The ER4 adapter/defaults tests, shared parser/encoder tests, WebUI checks and
ESP8285 builds passed before migration. Re-run fork-local tests and builds for
new changes. A build does not establish electrical or radio timing compatibility.

The build-only environment rejects uploads. The receiver-local WiFi preflight
requires the reviewed fork branch and a clean checkout; it does not modify
existing receiver settings. See [first trial](first-trial.md).

## Four-cell forwarding validation (2026-10-18)

Both X4R sniffing and ER4 polling observed `0x7d77ca40` and `0x7e07d342`
from `0xa1/0x0300`: 3988/4014/4006/4032 mV, total 16040 mV, minimum 3988 mV.
These payloads contain escaped bytes and are now regression fixtures through
S.Port parsing, cell assembly, CRSF encoding and the matching transmitter parser.
The transmitter parser changes in the separate transmitter checkout are tested
but have not been flashed. No transmitter UI cell display is added by this build.
On-air reception remains a separate bench check after leaving WiFi mode.

The CRSF voltage-group format follows the
[TBS specification](https://github.com/tbs-fpv/tbs-crsf-spec/blob/main/crsf.md#0x0e-voltages-or-voltage-group).
