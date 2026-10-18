# ER4 first trial: flash, configure, then connect sensors

Receiver firmware, tests and upload checks now live entirely in the
[ExpressLRS-SPort fork](https://github.com/cmlpreston/ExpressLRS-SPort), branch
`codex/sport-telemetry`. Nothing has been uploaded or wired by this project.

## Flash with sensors disconnected

1. Disconnect all S.Port signal wiring and servos from the spare ER4. Power the
   receiver alone from its compatible supply. Do not use an ESC/motor for this trial.
2. Turn the transmitter module off so the receiver can enter auto-WiFi. Wait for
   the receiver's configured timeout (normally 60 seconds). Connect the computer
   to `ExpressLRS RX`, password `expresslrs`, then open `http://10.0.0.1/`.
   If previously configured to join home WiFi, use its assigned address instead.
3. Verify the WebUI identifies **RadioMaster ER4**, and record installed firmware,
   regulatory domain, binding/model settings, PWM mapping and failsafe values.
   Export the configuration if offered and retain a stock ER4 recovery image.
   Confirm the transmitter module has compatible ELRS 4.x firmware for later RC tests.
4. Use the dedicated PlatformIO WiFi upload environment. It runs the fork's
   `src/python/sport_no_upload.py` and `src/python/sport_preflight.py` checks; it fetches origin/codex/sport-telemetry, checks its ancestry and the
   selected 4.1.0 base, verifies the hardware profile and clean committed source, and runs host tests before the normal uploader.
   Do not use WebUI firmware upload to bypass these checks, force-upload targets,
   the S3 transmitter environment or direct esptool commands.

From the fork repository root, after build/test and explicit authorization:

```sh
pio run -d src -e ER4_ESP8285_2400_RX_SPORT_via_WIFI \
  -t upload --upload-port 10.0.0.1
```

This command **writes receiver firmware**. Run it only when ready for the trial.
The reviewed configuration is the receiver fork branch printed by preflight,
ExpressLRS 4.1.0 commit `a9d4a9cb`, ER4/ESP8285 profile, and no display. If the
preflight rejects ancestry/source/profile checks, resolve that reason before
uploading; do not activate an unsafe override. A hardware mismatch must never
be force-accepted. Keep power connected through upload completion and reboot.

Internet access is needed for the receiver repository fetch; the RX hotspot may
not provide internet. A computer with a second connection, or putting the ER4
on the same home network as the computer, may be necessary. With home WiFi,
replace `10.0.0.1` in the command with the verified receiver address.

## Configure before attaching telemetry

Return to the flashed receiver WebUI with the sensor harness still disconnected.
Existing settings are preserved, so do not assume the default mapping was applied.

| Physical output | Mode | Input channel |
| --- | --- | --- |
| 1 | 50Hz servo PWM | 1, steering |
| 2 | Primary Serial TX | — |
| 3 | Primary Serial RX | — |
| 4 | 50Hz servo PWM | 2, throttle bench servo |

Choose **S.Port Telemetry** as the primary serial protocol. Keep AirPort disabled.
On upstream PWM receivers assigning one primary serial pin also assigns its
partner. Save and reboot, then verify both outputs show serial roles and output
4 is mapped to input channel 2. Retain/review failsafe values for the two servos.
The discovery build defaults centre servos only on fresh/reset configuration;
do not reset binding/settings just to get that mapping.

## Direct trial harness: conditional on verified signal levels

Independent development evidence points to normal S.Port data levels around
3.3V. It does not provide a guaranteed maximum for this exact sensor chain.
Before attaching it to the ER4, measure the working X4R S.Port signal relative
to ground with a scope, including both polls and sensor replies. Direct wiring
is not suitable if either drive level exceeds the receiver's GPIO limits.
Also verify S.Port power and each sensor's full-charge supply rating.

For the proposed direct harness:

```text
ER4 output 2 SIGNAL ----+
                        +---- S.Port SIGNAL of the daisy-chained sensors
ER4 output 3 SIGNAL ----+
ER4 GND --------------------- sensor-chain GND
Compatible sensor supply ---- sensor-chain V+
```

Identify connector pins by board markings, not cable colour alone. The positive
receiver rail is the pack/BEC voltage, not a 3.3V logic rail. Retain the known
compatible sensor power arrangement once verified; don't join independent supply
positives. Remove the X4R's S.Port signal connection so only the ER4 polls.
A LiFe-compatible receiver does not establish that every servo accepts full pack
voltage; use servos/supply appropriate to their ratings.

For the first powered test, leave the sensor SIGNAL disconnected while the ER4
boots, then attach it after application startup and verified serial configuration.
GPIO1 is also the ROM UART TX and can drive bootloader traffic before our code
releases it. Observe boot/reset and turnaround on a scope before treating a
permanently connected harness as validated. Disconnect the harness before
subsequent flashing/recovery.

## Capture and inspect

With transmitter module off, let the receiver poll the powered sensor chain.
It then enters auto-WiFi; polling stops but captured data remain until power loss.
Connect to receiver WiFi **without power-cycling** and open:

`http://10.0.0.1/sport/status` (or the receiver's home-network address).

Expected: increasing/nonzero `polls`, nonzero `samples`, entries in `readings`,
and few/no `checksum_errors`. `driver_active` is false in WiFi because the
adapter has released TX and stopped polling. JSON IDs and raw readings are
shown in decimal. Save the JSON so we can choose the GPS, temperature and cell
sources explicitly for the next firmware build.

If `polls` is zero, review protocol and output modes. If polls are nonzero but
samples are zero, check the supply, signal/ground, idle level, poll waveform and
reply turnaround. Record errors and waveforms before changing interface wiring.
The firmware is source-discovery only by default: no sensor telemetry will yet
appear on the transmitter. The first useful result is this ID/value capture.

Reference: [ExpressLRS WiFi updating](https://www.expresslrs.org/software/updating/wifi-updating/).

## Diagnostic capture version 2

The status now includes `diagnostics_version: 2`. `rx_bytes` counts all UART
bytes read by the adapter, including local TX echo; it is not a sensor-reply
count. `tx_phase_bytes` counts bytes consumed while the driver still owns TX,
and `release_discarded_bytes` is the subset cleared at handover.
`reply_phase_bytes` counts bytes read after TX release.

`max_release_us` is the longest observed poll-start-to-release time, including
loop scheduling delay (a two-byte poll itself takes about 347 us at 57600 baud).
`wire_observations` retains per-ID totals and the first 16 bytes of the last
nonempty TX/reply observation for that physical ID, as decimal byte arrays.
These arrays survive subsequent silent polls. Ordinary poll-echo-only IDs
are omitted from JSON, and at most eight diagnostic rows are returned to bound
WiFi memory use; `wire_observations_truncated` reports additional eligible IDs. They may be partial, corrupt,
local echo, or sensor data; they are not validated frames.
`first_reply_read_us` is when software first *read* bytes relative to that poll,
not a scope measurement of their arrival time. The TX and reply snapshots can
come from different polls; use their totals and contents as diagnostics only.

This revision adds observation only; it does not change inversion, baud rate,
poll cadence, TX release timing, or sensor selection. Repeat with the cell
monitor alone and its battery connected. Save `/sport/status` after auto-WiFi
without power-cycling, as before. No automatic flash is performed.

## Diagnostic capture version 3: leading handover zeros

The disconnected ER4 baseline showed a zero byte after almost every poll's TX
release. With the cell monitor attached, ID 0xa1 returned
`00 10 00 03 04 00 00 00 e8 00`. The middle eight bytes are a checksum-valid
0x0300 data reply. Treating the initial zero as its header misaligned the parser.
Version 3 ignores only zeros before the start of a reply. It preserves zeros
inside the packet and still checks the normal checksum and frame type.
`ignored_prefix_zeros` counts these discarded prefix bytes; raw diagnostics
retain them. This handles the observed UART artifact without establishing its
electrical cause or validating decoded cell voltages. Poll timing is unchanged.
