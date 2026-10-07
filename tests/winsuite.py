#!/usr/bin/env python3
"""winsuite: the Windows probes that used to need the UTM VM, split into prepare and run (0.0.25 X7).

  tests/winsuite.py prepare OUT UA   on a POSIX host: build every probe for win/arm64 and win/x86_64
                                     and record the expected outputs in a manifest.json inside OUT
  tests/winsuite.py run OUT [ARCH]   on a Windows machine: run the probes for ARCH (default: this
                                     machine's architecture) and compare; exit 1 on any mismatch or missing exe

What moves here (each was "compile on the Mac, run in the VM, compare"):
  winposix    tests/hosthdr/winposix*.c built by UA; expected = the same probe built by the host cc
  crossnative examples/*.c built by the Python route; expected = `python3 -m unisa run --target`
  forward     tests/forward/win.c built by UA; expected = the fixed line forward.sh checks
  lifecycle   tests/life/cases.c built by UA, one run per case (0.0.33 W3): stdout AND exit code.
              Expected = the POSIX shell statuses tests/lifecycle.sh checks against cc, which is the
              unisacc rule on every target: abort 134 and raise(SIGTERM) 143 (MSVC's CRT says 3 for
              both), exit codes are 32-bit (ret256 is 256, not 0), and a crash is the OS's own
              report (STATUS_ACCESS_VIOLATION, 0xC0000005).
Release-check runs prepare on a macOS runner and run on windows-latest and windows-11-arm.  The UTM
VM stays available for hands-on debugging only (owner 2026-10-04).
"""
import json, os, pathlib, platform, subprocess, sys, tempfile
ROOT = pathlib.Path(__file__).resolve().parents[1]
ARCHES = ('win/arm64', 'win/x86_64')
# W3: (case, stdout, exit code) for tests/life/cases.c on Windows; see the docstring
LIFE = (('ret0', '', 0), ('ret7', '', 7), ('ret255', '', 255), ('ret256', '', 256), ('exit3', '', 3),
        ('atexit', 'main\nh2\nh1', 5), ('pending', 'no newline', 4), ('abort', 'before', 134),
        ('raise', '', 143), ('handled', 'seen 15', 0), ('segv', '', 0xC0000005))

def norm(b):
    text = b.decode('utf-8', 'replace').replace('\r', '')
    return '\n'.join(l.rstrip() for l in text.split('\n') if l.strip())

def sh(cmd, cwd=None, t=60):
    return subprocess.run(cmd, cwd=cwd, capture_output=True, timeout=t)

def prepare(out, ua):
    out = pathlib.Path(out); out.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, PYTHONPATH=str(ROOT))
    entries = []
    with tempfile.TemporaryDirectory(prefix='winsuite-') as td:
        for p in sorted((ROOT / 'tests/hosthdr').glob('winposix*.c')):
            ref = pathlib.Path(td) / ('ref-' + p.stem)
            r = sh(['cc', '-std=c99', '-w', '-o', str(ref), str(p)])
            if r.returncode: sys.exit('winsuite: cc failed on %s' % p)
            run = pathlib.Path(td) / ('c-' + p.stem); run.mkdir()
            r = subprocess.run([str(ref)], cwd=run, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=20)
            want = norm(r.stdout)
            for a in ARCHES:
                exe = '%s-%s.exe' % (p.stem, a.split('/')[1])
                r = sh([ua, str(p), '-b', a, '-o', str(out / exe)])
                entries.append({'suite': 'winposix', 'name': p.stem, 'arch': a, 'exe': exe if not r.returncode else None,
                                'want': want, 'compile_error': r.stderr.decode()[:200] if r.returncode else None})
        for p in sorted((ROOT / 'examples').glob('*.c')):
            for a in ARCHES:
                exe = 'ex-%s-%s.exe' % (p.stem, a.split('/')[1])
                r = subprocess.run(['python3', '-m', 'unisa', 'compile', str(p), '-o', str(out / exe), '--target', a, '--drive', 'built'],
                                   cwd=ROOT, env=env, capture_output=True, timeout=120)
                if r.returncode: continue          # crossnative.sh skips what the Python route cannot build, too
                w = subprocess.run(['python3', '-m', 'unisa', 'run', str(p), '--target', a, '--drive', 'built'],
                                   cwd=ROOT, env=env, capture_output=True, timeout=120)
                entries.append({'suite': 'crossnative', 'name': p.stem, 'arch': a, 'exe': exe, 'want': norm(w.stdout)})
        for a in ARCHES:
            exe = 'forward-win-%s.exe' % a.split('/')[1]
            r = sh([ua, str(ROOT / 'tests/forward/win.c'), '-b', a, '-o', str(out / exe)])
            entries.append({'suite': 'forward', 'name': 'win', 'arch': a, 'exe': exe if not r.returncode else None,
                            'want': 'pid>0 1 tick>0 1 len 9', 'compile_error': r.stderr.decode()[:200] if r.returncode else None})
        life = ROOT / 'tests/life/cases.c'
        for a in ARCHES:
            exe = 'life-%s.exe' % a.split('/')[1]
            r = sh([ua, str(life), '-b', a, '-o', str(out / exe)])
            for case, want, rc in LIFE:
                entries.append({'suite': 'lifecycle', 'name': case, 'arch': a, 'exe': exe if not r.returncode else None,
                                'args': [case], 'want': want, 'want_rc': rc,
                                'compile_error': r.stderr.decode()[:200] if r.returncode else None})
        # nativeboot's Windows proof: the win image of the compiler rebuilds itself from the flat source
        flat = out / 'unisacc.flat.c'
        if subprocess.run(['python3', str(ROOT / 'tests/sourceflat.py'), str(flat)], cwd=ROOT, capture_output=True, timeout=60).returncode:
            sys.exit('winsuite: sourceflat failed')
        for a in ARCHES:
            exe = 'selfbuild-%s.exe' % a.split('/')[1]
            with open(out / exe, 'wb') as f:
                r = subprocess.run([ua, str(flat), '-b', a], stdout=f, stderr=subprocess.PIPE, timeout=180)
            entries.append({'suite': 'nativeboot', 'name': 'selfbuild', 'arch': a, 'kind': 'selfbuild',
                            'exe': exe if not r.returncode else None, 'want': None,
                            'compile_error': r.stderr.decode()[:200] if r.returncode else None})
    (out / 'manifest.json').write_text(json.dumps({'schema': 1, 'entries': entries}, indent=1) + '\n')
    bad = [e for e in entries if not e['exe']]
    print('winsuite prepare  %d probes for %s   compile failures %d' % (len(entries), ', '.join(ARCHES), len(bad)))
    for e in bad: print('  FAIL %s %s %s compile: %s' % (e['suite'], e['name'], e['arch'], e['compile_error']))
    return 1 if bad else 0

