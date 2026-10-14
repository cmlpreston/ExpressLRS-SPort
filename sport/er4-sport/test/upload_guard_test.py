import runpy
from pathlib import Path
path=Path(__file__).resolve().parents[3]/'src/python/sport_no_upload.py'
class Env:
    def subst(self,key):
        return str(path.parents[1]) if key=='$PROJECT_DIR' else 'ER4_ESP8285_2400_RX_SPORT_BUILD'
    def GetProjectOption(self,*_):return 'radiomaster.rx_2400.er4'
def run(targets):
    def stop(code):raise SystemExit(code)
    return runpy.run_path(str(path),init_globals={'Import':lambda _:None,'env':Env(),'COMMAND_LINE_TARGETS':targets,'Exit':stop})
run(['buildprog'])
for targets in (['upload'],['uploadforce'],['uploadfs']):
    try:run(targets)
    except SystemExit as error:assert error.code==1
    else:raise AssertionError('Compile-only environment allowed upload')
print('Build-only upload guard tests passed')
