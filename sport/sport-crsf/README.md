# Shared S.Port decoding

Canonical production source is `src/src/rx-serial/sport_crsf.cpp` and
`vehicle_sensors.cpp`, with declarations in `src/include`. Both receiver
adapters share these decoders. The VFAS/current BatteryBridge remains a
host-test fixture and is not used for the selected GPS/temperature/cell setup.

Run `make -C sport/sport-crsf test` from the fork root, or `make -C sport test`
for all receiver suites. Tests exercise escaping/checksums, malformed packets,
explicit source selection, units, complete fresh 4S scans, timer rollover and
transmitter parser compatibility. The test-only parser is in
`sport/compatibility/jr-r1`; no transmitter checkout is required.

The old C3 wiring document is historical; neither receiver driver uses those
C3 pin assignments. See the receiver-specific guides for current plans.
