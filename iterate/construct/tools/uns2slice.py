"""The UNS2 blob of a subset of stages, from the Python constructor. [J10]

    python3 iterate/construct/tools/uns2slice.py out.uns2 weights/gold/prec.tsv ...
    python3 iterate/construct/tools/uns2slice.py --shipped blob.uns2 weights/built.uns2

The first form builds each stage with intnet.build(stages=tsvgold.load_all(...))
and writes uns2.dump(nets_subset, reg): the reference construct.c -u must
equal byte for byte.

The second form checks every stage section of `blob.uns2` against the section
of the same name inside the shipped pack.  Sections are located by walking the
format's own length fields (name, dims, flens, w1len, per head the payload
length), not by decoding; a stage section is self-contained, so its bytes are
independent of which other stages share the blob.  The 16-byte header is NOT
compared here (it carries nStages and nUnits, which differ by construction).
"""
import os
import struct
import sys

R = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(
    os.path.abspath(__file__)))))
sys.path.insert(0, R)
from unisa import intnet, tsvgold, uns2  # noqa: E402


def sections(blob):
    assert blob[:4] == b"UNS2", "not UNS2"
    nst = struct.unpack("<H", blob[6:8])[0]
    off, out = 16, {}
    for _ in range(nst):
        start = off
        name = blob[off:off + 16].rstrip(b"\x00").decode()
        _h0, _H, nf, nh, _ = struct.unpack("<HHBBH", blob[off + 16:off + 24])
        off += 24 + 2 * nf
        off += 4 + struct.unpack("<I", blob[off:off + 4])[0]
        for _ in range(nh):
            off += 8 + struct.unpack("<I", blob[off + 4:off + 8])[0]
        out[name] = blob[start:off]
    assert off == len(blob), "trailing bytes"
    return out


def main(argv):
    if argv[0] == "--shipped":
        mine = sections(open(argv[1], "rb").read())
        ship = sections(open(argv[2], "rb").read())
        bad = 0
        for name, b in sorted(mine.items()):
            same = ship.get(name) == b
            bad += not same
            print("section %s  %d B  shipped %s B  %s" % (
                name, len(b), len(ship[name]) if name in ship else "-",
                "IDENTICAL" if same else "DIFFERENT"))
        return 1 if bad else 0
    out, paths = argv[0], argv[1:]
    d = os.path.dirname(paths[0]) or "."
    names = [os.path.basename(p)[:-len(".tsv")] for p in paths]
    reg = tsvgold.load_all(d, names)
    nets = {n: intnet.build(n, stages=reg) for n in names}
    blob = uns2.dump(nets, reg)
    with open(out, "wb") as f:
        f.write(blob)
    print("%s  %d stages  %d B" % (out, len(nets), len(blob)))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
