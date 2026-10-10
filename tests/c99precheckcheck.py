#!/usr/bin/env python3
"""c99precheck controls (0.0.39 WF1): per-probe verdicts from difftest_o lines (refuse only when all three
-O levels refuse; any WRONG/FAIL is 'other'); the shard map follows difftest_o.sh's own bash glob order."""
import importlib.util, pathlib, subprocess
ROOT = pathlib.Path(__file__).resolve().parents[1]

import hashlib, tempfile

def input_snapshot(root):
    return {p.relative_to(root).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
            for folder in ('examples', 'tests/c')
            for p in (root / folder).rglob('*') if p.is_file()}

def require_unchanged(before, after):
    assert before == after, 'c99precheck modified repository inputs'

index_before = subprocess.check_output(['git', 'ls-files', '-s', '-z'], cwd=ROOT)
examples_before = input_snapshot(ROOT)
# Controlled negatives: missing member and same member with changed bytes.
with tempfile.TemporaryDirectory() as tmp:
    private = pathlib.Path(tmp); (private / 'examples').mkdir()
    probe = private / 'examples' / 'probe.c'; probe.write_text('original')
    original = input_snapshot(private)
    for changed in ({}, {**original, 'examples/probe.c': 'changed-sha'}):
        try: require_unchanged(original, changed)
        except AssertionError: pass
        else: raise AssertionError('input mutation accepted')
    try: require_unchanged(b'index-before', b'index-after')
    except AssertionError: pass
    else: raise AssertionError('index mutation accepted')
# End-to-end old collision: its shell prints green, but the guard exits 1.
with tempfile.TemporaryDirectory() as tmp:
    private = pathlib.Path(tmp); (private / 'examples').mkdir()
    (private / 'examples/probe.c').write_text('original')
    child = """import pathlib, hashlib, subprocess
root=pathlib.Path('.')
def snapshot():
 return {str(p):hashlib.sha256(p.read_bytes()).hexdigest() for folder in ('examples','tests/c') for p in (root/folder).rglob('*') if p.is_file()}
before=snapshot()
r=subprocess.run(['bash','-c','f=$(mktemp); printf \"for f in examples/*.c; do :; done\\n\" > \"$f\"; . \"$f\"; rm -f \"$f\"; echo legacy-green'],capture_output=True,text=True,check=True)
print(r.stdout,flush=True)
assert before==snapshot(), 'c99precheck modified repository inputs'
"""
    import sys
    result = subprocess.run([sys.executable, '-c', child], cwd=private, capture_output=True, text=True)
    assert result.returncode == 1 and 'legacy-green' in result.stdout and 'modified repository inputs' in result.stderr, result
spec = importlib.util.spec_from_file_location('c99precheck_under_test', ROOT / 'release/tools/c99precheck.py')
P = importlib.util.module_from_spec(spec); spec.loader.exec_module(P)
rows = [{'probe': 'tests/c/a.c', 'expect': 'refuse'}, {'probe': 'tests/c/b.c', 'expect': 'agree'},
        {'probe': 'tests/c/c.c', 'expect': 'refuse'}, {'probe': 'tests/c/d.c', 'expect': 'agree'}]
text = '\n'.join(['  REFUSE a -O0: not covered: x', '  REFUSE a -O1: not covered: x', '  REFUSE a -O2: not covered: x',
                  '  REFUSE c -O0: not covered: y', '  WRONG d              -O2: got [1] cc -O2 says [2]',
                  'difftest_o SHARD=1/1 probes=4  agree 7   wrong 1   refuse 4   known 0   revived 0   (x)'])
v = P.verdicts(text, rows)
assert [v[r['probe']]['got'] for r in rows] == ['refuse', 'agree', 'other', 'other'], v
assert [v[r['probe']]['ok'] for r in rows] == [True, True, False, False], v
# refuse needs -O0, -O1 and -O2 once each (three -O0 lines are not a refusal); a nonzero rc must be explained
three = 'difftest_o SHARD=1/1 probes=1  agree 0   wrong 0   refuse 3   known 0   revived 0'
dup = P.verdicts('\n'.join(['  REFUSE a -O0: not covered: x'] * 3 + [three]), rows[:1])
assert dup['tests/c/a.c']['got'] in ('other', 'unproven') and not dup['tests/c/a.c']['ok'], dup   # same level thrice
ok3 = P.verdicts('\n'.join(['  REFUSE a %s: not covered: x' % o for o in ('-O0', '-O1', '-O2')] + [three]), rows[:1], rc=1)
assert ok3['tests/c/a.c']['got'] == 'refuse', ok3
agree1 = 'difftest_o SHARD=1/1 probes=1  agree 3   wrong 0   refuse 0   known 0   revived 0'
assert P.verdicts(agree1, rows[1:2], rc=0)['tests/c/b.c']['got'] == 'agree'
assert P.verdicts(agree1, rows[1:2], rc=1)['tests/c/b.c']['got'] == 'unproven'   # unexplained nonzero rc
# named field: a named refusal counts 3 levels (agree+wrong+refuse+3*named == 3n) and reads as refuse
nm = '\n'.join(['  named-refuse a: not covered: x', 'difftest_o SHARD=1/1 probes=2  agree 3   wrong 0   refuse 0   known 0   revived 0   named 1   (x)'])
vn = P.verdicts(nm, rows[:2], rc=0)
assert vn['tests/c/a.c']['got'] == 'refuse' and vn['tests/c/b.c']['got'] == 'agree', vn
assert P.verdicts(nm.replace('named 1', 'named 0'), rows[:2], rc=0)['tests/c/b.c']['got'] == 'unproven'   # totals no longer add up
one = 'difftest_o SHARD=1/1 probes=1  agree 0   wrong 0   refuse 0   known 0   revived 0   named 1   (x)'
for bad in (one,                                                              # named count with no named line
            '  named-refuse zz: not covered: x\n' + one,                      # named line for a foreign probe
            '  named-refuse b: x\n  named-refuse b: x\n' + one.replace('named 1', 'named 2')):   # duplicate
    assert P.verdicts(bad, rows[1:2], rc=0)['tests/c/b.c']['got'] == 'unproven', bad
