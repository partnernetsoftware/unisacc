#!/usr/bin/env python3
"""Real snapshots share one input between cc and the shipped model compiler.
Live self maps require structural checks, not equality between different binaries.
All commands and descendants are bounded; there is no synthetic demo fallback.
"""
import argparse, os, pathlib, platform, subprocess, tempfile, time

ROOT = pathlib.Path(os.environ.get('APP_ROOT', pathlib.Path(__file__).resolve().parents[1])).resolve()
p = argparse.ArgumentParser()
p.add_argument('--apps', type=pathlib.Path, default=ROOT / 'examples/apps')
p.add_argument('--compiler', type=pathlib.Path, default=pathlib.Path(os.environ.get('MODEL_COM', ROOT / 'unisacc.com')))
p.add_argument('--live-windows', action='store_true')
a = p.parse_args()
a.apps = a.apps.resolve(); a.compiler = a.compiler.resolve()
deadline = time.monotonic() + 50

def run(args, expected=0):
    remaining = min(20, int(deadline - time.monotonic()))
    if remaining < 1: raise RuntimeError('whole check deadline exceeded')
    r = subprocess.run(['python3', str(ROOT/'tests/bound.py'), str(remaining), *map(str, args)],
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if r.returncode != expected:
        raise RuntimeError(f'{args}: exit {r.returncode}, expected {expected}: {r.stderr.decode(errors="replace")}')
    return r

try:
    prefix = ['/bin/sh', a.compiler] if a.compiler.read_bytes()[:2] == b'MZ' else [a.compiler]
    with tempfile.TemporaryDirectory(prefix='unisacc-apps-test-') as td:
        d = pathlib.Path(td)
        # Actual unisacc programs collect their own live inputs. No cc collector.
        processes = run([*prefix, '-run', a.apps/'procview.c', '--', '--capture']).stdout
        maps = run([*prefix, '-run', a.apps/'memmap.c', '--', '--capture']).stdout
        if not processes.strip() or not maps.strip(): raise RuntimeError('empty application-captured live data')
        (d/'processes').write_bytes(processes)
        (d/'maps').write_bytes(maps)
        if platform.system() == 'Darwin':
            windows = run([*prefix, '-run', a.apps/'winlayout.c', '--', '--capture']).stdout
            if not windows.startswith(b'screen '): raise RuntimeError('live window snapshot missing screen')
        else:
            # Analytic fixture only; live windows are unavailable on this platform.
            windows = b'screen 100 100\n0 0 100 100 bottom\n0 0 50 100 top\n'
        (d/'windows').write_bytes(windows)
        inputs = {'procview':d/'processes', 'memmap':d/'maps', 'winlayout':d/'windows', 'exeinfo':a.compiler}
        for app, source_input in inputs.items():
            source = a.apps/(app+'.c')
            ffi_flags = ['-DUFFI_HOST_SHIM', '-include', ROOT/'exec/ffi/hostshim.h', '-idirafter', ROOT/'include', '-lffi'] if platform.system() == 'Darwin' else []
            run(['cc', '-std=c99', '-Wall', '-Wextra', source, *ffi_flags, '-o', d/app])
            want = run([d/app, source_input]).stdout
            if not want.strip() or b'== sample:' in want: raise RuntimeError(f'{app}: empty/synthetic output')
            got = run([*prefix, '-run', source, source_input]).stdout
            if got != want: raise RuntimeError(f'{app}: -run differs from cc')
            run([*prefix, '-O2', source, '-o', d/(app+'-native')])
            native = run([d/(app+'-native'), source_input]).stdout
            if native != want: raise RuntimeError(f'{app}: native differs from cc')
            run([*prefix, '-run', source, d/'missing'], expected=1)
            if app == 'exeinfo':
                # Each application itself reads real executables. The cc process
                # is a reference, never a collector for the model application.
                defaults = [run([d/app]).stdout,
                            run([*prefix, '-run', source]).stdout,
                            run([d/(app+'-native')]).stdout]
                for text in defaults:
                    if not text.startswith(b'== ') or b'format   ' not in text:
                        raise RuntimeError('exeinfo default did not dissect a host executable')
                    if b'== sample:' in text or b'SAMPLE' in text:
                        raise RuntimeError('synthetic default')
                # Linux reads /proc/self/exe: different binaries are legitimate.
                # macOS/Windows defaults name the same existing system files.
                if platform.system() != 'Linux' and not defaults[0] == defaults[1] == defaults[2]:
                    raise RuntimeError('exeinfo same system-file defaults differ')
            else:
                own = run([*prefix, '-run', source], expected=1 if app=='winlayout' and platform.system()!='Darwin' else 0)
                if app=='winlayout' and platform.system()!='Darwin':
                    print('winlayout: live API unavailable on this platform; not counted as live pass',flush=True)
                    continue
                text = own.stdout
                if app == 'procview' and b'== process tree (' not in text:
                    raise RuntimeError('live process collection missing structure')
                if app == 'memmap':
                    if b'== regions (' not in text or b'lowest start' not in text:
                        raise RuntimeError('live self maps missing structure')
                    if platform.system() == 'Darwin' and b'querying this process, pid ' not in own.stderr:
                        raise RuntimeError('live self maps did not name its own pid')
                if app == 'winlayout' and b'== screen ' not in text:
                    raise RuntimeError('live window collection missing structure')
                if b'== sample:' in text or b'SAMPLE' in text:
                    raise RuntimeError('synthetic default')
            if app != 'exeinfo':
                (d/'empty').write_bytes(b'')
                run([*prefix, '-run', source, d/'empty'], expected=1)
            print(f'PASS {app}: cc / model -run / native, missing input, default and empty checks', flush=True)
        print('real apps: 4 checked; process/maps snapshots live; windows '+('application-captured live' if platform.system()=='Darwin' else 'analytic fixture only'))
except (OSError, RuntimeError) as error:
    print('real apps: FAIL:', error, flush=True)
    raise SystemExit(1)
