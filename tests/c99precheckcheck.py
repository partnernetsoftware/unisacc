#!/usr/bin/env python3
"""c99precheck controls (0.0.39 WF1): per-probe verdicts from difftest_o lines (refuse only when all three
-O levels refuse; any WRONG/FAIL is 'other'); the shard map follows difftest_o.sh's own bash glob order."""
import importlib.util, pathlib, subprocess
ROOT = pathlib.Path(__file__).resolve().parents[1]
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
assert dup['tests/c/a.c']['got'] == 'other', dup
ok3 = P.verdicts('\n'.join(['  REFUSE a %s: not covered: x' % o for o in ('-O0', '-O1', '-O2')] + [three]), rows[:1], rc=1)
assert ok3['tests/c/a.c']['got'] == 'refuse', ok3
agree1 = 'difftest_o SHARD=1/1 probes=1  agree 3   wrong 0   refuse 0   known 0   revived 0'
assert P.verdicts(agree1, rows[1:2], rc=0)['tests/c/b.c']['got'] == 'agree'
assert P.verdicts(agree1, rows[1:2], rc=1)['tests/c/b.c']['got'] == 'unproven'   # unexplained nonzero rc
# fail closed: no summary, wrong probe count, known entries or unaccounted totals -> unproven, never agree
for bad in ('', 'difftest_o SHARD=1/1 probes=3  agree 9   wrong 0   refuse 0   known 0   revived 0',
            'difftest_o SHARD=1/1 probes=4  agree 9   wrong 0   refuse 0   known 3   revived 0',
            'difftest_o SHARD=1/1 probes=4  agree 2   wrong 0   refuse 0   known 0   revived 0'):
    u = P.verdicts(bad, rows)
    assert all(x['got'] == 'unproven' and not x['ok'] for x in u.values()), (bad, u)
# an inherited PROBES never narrows a formal run: without WF1_PRECHECK=1 difftest_o keeps the whole shard
import os
full = subprocess.run(['bash', '-c', 'source <(sed -n "/^FILES=()/,/^done/p" tests/difftest_o.sh); echo ${#FILES[@]}'], cwd=ROOT,
                      env=dict(os.environ, SH_N='4', SH_K='1', PROBES='tests/c/fb12-31-unused-static-refs-undefined.c'), capture_output=True, text=True).stdout.strip()
narrow = subprocess.run(['bash', '-c', 'source <(sed -n "/^FILES=()/,/^done/p" tests/difftest_o.sh); echo ${#FILES[@]}'], cwd=ROOT,
                        env=dict(os.environ, SH_N='4', SH_K='1', WF1_PRECHECK='1', PROBES='tests/c/fb12-31-unused-static-refs-undefined.c'), capture_output=True, text=True).stdout.strip()
assert int(full) > 1 and narrow == '1', (full, narrow)
# shard map = the bash loop in difftest_o.sh (glob order, host skipped, i % 4)
bash = subprocess.run(['bash', '-c', 'i=0; for f in tests/c/*.c examples/*.c; do [ "$(basename "$f" .c)" = host ] && continue; echo "$f $((i % 4 + 1))"; i=$((i+1)); done'],
                      cwd=ROOT, capture_output=True, text=True, check=True).stdout.split('\n')
want = dict(l.split() for l in bash if l)
for f in list(want)[:40] + list(want)[-40:]:
    assert P.shard_of(f) == ['difftest_o-' + want[f], 'com-difftest_o-' + want[f]], (f, P.shard_of(f), want[f])
assert P.shard_of('tests/c/no-such-probe.c') is None
print('c99precheck  verdicts per probe (refuse needs all three -O), shard map equals difftest_o glob order, fail-closed without complete summary, inherited PROBES ignored')
