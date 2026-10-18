from pathlib import Path
import json
import runpy
import tempfile
import unittest

wifi = runpy.run_path(str(Path(__file__).resolve().parents[1] / 'src/python/sport_local_wifi.py'))

class Tests(unittest.TestCase):
    def test_local_defaults(self):
        with tempfile.TemporaryDirectory() as directory:
            load = wifi['load_local_wifi']
            env = 'ER4_ESP8285_2400_RX_SPORT_via_WIFI'
            self.assertEqual(load(directory, env), {})
            p = Path(directory) / wifi['FILENAME']
            valid = {'wifi-ssid': 'Bench "network"', 'wifi-password': 'example\\password'}
            p.write_text(json.dumps(valid))
            p.chmod(0o600)
            self.assertEqual(load(directory, env), valid)
            self.assertEqual(load(directory, 'Unified_ESP32_2400_TX_via_WIFI'), {})
            for invalid in [[], {'wifi-ssid': 'x'}, {**valid, 'uid': [1]},
                            {**valid, 'wifi-ssid': ''}, {**valid, 'wifi-ssid': 'x'*33},
                            {**valid, 'wifi-password': 'short'}, {**valid, 'wifi-password': 123}]:
                p.write_text(json.dumps(invalid))
                with self.assertRaises(ValueError) as error:
                    load(directory, env)
                self.assertNotIn(valid['wifi-password'], str(error.exception))
            p.write_text(json.dumps(valid))
            p.chmod(0o644)
            with self.assertRaisesRegex(ValueError, 'owner-only'):
                load(directory, env)

if __name__ == '__main__': unittest.main()
