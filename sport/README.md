# Receiver-integrated S.Port telemetry

This project is maintained in [cmlpreston/ExpressLRS-SPort](https://github.com/cmlpreston/ExpressLRS-SPort),
on branch `codex/sport-telemetry`, based on upstream ExpressLRS 4.1.0 commit
`a9d4a9cb5b5687c4c9d7e9e7fbdf44ad93651da6`.

Production drivers are in `src/src/rx-serial`; shared declarations are in
`src/include`. There are no source overlays or dependencies on the transmitter
checkout. `compatibility/jr-r1` is a documented, test-only parser snapshot.

| Receiver | Implementation | Current status |
| --- | --- | --- |
| ER4 / ESP8285 | Primary UART0 TX GPIO1, RX GPIO3, software-controlled TX release | Discovery firmware and guarded WiFi upload workflow prepared; not flashed |
| ER6G / ESP32 | Serial2 single-pin UART1, inverted GPIO routing | Build-tested; upload disabled pending hardware review |

Both drivers reuse the same S.Port parser, poller and selected sensor decoder.
GPS/temperature/4S cell encoders suppress stale readings. Receiver voltage is
kept separate; no drive-pack battery 0x08 frame is emitted. Source selection is
unconfigured by default. ER4 exposes discovered raw IDs/values via `/sport/status`
in WiFi after collection. No sensor telemetry is forwarded by default.

## Build and test

```sh
git clone --branch codex/sport-telemetry https://github.com/cmlpreston/ExpressLRS-SPort.git
cd ExpressLRS-SPort
make -C sport test
cd src/html
npm ci
npm run build:sx128x-rx
npm run build:sx128x-rx-8285
cd ../..
pio run -d src -e ER4_ESP8285_2400_RX_SPORT_BUILD
pio run -d src -e ER6G_ESP32_2400_RX_SPORT_BUILD
pio run -d src -e ER4_ESP8285_2400_RX_SPORT_via_WIFI
```

The commands above build only. They do not flash. The two WebUI variants must
be rebuilt before firmware compilation because upstream copies generated headers.
Host suites use address/undefined-behaviour sanitizers. The ER4 adapter test
stages the canonical production driver to simulate UART/pin/router boundaries;
no separate copy of driver implementation is maintained.

## Upload policy

Only ordinary `upload` in `ER4_ESP8285_2400_RX_SPORT_via_WIFI` is enabled.
All build-only, ER6G, forced and other upload targets are rejected by
`src/python/sport_no_upload.py`. The receiver-local preflight checks profile,
4.1.0 ancestry, refreshed origin experiment branch, clean checkout and host tests.
It uses no transmitter repository, absolute workspace paths or separate source
manifest. Source identity comes from the committed fork itself.

An explicit human instruction is still required before an agent uploads firmware.
See [ER4 first-trial procedure](er4-sport/first-trial.md) for the later user-run
command, required configuration and proposed wiring. GPIO logic levels, reset
behaviour, bus turnaround, servo jitter and failsafe still need bench validation.

## Scope and provenance

Migrated from the JR R1 transmitter prototype on 2026-10-14. That repository
continues to own transmitter telemetry display/alarms and its production CRSF
parser. The compatibility snapshot records its origin and is not linked into
receiver binaries. Receiver code, tests, builds and upload checks now live here.

Hardware definitions used for validation are ExpressLRS/targets commit
`51a697b3220f069baf3adf2c5353b710df70bbe0` (ER4 and ER6G profiles). Upstream's
build tooling obtains this separate hardware repository; it is not firmware source.
Recheck hardware changes before any later upload.
