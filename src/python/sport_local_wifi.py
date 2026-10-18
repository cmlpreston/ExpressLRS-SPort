"""Private WiFi defaults for ER4 trial firmware, never compiler flags."""
import json
import os
from pathlib import Path
import stat

ENVS = {'ER4_ESP8285_2400_RX_SPORT_BUILD', 'ER4_ESP8285_2400_RX_SPORT_via_WIFI'}
FILENAME = 'sport-wifi.local.json'

def load_local_wifi(project, environment):
    if environment not in ENVS:
        return {}
    path = Path(project) / FILENAME
    if not path.exists():
        return {}
    if os.name == 'posix' and stat.S_IMODE(path.stat().st_mode) & 0o077:
        raise ValueError('Private WiFi file must be owner-only: chmod 600 src/' + FILENAME)
    try:
        data = json.loads(path.read_text())
    except (ValueError, OSError):
        raise ValueError('Cannot read private WiFi JSON file.') from None
    if not isinstance(data, dict) or set(data) != {'wifi-ssid', 'wifi-password'}:
        raise ValueError('Private WiFi file requires only wifi-ssid and wifi-password.')
    ssid, password = data['wifi-ssid'], data['wifi-password']
    if not isinstance(ssid, str) or not 1 <= len(ssid.encode('utf-8')) <= 32 or '\x00' in ssid:
        raise ValueError('Private WiFi SSID must contain 1 to 32 UTF-8 bytes and no NUL.')
    if not isinstance(password, str) or '\x00' in password:
        raise ValueError('Private WiFi password must be a string with no NUL.')
    size = len(password.encode('utf-8'))
    if not (size == 0 or 8 <= size <= 63 or
            (size == 64 and all(c in '0123456789abcdefABCDEF' for c in password))):
        raise ValueError('Private WiFi password must be empty, 8 to 63 bytes, or a 64-digit hexadecimal key.')
    return data
