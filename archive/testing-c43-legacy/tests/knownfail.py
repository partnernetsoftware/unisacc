"""Shared known-failure ledger parser and pass/revival contract.

Each non-comment line starts with a unique probe name and a reason.  A listed
probe is tolerated only while it fails; success is a revived failure.
"""

from pathlib import Path
import sys


def read(path):
    entries = {}
    for number, raw in enumerate(Path(path).read_text().splitlines(), 1):
        line = raw.strip()
        if not line or line.startswith('#'):
            continue
        parts = line.split(maxsplit=1)
        if len(parts) != 2 or parts[0] in entries:
            raise ValueError(f'{path}:{number}: malformed or duplicate knownfail entry')
        entries[parts[0]] = parts[1]
    return entries


def verdict(entries, name, passed):
    """Return pass, fail, known or revived for one measured probe."""
    return ('revived' if passed else 'known') if name in entries else ('pass' if passed else 'fail')


def main(argv):
    if len(argv) < 3 or argv[1] != 'keys':
        raise SystemExit('usage: knownfail.py keys LEDGER...')
    names = set()
    for path in argv[2:]:
        names.update(read(path))
    for name in sorted(names):
        print(name)


if __name__ == '__main__':
    main(sys.argv)
