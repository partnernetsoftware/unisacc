#!/usr/bin/env python3
"""doceditcheck (0.0.40): the 0586f01a truncating append is caught (rc 1); docedit appends keep the old text as an
exact prefix (UTF-8 and raw bytes), keep the file mode, leave no temp file, and refuse a missing required heading."""
import os, pathlib, shutil, subprocess, sys, tempfile
TOOL = pathlib.Path(__file__).resolve().parents[1] / 'release/tools/docedit.py'
def run(*a): return subprocess.run([sys.executable, str(TOOL), *a], capture_output=True, text=True, timeout=20)
def fail(m): print('docedit: ' + m); sys.exit(1)
T = pathlib.Path(tempfile.mkdtemp(prefix='unisacc-docedit.'))
try:
    plan = T / 'plan.md'; plan.write_bytes('# v0.0.39\n\n## R9\n| K2a | 顺延 0.0.40 |\n'.encode() + b'raw \xff byte\n'); os.chmod(plan, 0o640)
    before = T / 'before.md'; shutil.copy(plan, before)
    note = T / 'note.md'; note.write_text('## 裁定③\n- stamp reviewed\n')
    # 1. the 0586f01a pattern: write mode opens (truncates) before the read -> verify is RED
    legacy = T / 'legacy.md'; shutil.copy(plan, legacy); legacy_before = T / 'legacy-before.md'; shutil.copy(legacy, legacy_before)
    exec("open(p,'wb').write(open(p,'rb').read()+n)", {'p': str(legacy), 'n': note.read_bytes()})
    r = run('verify', str(legacy), str(legacy_before), '--require', '## R9')
    if r.returncode != 1 or 'not an exact prefix' not in r.stdout: fail('legacy truncating append not caught: %s' % r.stdout)
    # 2. docedit append keeps the prefix byte-for-byte, the mode, and leaves no temp file
    r = run('append', str(plan), str(note), '--require', '## R9', '--require', '## 裁定③')
    if r.returncode != 0: fail('append failed: %s' % r.stdout)
    if not plan.read_bytes().startswith(before.read_bytes()) or not plan.read_bytes().endswith(note.read_bytes()): fail('append did not keep prefix + new section')
    if os.stat(plan).st_mode & 0o777 != 0o640: fail('append changed the file mode')
    if any(p.name.startswith('.docedit.') for p in T.iterdir()): fail('temp file left behind')
    if run('verify', str(plan), str(before), '--require', '## R9').returncode != 0: fail('verify of a good append not ok')
    # 3. a missing required heading refuses the append and leaves the file untouched
    snap = plan.read_bytes(); r = run('append', str(plan), str(note), '--require', '## R10')
    if r.returncode != 1 or plan.read_bytes() != snap: fail('missing required heading not refused / file changed')
    # 4. a rewritten middle (prefix changed) is RED even when the length grows
    plan.write_bytes(snap.replace('顺延'.encode(), '已完成'.encode()) + b'more\n')
    if run('verify', str(plan), str(before)).returncode != 1: fail('rewritten middle not caught')
    print('docedit  0586f01a truncating append RED; append keeps exact byte prefix, mode, no temp file; missing heading refused with file untouched; rewritten middle RED')
finally:
    shutil.rmtree(T, ignore_errors=True)
