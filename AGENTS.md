# S.Port receiver experiment

This is receiver firmware based on ExpressLRS 4.1.0. No firmware upload is
permitted unless the user explicitly requests an upload to an identified receiver.
Preparing or building firmware is not authorization to flash it.

Keep production S.Port sources in src/src/rx-serial and src/include. Host tests
must exercise these canonical sources; do not maintain duplicate overlays.
Use `make -C sport test` and build the selected hardware environment after edits.

Use only PlatformIO upload targets so python/sport_no_upload.py and
python/sport_preflight.py run. Never invoke esptool directly or use unsafe/force
upload overrides. The reviewed ER4 WiFi environment is the only enabled upload
path. ER6G and all build-only environments refuse upload. Keep upstream radio,
PWM, binding and failsafe behaviour intact unless explicitly reviewed.

Before an authorized ER4 upload, verify connected receiver model, supply, RF
firmware compatibility, output mapping and recovery access. Disconnect telemetry
and servo wiring during upload. The preflight refreshes origin/codex/sport-telemetry,
requires its ancestry and the pinned 4.1.0 base, a clean checkout and passing host
tests. Do not switch to older hardware experiments or apply old stashes.
