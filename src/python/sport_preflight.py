"""Receiver-only S.Port upload validation. No transmitter repository dependency."""
from pathlib import Path
import subprocess

BASE_COMMIT = 'a9d4a9cb5b5687c4c9d7e9e7fbdf44ad93651da6'
TRIAL_BRANCH = 'codex/sport-telemetry'
UPLOAD_ENV = 'ER4_ESP8285_2400_RX_SPORT_via_WIFI'

def validate_target(name, board, profile, targets):
    if set(targets) != {'upload'} or name != UPLOAD_ENV:
        raise ValueError('Only the ordinary ER4 S.Port WiFi upload target is enabled; force/other uploads are blocked.')
    if board != 'esp8285' or profile != 'radiomaster.rx_2400.er4':
        raise ValueError('Expected RadioMaster ER4 ESP8285 profile.')

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
    print('  environment: ' + UPLOAD_ENV)
    print('  receiver: RadioMaster ER4 / ESP8285; no display')
    print('  mode: source discovery only by default; verify output mapping before wiring\n')
