"""All S.Port builds share this gate; no upload occurs unless preflight passes."""
Import('env')
if any(target.startswith('upload') for target in COMMAND_LINE_TARGETS):
    import runpy
    from pathlib import Path
    try:
        checks = runpy.run_path(str(Path(env.subst('$PROJECT_DIR')) / 'python/sport_preflight.py'))
        checks['run'](env, COMMAND_LINE_TARGETS)
    except (ValueError, OSError) as error:
        print('\nS.PORT RECEIVER UPLOAD PREFLIGHT: REFUSED\n' + str(error) + '\nNo firmware has been written.\n')
        Exit(1)
