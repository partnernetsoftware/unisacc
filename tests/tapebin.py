"""Structural and byte-stability checks for the v1 Python codec."""

import pathlib
import subprocess
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))

from unisa.tape import parse
from unisa.tapebin import decode, encode


def check(text):
    original = parse(text)
    first = encode(original)
    if first != encode(original):
        raise AssertionError("nondeterministic encoding")
    restored = decode(first)
    if original.records != restored.records:
        raise AssertionError("record order or value changed")
    if encode(restored) != first:
        raise AssertionError("binary roundtrip changed")
    if parse(restored.to_canonical_text()).records != original.records:
        raise AssertionError("canonical print changed records")
    for damaged in (first[:-1], first[:64] + bytes([first[64] ^ 1]) + first[65:]):
        try:
            decode(damaged)
        except ValueError:
            pass
        else:
            raise AssertionError("damaged package accepted")


def main():
    check('L:\n  nop\n.bss z 24\n.str a "x"\nL:\n.str a "y"\n  call L\n')
    files = sorted(pathlib.Path("examples").glob("*.c"))[:10]
    if not files:
        raise AssertionError("no C inputs")
    for path in files:
        tape = subprocess.check_output([sys.executable, "-m", "unisa", "tape", str(path)])
        check(tape.decode("latin-1"))
    print("tapebin Python codec: %d real tapes + ordered-record probe" % len(files))


if __name__ == "__main__":
    main()
