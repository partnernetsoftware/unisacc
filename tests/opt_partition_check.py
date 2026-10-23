#!/usr/bin/env python3
"""Static partition and failure controls; no compiler build or product suite."""
from pathlib import Path
import subprocess
import sys

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
    for shell, name in [('bash', 'opt')]:
        subprocess.run([shell, '-n', str(ROOT / ('tests/' + name + '.sh'))], check=True, timeout=3)
    print('opt partitions: full input union; syntax ok')


if __name__ == '__main__':
    try:
        main()
    except (AssertionError, OSError, subprocess.SubprocessError) as exc:
        print('opt partition FAIL: ' + str(exc), file=sys.stderr)
        sys.exit(1)
