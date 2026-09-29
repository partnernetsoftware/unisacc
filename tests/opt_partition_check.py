#!/usr/bin/env python3
"""Static partition and failure controls; no compiler build or product suite."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parent.parent


def main():
    for paths in [('examples/*.c', 'tests/c/*.c'),
                  ('examples/*.c', 'tests/c/*.c', 'tests/c99/*.c',
                   'corpus/c-testsuite/tests/single-exec/*.c')]:
        files = [str(p.relative_to(ROOT)) for pat in paths for p in sorted(ROOT.glob(pat))]
        chunks = [files[k::4] for k in range(4)]
        assert all(chunks)
        assert sorted(f for chunk in chunks for f in chunk) == sorted(files)
        assert len({f for chunk in chunks for f in chunk}) == len(files)
    folded = 'hello fact ptr fib switch struct do'.split()
    assert sorted(f for k in range(4) for f in folded[k::4]) == sorted(folded)
    opt = (ROOT / 'tests/opt.sh').read_text()
    assert 'all|run|closure|self' in opt and 'pick || continue' in opt
    assert 'tape build failed or empty' in opt and 'empty image' in opt
    assert 'bound 15 "$T/o2x"' in opt
    script = (ROOT / 'tests/optpy.sh').read_text().split("<<'PY'\n", 1)[1].rsplit('\nPY', 1)[0]
    compile(script, 'optpy-inline', 'exec')
    # Both optimisation levels printing identical bytes and failing must fail.
    with tempfile.TemporaryDirectory(prefix='unisacc-opt-plan-') as d:
        temp = Path(d)
        helper = temp / 'bound'
        helper.write_text('#!/bin/sh\nshift\nexec "$@"\n')
        helper.chmod(0o755)
        ua = temp / 'ua'
        ua.write_text('#!/bin/sh\nprintf "same tape\\n"\nexit 2\n')
        ua.chmod(0o755)
        env = dict(os.environ, OPTPY_PART='self', SHARD='1/1')
        # optpy.sh passes UA, the bound helper and the self source (third argument).
        self_source = temp / 'self.c'
        self_source.write_text('int main(void){return 0;}\n')
        proc = subprocess.run([sys.executable, '-', str(ua), str(helper), str(self_source)],
                              input=script, text=True, capture_output=True,
                              cwd=ROOT, env=env, timeout=5)
        assert proc.returncode != 0 and 'rc=2' in proc.stderr, proc.stderr
        env['SHARD'] = '2/1'
        proc = subprocess.run([sys.executable, '-', str(ua), str(helper), str(self_source)],
                              input=script, text=True, capture_output=True,
                              cwd=ROOT, env=env, timeout=5)
        assert proc.returncode != 0 and 'invalid SHARD' in proc.stderr
    accept = (ROOT / 'tests/acceptance.sh').read_text()
    for case in ('6-build', '6-fold', '6-fault', '6-image', '10-Btape',
                 '10-Bimage', '10-Ctape', '10-Uimage', '10-compare'):
        assert case in accept
    assert 'set(stages)==set(ALL)' in accept
    assert 'bootstrap state missing/stale' in accept
    assert 'cmp -s "$S/B.tape" "$S/C.tape" && cmp -s "$S/B.tape" "$S/U.tape"' in accept
    for shell, name in [('sh', 'acceptance'), ('bash', 'opt'), ('bash', 'optpy')]:
        subprocess.run([shell, '-n', str(ROOT / ('tests/' + name + '.sh'))], check=True, timeout=3)
    print('opt/acceptance partitions: full input union; syntax and shared-failure/invalid-shard controls ok')


if __name__ == '__main__':
    try:
        main()
    except (AssertionError, OSError, subprocess.SubprocessError) as exc:
        print('opt partition FAIL: ' + str(exc), file=sys.stderr)
        sys.exit(1)
