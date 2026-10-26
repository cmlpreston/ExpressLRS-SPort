"""Receiver-only S.Port upload validation. No transmitter repository dependency."""
from pathlib import Path
import subprocess

BASE_COMMIT = 'a9d4a9cb5b5687c4c9d7e9e7fbdf44ad93651da6'
TRIAL_BRANCH = 'codex/sport-telemetry'
UPLOAD_ENV = 'ER4_ESP8285_2400_RX_SPORT_via_WIFI'

UPLOAD_TARGETS = {
    UPLOAD_ENV: ('esp8285', 'radiomaster.rx_2400.er4', 'RadioMaster ER4 / ESP8285'),
    'ER6G_ESP32_2400_RX_SPORT_via_WIFI': ('esp32dev', 'radiomaster.rx_2400.er6g', 'RadioMaster ER6-G / ESP32'),
}

def validate_target(name, board, profile, targets):
    if set(targets) != {'upload'} or name not in UPLOAD_TARGETS:
        raise ValueError('Only reviewed ER4/ER6G S.Port WiFi upload targets are enabled; force/other uploads are blocked.')
    expected_board, expected_profile, _ = UPLOAD_TARGETS[name]
    if (board, profile) != (expected_board, expected_profile):
        raise ValueError('Board or receiver hardware profile does not match the selected S.Port environment.')

def validate_er6g_device(settings, cfg):
    if settings.get('product_name') != 'RadioMaster ER6-G 2.4GHz Diversity+6xPWM RX':
        raise ValueError('Upload destination is not the reviewed ER6-G model.')
    if settings.get('target') != 'UNIFIED_ESP32_2400_RX':
        raise ValueError('ER6-G destination has an unexpected firmware target.')
    if [p.get('pin') for p in cfg.get('pwm', [])] != [14,1,3,15,2,4]:
        raise ValueError('ER6-G output pin map differs from the reviewed profile.')
    if settings.get('radio-type') != 'SX128X' or settings.get('reg_domain_high') != 'ISM2G4':
        raise ValueError('ER6-G radio/domain differs from the reviewed 2.4GHz ISM profile.')

def validate_git(branch, base_ok, remote_ok, dirty):
    if branch != TRIAL_BRANCH:
        raise ValueError('Use the reviewed codex/sport-telemetry receiver branch.')
    if not base_ok:
        raise ValueError('Receiver source is not descended from the selected ExpressLRS 4.1.0 base.')
    if not remote_ok:
        raise ValueError('Local receiver branch is behind or unrelated to its refreshed origin branch.')
    if dirty:
        raise ValueError('Commit and review receiver changes before an upload; checkout must be clean.')

def validate_version_file(actual, pinned):
    # Upstream keeps a git-archive placeholder here, not a release number.
    # The fixed base commit and ancestry check establish the release version.
    if actual != pinned:
        raise ValueError('VERSION differs from the pinned ExpressLRS 4.1.0 source.')

def run(env, targets):
    project = Path(env.subst('$PROJECT_DIR')).resolve()
    root = project.parent
    validate_target(env.subst('$PIOENV'), env.subst('$BOARD'), env.GetProjectOption('board_config', ''), targets)
    def git(*args):
        return subprocess.run(['git', *args], cwd=root, text=True, capture_output=True)
    name = env.subst('$PIOENV')
    if name.startswith('ER6G_'):
        import json
        from urllib.request import build_opener, ProxyHandler
        from sport_local_wifi import load_local_wifi
        if not load_local_wifi(project, name):
            raise ValueError('ER6-G upload requires private WiFi defaults to retain home-network access.')
        address = str(env.subst('$UPLOAD_PORT'))
        with build_opener(ProxyHandler({})).open('http://' + address + '/config', timeout=6) as response:
            destination = json.load(response)
        validate_er6g_device(destination.get('settings',{}), destination.get('config',{}))
    pinned_version = git('show', BASE_COMMIT + ':src/VERSION')
    if pinned_version.returncode:
        raise ValueError('Cannot read VERSION from the pinned ExpressLRS 4.1.0 base.')
    validate_version_file((project / 'VERSION').read_text(), pinned_version.stdout)
    fetched = git('fetch', 'origin', TRIAL_BRANCH)
    if fetched.returncode:
        raise ValueError('Could not refresh receiver origin branch: ' + fetched.stderr.strip())
    status = git('status', '--short', '--branch')
    print(status.stdout)
    branch = git('branch', '--show-current').stdout.strip()
    validate_git(branch,
        git('merge-base', '--is-ancestor', BASE_COMMIT, 'HEAD').returncode == 0,
        git('merge-base', '--is-ancestor', 'origin/' + TRIAL_BRANCH, 'HEAD').returncode == 0,
        bool(git('status', '--porcelain').stdout.strip()))
    if subprocess.run(['make', '-C', str(root / 'sport'), 'test']).returncode:
        raise ValueError('Receiver host tests failed.')
    print('\nS.PORT RECEIVER UPLOAD PREFLIGHT: PASSED')
    print('  branch: ' + branch)
    print('  base: ExpressLRS 4.1.0 / ' + BASE_COMMIT[:8])
    print('  environment: ' + name)
    print('  receiver: ' + UPLOAD_TARGETS[name][2] + '; no display')
    print('  mode: selected 4S cells and FAS300 ADV; verify output mapping before wiring\n')