def run(out, arch=None):
    out = pathlib.Path(out)
    # an x64 Python on Windows 11 arm reports AMD64, so the workflow names the target itself
    arch = arch or ('win/arm64' if platform.machine().lower() in ('arm64', 'aarch64') else 'win/x86_64')
    entries = [e for e in json.loads((out / 'manifest.json').read_text())['entries'] if e['arch'] == arch]
    ok = bad = 0
    for e in entries:
        if not e['exe']:
            bad += 1; print('  FAIL %-11s %-16s %s (no executable)' % (e['suite'], e['name'], arch)); continue
        if e.get('kind') == 'selfbuild':
            img = (out / e['exe']).resolve()
            with tempfile.TemporaryDirectory(prefix='winsuite-') as td:
                try:
                    r = subprocess.run([str(img), str((out / 'unisacc.flat.c').resolve()), '-b', arch], cwd=td,
                                       stdin=subprocess.DEVNULL, capture_output=True, timeout=300)
                    same = r.returncode == 0 and r.stdout == img.read_bytes()
                except subprocess.TimeoutExpired:
                    same = False
            if same: ok += 1; print('  ok   nativeboot  %s rebuilt itself byte for byte' % arch)
            else: bad += 1; print('  FAIL nativeboot  %s self-build differs or failed' % arch)
            continue
        with tempfile.TemporaryDirectory(prefix='winsuite-') as td:
            try:
                r = subprocess.run([str((out / e['exe']).resolve())] + e.get('args', []), cwd=td, stdin=subprocess.DEVNULL,
                                   stdout=subprocess.PIPE, stderr=subprocess.DEVNULL if 'want_rc' in e else subprocess.STDOUT, timeout=20)
                got = norm(r.stdout)
                if 'want_rc' in e and (r.returncode & 0xFFFFFFFF) != e['want_rc']:
                    got += ' <exit %d, want %d>' % (r.returncode & 0xFFFFFFFF, e['want_rc'])
            except subprocess.TimeoutExpired:
                got = '<timeout>'
        if got == e['want']:
            ok += 1; print('  ok   %-11s %-16s %s' % (e['suite'], e['name'], arch))
        else:
            bad += 1; print('  FAIL %-11s %-16s %s\n    want %r\n    got  %r' % (e['suite'], e['name'], arch, e['want'][:200], got[:200]))
    print('winsuite %s  ok %d   wrong %d' % (arch, ok, bad))
    return 0 if ok and not bad else 1

if __name__ == '__main__':
    if len(sys.argv) >= 4 and sys.argv[1] == 'prepare': sys.exit(prepare(sys.argv[2], sys.argv[3]))
    if len(sys.argv) >= 3 and sys.argv[1] == 'run': sys.exit(run(sys.argv[2], sys.argv[3] if len(sys.argv) > 3 else None))
    sys.exit(__doc__)
