#!/usr/bin/env python3
"""Real snapshots share one input between cc and the shipped model compiler.
Live self maps require structural checks, not equality between different binaries.
All commands and descendants are bounded; there is no synthetic demo fallback.
"""
import argparse, os, pathlib, platform, re, subprocess, tempfile, time
from appsstructurecheck import processes as check_processes, maps as check_maps
from appsstructurecheck import windows as check_windows, process_report, window_list, check_controls

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
    allowed = expected if isinstance(expected, tuple) else (expected,)
    if r.returncode not in allowed:
        raise RuntimeError(f'{args}: exit {r.returncode}, expected {expected}: {r.stderr.decode(errors="replace")}')
    return r

try:
    check_controls()
    prefix = ['/bin/sh', a.compiler] if a.compiler.read_bytes()[:2] == b'MZ' else [a.compiler]
    with tempfile.TemporaryDirectory(prefix='unisacc-apps-test-') as td:
        d = pathlib.Path(td)
        # Actual unisacc programs collect their own live inputs. No cc collector.
        process_capture = run([*prefix, '-run', a.apps/'procview.c', '--', '--capture'])
        map_capture = run([*prefix, '-run', a.apps/'memmap.c', '--', '--capture'])
        processes, maps = process_capture.stdout, map_capture.stdout
        process_count, map_count = check_processes(processes), check_maps(maps)
        # Capture RSS=0 does not distinguish unreadable RSS from genuine zero.
        # The live default report separately checks its '?' privilege notice.
        if platform.system() == 'Darwin':
            if not re.search(rb'querying this process, pid [1-9][0-9]*', map_capture.stderr):
                raise RuntimeError('captured self maps did not name their own PID')
        print(f'captured structure: {process_count} unique processes, {map_count} ordered non-overlapping regions', flush=True)
        (d/'processes').write_bytes(processes)
        (d/'maps').write_bytes(maps)
        if platform.system() == 'Darwin':
            windows = run([*prefix, '-run', a.apps/'winlayout.c', '--', '--capture']).stdout
        else:
            # Analytic fixture only; live windows are unavailable on this platform.
            windows = b'screen 100 100\n0 0 100 100 bottom\n0 0 50 100 top\n'
        check_windows(windows)
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
                if app == 'procview':
                    process_report(text)
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
        # winlist has no captured-input analyzer: each model invocation queries
        # its own window server. Compare structure, never live output with cc.
        live_lists = 0
        for args, all_windows in (([], False), (['--all'], True)):
            result = run([*prefix, '-run', a.apps/'winlist.c', '--', *args], expected=(0, 1))
            if result.returncode == 0:
                if platform.system() != 'Darwin':
                    raise RuntimeError('winlist unexpectedly claims a live non-macOS binding')
                window_list(result.stdout, all_windows)
                live_lists += 1
            else:
                unavailable = re.search(rb'winlist: (framework load failed|missing |FFI call |no GUI window-server session|the window server is only reachable)', result.stderr)
                if result.stdout.strip() or not unavailable:
                    raise RuntimeError('winlist exit 1 is not an explicit unavailable-query diagnostic')
                if a.live_windows:
                    raise RuntimeError('required live winlist query unavailable: '+result.stderr.decode(errors='replace'))
                print('winlist: query unavailable; not counted as live pass: '+result.stderr.decode(errors='replace').strip(), flush=True)
        run([*prefix, '-run', a.apps/'winlist.c', '--', '--invalid'], expected=2)
        print(f'winlist: {live_lists}/2 live modes checked; invalid argument rejected', flush=True)
        print('real apps: 4 captured-input analyzers checked; process/maps snapshots live; windows '+('application-captured live' if platform.system()=='Darwin' else 'analytic fixture only'))
except (OSError, RuntimeError) as error:
    print('real apps: FAIL:', error, flush=True)
    raise SystemExit(1)
