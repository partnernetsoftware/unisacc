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
                  '  REFUSE c -O0: not covered: y', '  WRONG d              -O2: got [1] cc -O2 says [2]', 'difftest_o ...'])
v = P.verdicts(text, rows)
assert [v[r['probe']]['got'] for r in rows] == ['refuse', 'agree', 'other', 'other'], v
assert [v[r['probe']]['ok'] for r in rows] == [True, True, False, False], v
# shard map = the bash loop in difftest_o.sh (glob order, host skipped, i % 4)
bash = subprocess.run(['bash', '-c', 'i=0; for f in tests/c/*.c examples/*.c; do [ "$(basename "$f" .c)" = host ] && continue; echo "$f $((i % 4 + 1))"; i=$((i+1)); done'],
                      cwd=ROOT, capture_output=True, text=True, check=True).stdout.split('\n')
want = dict(l.split() for l in bash if l)
for f in list(want)[:40] + list(want)[-40:]:
    assert P.shard_of(f) == ['difftest_o-' + want[f], 'com-difftest_o-' + want[f]], (f, P.shard_of(f), want[f])
assert P.shard_of('tests/c/no-such-probe.c') is None
print('c99precheck  verdicts per probe (refuse needs all three -O), shard map equals difftest_o glob order')
