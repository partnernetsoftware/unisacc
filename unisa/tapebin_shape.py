"""Export and audit the frozen tapebin v1 opcode/operand registry."""

import pathlib
import sys

from .tape import SHAPE


ROOT = pathlib.Path(__file__).resolve().parents[1]
SNAPSHOT = ROOT / "docs" / "tapebin-v1.shape.tsv"


def render():
    return "opcode\tname\tshape\n" + "".join(
        "%d\t%s\t%s\n" % (i, name, "".join(shape))
        for i, (name, shape) in enumerate(SHAPE.items())
    )


def main():
    want = render()
    if sys.argv[1:] == ["--write"]:
        SNAPSHOT.write_text(want, encoding="ascii")
    elif sys.argv[1:] == ["--check"]:
        if SNAPSHOT.read_text(encoding="ascii") != want:
            raise SystemExit("tapebin shape snapshot is stale")
        print("tapebin shape: %d opcodes" % len(SHAPE))
    else:
        raise SystemExit("usage: python3 -m unisa.tapebin_shape --write|--check")


if __name__ == "__main__":
    main()
