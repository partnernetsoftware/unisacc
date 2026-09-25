"""Compare a whole UNS2 pack with a reference pack, header included. [J10]

    python3 iterate/construct/tools/uns2pack.py mine.uns2 weights/built.uns2 [N]

A test oracle only; nothing here feeds construct.c.  Both files are walked by
the format's own length fields (as uns2slice.py does).  Checked, each on its
own line, then the raw bytes:

  - magic "UNS2" and the version bytes;
  - nStages (u16 at 6), equal in both and, with N given, equal to N;
  - nUnits (u32 at 8), equal in both and equal to the sum of H over the
    sections of each file;
  - the reserved u32 at 12;
  - the section names in file order: sorted (as uns2.dump's sorted(nets))
    and the same sequence in both;
  - no trailing bytes, and the full file length equal;
  - finally the whole file, byte for byte (header included).
Exit 1 on the first class of difference found (all lines are printed).
"""
import struct
import sys


def walk(blob):
    if blob[:4] != b"UNS2":
        raise SystemExit("uns2pack: not UNS2")
    ver = blob[4:6]
    nst, units, resv = struct.unpack("<HII", blob[6:16])
    off, names, hs = 16, [], 0
    for _ in range(nst):
        name = blob[off:off + 16].rstrip(b"\x00").decode()
        _h0, H, nf, nh, _ = struct.unpack("<HHBBH", blob[off + 16:off + 24])
        off += 24 + 2 * nf
        off += 4 + struct.unpack("<I", blob[off:off + 4])[0]
        for _ in range(nh):
            off += 8 + struct.unpack("<I", blob[off + 4:off + 8])[0]
        names.append(name)
        hs += H
    return ver, nst, units, resv, names, hs, off


def main(argv):
    a, b = open(argv[0], "rb").read(), open(argv[1], "rb").read()
    want = int(argv[2]) if len(argv) > 2 else None
    va, na, ua, ra, sa, ha, ea = walk(a)
    vb, nb, ub, rb, sb, hb, eb = walk(b)
    bad = 0

    def chk(ok, what):
        nonlocal bad
        bad += not ok
        print("pack %s: %s" % ("ok" if ok else "DIFFERS", what))
    chk(va == vb == b"\x01\x00", "version %s / %s" % (va.hex(), vb.hex()))
    chk(na == nb and (want is None or na == want),
        "nStages %d / %d%s" % (na, nb, "" if want is None else " (want %d)" % want))
    chk(ua == ub and ua == ha and ub == hb,
        "nUnits %d / %d, sum of section H %d / %d" % (ua, ub, ha, hb))
    chk(ra == rb == 0, "reserved %d / %d" % (ra, rb))
    chk(sa == sorted(sa) and sa == sb,
        "stage order sorted and equal: %s" % " ".join(sa))
    chk(set(sa) == set(sb), "name set equal (%d names)" % len(set(sa)))
    chk(ea == len(a) and eb == len(b) and len(a) == len(b),
        "length %d / %d B, no trailing bytes" % (len(a), len(b)))
    chk(a == b, "raw bytes identical, header included (%d B)" % len(a))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
