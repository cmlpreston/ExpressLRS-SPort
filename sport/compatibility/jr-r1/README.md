Test-only snapshot from cmlpreston/jr-r1-native-crsf, HEAD 18260e38a08c17558f7d6f6db663817205f7ba43, copied 2026-10-14. This snapshot verifies packet compatibility and is not linked into receiver firmware. The transmitter repository remains authoritative for its parser.

Updated 2026-10-18 with the matching local transmitter parser changes for source-0 four-cell 0x0e voltage groups (millivolts), atomic validation and 3-second freshness. These transmitter changes have host tests but are not yet flashed to the transmitter. Protocol reference: https://github.com/tbs-fpv/tbs-crsf-spec/blob/main/crsf.md#0x0e-voltages-or-voltage-group

Updated 2026-10-26 from the matching local transmitter parser for FAS battery ID 1, general-voltage group 128, and independent freshness. The snapshot also includes the authoritative 1–12S cell parser. This remains test-only code.