mix = '\n'.join(['  named-refuse a: not covered: x', '  WRONG a              -O2: got [1] cc -O2 says [2]',
                 'difftest_o SHARD=1/1 probes=2  agree 2   wrong 1   refuse 0   known 0   revived 0   named 1   (x)'])
vm = P.verdicts(mix, rows[:2], rc=1)
assert all(x['got'] == 'unproven' for x in vm.values()), vm         # named and WRONG for one probe: contradictory
dupl = '\n'.join(['  REFUSE a -O0: not covered: x', '  REFUSE a -O0: not covered: x', '  REFUSE a -O2: not covered: x',
                  'difftest_o SHARD=1/1 probes=1  agree 0   wrong 0   refuse 3   known 0   revived 0   (x)'])
assert P.verdicts(dupl, rows[:1], rc=1)['tests/c/a.c']['got'] == 'unproven'   # same level twice
# conservation: summary wrong>0 without the named line, or a line naming a foreign probe -> unproven
hidden = 'difftest_o SHARD=1/1 probes=1  agree 2   wrong 1   refuse 0   known 0   revived 0'
assert P.verdicts(hidden, rows[1:2], rc=1)['tests/c/b.c']['got'] == 'unproven'
foreign = '\n'.join(['  WRONG zz -O2: got [1] cc -O2 says [2]', hidden])
assert P.verdicts(foreign, rows[1:2], rc=1)['tests/c/b.c']['got'] == 'unproven'
refuse3 = '\n'.join(['  REFUSE a %s: not covered: x' % o for o in ('-O0', '-O1', '-O2')] + [three])
for bad_rc in (142, -9, 2):   # timeout, signal, usage: never a normal refusal
    assert P.verdicts(refuse3, rows[:1], rc=bad_rc)['tests/c/a.c']['got'] == 'unproven', bad_rc
# fail closed: no summary, wrong probe count, known entries or unaccounted totals -> unproven, never agree
for bad in ('', 'difftest_o SHARD=1/1 probes=3  agree 9   wrong 0   refuse 0   known 0   revived 0',
            'difftest_o SHARD=1/1 probes=4  agree 9   wrong 0   refuse 0   known 3   revived 0',
            'difftest_o SHARD=1/1 probes=4  agree 2   wrong 0   refuse 0   known 0   revived 0'):
    u = P.verdicts(bad, rows)
    assert all(x['got'] == 'unproven' and not x['ok'] for x in u.values()), (bad, u)
# an inherited PROBES never narrows a formal run: without WF1_PRECHECK=1 difftest_o keeps the whole shard
import os
full = subprocess.run(['bash', '-c', 'wf1_list_tmp=$(mktemp); sed -n "/^FILES=()/,/^done/p" tests/difftest_o.sh > "$wf1_list_tmp"; . "$wf1_list_tmp"; rm -f "$wf1_list_tmp"; echo ${#FILES[@]}'], cwd=ROOT,
                      env=dict(os.environ, SH_N='4', SH_K='1', PROBES='tests/c/fb12-31-unused-static-refs-undefined.c'), capture_output=True, text=True, check=True).stdout.strip()
narrow = subprocess.run(['bash', '-c', 'wf1_list_tmp=$(mktemp); sed -n "/^FILES=()/,/^done/p" tests/difftest_o.sh > "$wf1_list_tmp"; . "$wf1_list_tmp"; rm -f "$wf1_list_tmp"; echo ${#FILES[@]}'], cwd=ROOT,
                        env=dict(os.environ, SH_N='4', SH_K='1', WF1_PRECHECK='1', PROBES='tests/c/fb12-31-unused-static-refs-undefined.c'), capture_output=True, text=True, check=True).stdout.strip()
assert int(full) > 1 and narrow == '1', (full, narrow)
# shard map = the bash loop in difftest_o.sh (glob order, host skipped, i % 4)
bash = subprocess.run(['bash', '-c', 'i=0; for f in tests/c/*.c examples/*.c; do [ "$(basename "$f" .c)" = host ] && continue; echo "$f $((i % 4 + 1))"; i=$((i+1)); done'],
                      cwd=ROOT, capture_output=True, text=True, check=True).stdout.split('\n')
want = {k: v for k, v in (l.split() for l in bash if l) if (ROOT / k).exists()}   # an unmatched glob stays literal in bash
for f in list(want)[:40] + list(want)[-40:]:
    assert P.shard_of(f) == ['difftest_o-' + want[f], 'com-difftest_o-' + want[f]], (f, P.shard_of(f), want[f])
assert P.shard_of('tests/c/no-such-probe.c') is None
require_unchanged(index_before, subprocess.check_output(['git', 'ls-files', '-s', '-z'], cwd=ROOT))
require_unchanged(examples_before, input_snapshot(ROOT))
print('c99precheck  verdicts per probe (refuse needs all three -O), shard map equals difftest_o glob order, fail-closed without complete summary, inherited PROBES ignored')
