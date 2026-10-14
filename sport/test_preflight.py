from pathlib import Path
import runpy
import unittest
checks = runpy.run_path(str(Path(__file__).resolve().parents[1] / 'src/python/sport_preflight.py'))
class Tests(unittest.TestCase):
    def test_target(self):
        checks['validate_target'](checks['UPLOAD_ENV'],'esp8285','radiomaster.rx_2400.er4',['upload'])
        for env,board,profile,targets in [
            ('ER6G_ESP32_2400_RX_SPORT_BUILD','esp32dev','radiomaster.rx_2400.er6g',['upload']),
            (checks['UPLOAD_ENV'],'esp32dev','radiomaster.rx_2400.er4',['upload']),
            (checks['UPLOAD_ENV'],'esp8285','radiomaster.rx_2400.er6g',['upload']),
            (checks['UPLOAD_ENV'],'esp8285','radiomaster.rx_2400.er4',['uploadforce']),
            (checks['UPLOAD_ENV'],'esp8285','radiomaster.rx_2400.er4',['uploadfs'])]:
            with self.assertRaises(ValueError):checks['validate_target'](env,board,profile,targets)
    def test_git(self):
        checks['validate_git']('codex/sport-telemetry',True,True,False)
        for args in [('main',True,True,False),('codex/sport-telemetry',False,True,False),
                     ('codex/sport-telemetry',True,False,False),('codex/sport-telemetry',True,True,True)]:
            with self.assertRaises(ValueError):checks['validate_git'](*args)
if __name__=='__main__':unittest.main()
