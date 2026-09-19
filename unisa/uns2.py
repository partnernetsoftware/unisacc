"""UNS2 -- serialized constructed integer nets. [K-5] [E-22]

UNS1 stores dense tensors.  A constructed net is not dense: W1 is binary and a
unit is fully described by one literal bitmask per key field, b1 is recoverable
from those masks, and W2 is sparse small integers.  Storing it as a tensor
would be storing mostly zeros.

    header 16B : 'UNS2' | ver u8 | flags u8 | nStages u16 | nUnits u32 | pad u32
    per stage  : name[16] | h0 u16 | H u16 | nFields u8 | nHeads u8 | pad u16
                 fieldLens u16[nFields]
                 W1  : H * h0 bits, unit-major (the literal masks)
                 per head: nClasses u16 | wBits u8 | pad u8
                           presence H bits, then per present unit
                           (classIdx, weight) packed at ceil(log2(nCls)) + wBits

Everything is byte-reproducible: no timestamps, no dict iteration order. [D-5]
"""
import struct

MAGIC = b"UNS2"
VERSION = 1


class BitW:
    __slots__ = ("b", "acc", "n")

    def __init__(self):
        self.b, self.acc, self.n = bytearray(), 0, 0

    def put(self, val, bits):
        for i in range(bits - 1, -1, -1):
            self.acc = (self.acc << 1) | ((val >> i) & 1)
            self.n += 1
            if self.n == 8:
                self.b.append(self.acc)
                self.acc, self.n = 0, 0

    def done(self):
        if self.n:
            self.b.append(self.acc << (8 - self.n))
            self.acc, self.n = 0, 0
        return bytes(self.b)


class BitR:
    __slots__ = ("b", "i")

    def __init__(self, b):
        self.b, self.i = b, 0

    def get(self, bits):
        v = 0
        for _ in range(bits):
            byte = self.b[self.i >> 3]
            v = (v << 1) | ((byte >> (7 - (self.i & 7))) & 1)
            self.i += 1
        return v


def _w(n):
    return max(1, (n - 1).bit_length())


def dump(nets, stages):
    """nets: {name: IntNet}; stages: gold STAGES (for field vocab sizes)."""
    order = sorted(nets)
    body = b""
    total_units = 0
    for name in order:
        n = nets[name]
        S = stages[name]
        flens = [len(v) for (_, v) in S.fields]
        h0 = sum(flens)
        total_units += n.H
        bw = BitW()
        # W1: literal mask per unit, unit-major
        cols = [[0] * h0 for _ in range(n.H)]
        for c in range(h0):
            for j in n.feeds[c]:
                cols[j][c] = 1
        for j in range(n.H):
            for c in range(h0):
                bw.put(cols[j][c], 1)
        w1 = bw.done()
        heads = b""
        for hn, cl in n.heads:
            wmax = max([w for row in n.w2[hn] for (_, w) in row] or [1])
            wb = max(1, wmax.bit_length())
            cb = _w(len(cl))
            hb = BitW()
            for j in range(n.H):
                hb.put(1 if n.w2[hn][j] else 0, 1)
            for j in range(n.H):
                for (k, w) in n.w2[hn][j]:
                    hb.put(k, cb)
                    hb.put(w, wb)
                if len(n.w2[hn][j]) > 1:
                    pass
            heads += struct.pack("<HBB", len(cl), wb, 0) + \
                struct.pack("<I", len(hb.b)) + hb.done()
        body += name.encode()[:16].ljust(16, b"\x00")
        body += struct.pack("<HHBBH", h0, n.H, len(flens), len(n.heads), 0)
        body += b"".join(struct.pack("<H", x) for x in flens)
        body += struct.pack("<I", len(w1)) + w1 + heads
    head = MAGIC + struct.pack("<BBHII", VERSION, 0, len(order),
                               total_units, 0)
    return head + body


def size_report(nets, stages):
    """Per-stage byte cost, for the size table."""
    out = {}
    for name in sorted(nets):
        one = dump({name: nets[name]}, stages)
        out[name] = len(one) - 16          # minus the shared header
    return out
