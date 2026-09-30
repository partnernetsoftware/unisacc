"""Export and audit the frozen tapebin v1 opcode/operand registry."""

import pathlib
import sys

from .tape import SHAPE


ROOT = pathlib.Path(__file__).resolve().parents[1]
SNAPSHOT = ROOT / "docs" / "tapebin-v1.shape.tsv"
CHEADER = ROOT / "src" / "tapebin_shape.inc"


def render():
    return "opcode\tname\tshape\n" + "".join(
        "%d\t%s\t%s\n" % (i, name, "".join(shape))
        for i, (name, shape) in enumerate(SHAPE.items())
    )


def render_c():
    rows = tuple(SHAPE.items())
    return ("/* Generated from unisa/tape.py SHAPE; do not edit. */\n"
            "#define TB_OPCOUNT %d\n" % len(rows)
            + "char *tb_opnames[TB_OPCOUNT] = {" + ", ".join('"%s"' % name for name, _ in rows) + "};\n"
            + "char *tb_shapes[TB_OPCOUNT] = {" + ", ".join('"%s"' % "".join(shape) for _, shape in rows) + "};\n")


def main():
    want = render()
    if sys.argv[1:] == ["--write"]:
        SNAPSHOT.write_text(want, encoding="ascii")
        CHEADER.write_text(render_c(), encoding="ascii")
    elif sys.argv[1:] == ["--check"]:
        if SNAPSHOT.read_text(encoding="ascii") != want:
            raise SystemExit("tapebin shape snapshot is stale")
        if CHEADER.read_text(encoding="ascii") != render_c():
            raise SystemExit("tapebin C shape table is stale")
        print("tapebin shape: %d opcodes" % len(SHAPE))
    else:
        raise SystemExit("usage: python3 -m unisa.tapebin_shape --write|--check")


if __name__ == "__main__":
    main()
