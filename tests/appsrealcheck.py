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
    r = subprocess.run(['perl', str(ROOT/'tests/bound.pl'), str(remaining), *map(str, args)],
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if r.returncode != expected:
        raise RuntimeError(f'{args}: exit {r.returncode}, expected {expected}: {r.stderr.decode(errors="replace")}')
    return r

try:
    prefix = ['/bin/sh', a.compiler] if a.compiler.read_bytes()[:2] == b'MZ' else [a.compiler]
    with tempfile.TemporaryDirectory(prefix='unisacc-apps-test-') as td:
        d = pathlib.Path(td)
        processes = run(['ps', '-axo', 'pid=,ppid=,rss=,comm=']).stdout
        if not processes.strip(): raise RuntimeError('empty live process snapshot')
        (d/'processes').write_bytes(processes)
        if platform.system() == 'Darwin':
            run(['cc', '-std=c99', '-Wall', '-Wextra', a.apps/'tools/selfmaps.c', '-o', d/'collector'])
            maps = run([d/'collector']).stdout
        elif platform.system() == 'Linux':
            maps = pathlib.Path('/proc/self/maps').read_bytes()
        else: raise RuntimeError('live maps collector not available on this host')
        if not maps.strip(): raise RuntimeError('empty live maps snapshot')
        (d/'maps').write_bytes(maps)
        if a.live_windows:
            if platform.system() != 'Darwin': raise RuntimeError('live windows require macOS collector')
            run(['cc', a.apps/'tools/wingeom.c', '-framework', 'CoreGraphics', '-framework', 'CoreFoundation', '-o', d/'windows-collector'])
            windows = run([d/'windows-collector']).stdout
        else:
            # Analytic test fixture only. Application code never supplies it.
            windows = b'screen 100 100\n0 0 100 100 bottom\n0 0 50 100 top\n'
        (d/'windows').write_bytes(windows)
        inputs = {'procview':d/'processes', 'memmap':d/'maps', 'winlayout':d/'windows', 'exeinfo':a.compiler}
        for app, source_input in inputs.items():
            source = a.apps/(app+'.c')
            run(['cc', '-std=c99', '-Wall', '-Wextra', source, '-o', d/app])
            want = run([d/app, source_input]).stdout
            if not want.strip() or b'== sample:' in want: raise RuntimeError(f'{app}: empty/synthetic output')
            got = run([*prefix, '-run', source, source_input]).stdout
            if got != want: raise RuntimeError(f'{app}: -run differs from cc')
            run([*prefix, '-O2', source, '-o', d/(app+'-native')])
            native = run([d/(app+'-native'), source_input]).stdout
            if native != want: raise RuntimeError(f'{app}: native differs from cc')
            run([*prefix, '-run', source, d/'missing'], expected=1)
            if app != 'memmap' or platform.system() != 'Linux':
                run([d/app], expected=1)
                run([*prefix, '-run', source], expected=1)
            else:
                own = run([*prefix, '-run', source]).stdout
                if b'== regions' not in own or b'lowest start' not in own: raise RuntimeError('live self maps missing structure')
            if app != 'exeinfo':
                (d/'empty').write_bytes(b'')
                run([*prefix, '-run', source, d/'empty'], expected=1)
            print(f'PASS {app}: cc / model -run / native, missing input, default and empty checks', flush=True)
        print('real apps: 4 checked; process/maps snapshots live; windows '+('live' if a.live_windows else 'analytic fixture'))
except (OSError, RuntimeError) as error:
    print('real apps: FAIL:', error, flush=True)
    raise SystemExit(1)
