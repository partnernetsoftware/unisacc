"""UNS2 -- serialized constructed integer nets. [K-5] [E-22]

UNS1 stores dense tensors.  A constructed net is not dense: W1 is binary and a
unit is fully described by one literal bitmask per key field, b1 is recoverable
from those masks, and W2 is sparse small integers.  Storing it as a tensor
would be storing mostly zeros.

    header 16B : 'UNS2' | ver u8 | flags u8 | nStages u16 | nUnits u32 | pad u32
    per stage  : name[16] | h0 u16 | H u16 | nFields u8 | nHeads u8 | pad u16
                 fieldLens u16[nFields]
                 W1  : H * h0 bits, unit-major (the literal masks)
                 per head: nClasses u16 | wBits u8 | pad u8 | payloadLen u32
                           payload: for each of the H units, an 8-bit count of
                           its entries, then that many (classIdx, weight)
                           packed at ceil(log2(nCls)) + wBits bits
                           (an earlier note said "presence H bits": the code
                           has always written the 8-bit count)

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
            ws = [w for row in n.w2[hn] for (_, w) in row] or [1]
            wb = max(1, max(ws).bit_length())
            cb = _w(len(cl))
            hb = BitW()
            for j in range(n.H):
                row = n.w2[hn][j]
                # a factored head can map one unit to many classes -- isel
                # reaches 41, so a 4-bit count silently truncates
                assert len(row) < 256, "%s: unit %d has %d entries" % (
                    hn, j, len(row))
                hb.put(len(row), 8)
                for (k, w) in row:
                    hb.put(k, cb)
                    hb.put(w, wb)
            payload = hb.done()
            heads += struct.pack("<HBB", len(cl), wb, 0) + \
                struct.pack("<I", len(payload)) + payload
        body += name.encode()[:16].ljust(16, b"\x00")
        body += struct.pack("<HHBBH", h0, n.H, len(flens), len(n.heads), 0)
        body += b"".join(struct.pack("<H", x) for x in flens)
        body += struct.pack("<I", len(w1)) + w1 + heads
    head = MAGIC + struct.pack("<BBHII", VERSION, 0, len(order),
                               total_units, 0)
    return head + body


def load(blob, stages):
    """Rebuild IntNets from a UNS2 image.  The round trip is what makes the
    shipped artifact verifiable rather than merely well-formed."""
    from .intnet import IntNet
    assert blob[:4] == MAGIC, "not UNS2"
    ver, flags, nst, nunits, _ = struct.unpack("<BBHII", blob[4:16])
    off = 16
    out = {}
    for _ in range(nst):
        name = blob[off:off + 16].rstrip(b"\x00").decode()
        h0, H, nf, nh, _ = struct.unpack("<HHBBH", blob[off + 16:off + 24])
        off += 24
        flens = [struct.unpack("<H", blob[off + 2 * i:off + 2 * i + 2])[0]
                 for i in range(nf)]
        off += 2 * nf
        w1len = struct.unpack("<I", blob[off:off + 4])[0]
        off += 4
        br = BitR(blob[off:off + w1len])
        feeds = [[] for _ in range(h0)]
        for j in range(H):
            for c in range(h0):
                if br.get(1):
                    feeds[c].append(j)
        off += w1len
        S = stages[name]
        n = IntNet.__new__(IntNet)
        n.stage = name
        n.H = H
        n.feeds = feeds
        o = 0
        n.offs = []
        for L in flens:
            n.offs.append(o)
            o += L
        n.heads = []
        n.w2 = {}
        n.ncls = {}
        for (hn, cl, _sh) in S.heads:
            ncl, wb, _ = struct.unpack("<HBB", blob[off:off + 4])
            hlen = struct.unpack("<I", blob[off + 4:off + 8])[0]
            off += 8
            hb = BitR(blob[off:off + hlen])
            off += hlen
            cb = _w(ncl)
            rows = []
            for j in range(H):
                cnt = hb.get(8)
                rows.append([(hb.get(cb), hb.get(wb)) for _ in range(cnt)])
            n.heads.append((hn, list(cl)))
            n.w2[hn] = rows
            n.ncls[hn] = ncl
        # b1 is NOT stored: it is -(fields the unit tests - 1)  [K-5]
        fields, base = [], 0
        for L in flens:
            fields.append((base, L))
            base += L
        touched = [set() for _ in range(H)]
        for fi, (b, L) in enumerate(fields):
            for i in range(L):
                for j in feeds[b + i]:
                    touched[j].add(fi)
        n.b1 = [-(len(touched[j]) - 1) for j in range(H)]
        n.vidx = [{v: i for i, v in enumerate(vo)} for (_, vo) in S.fields]
        n.exact = None
        n.maxlogit = None
        out[name] = n
    return out


def size_report(nets, stages):
    """Per-stage byte cost, for the size table."""
    out = {}
    for name in sorted(nets):
        one = dump({name: nets[name]}, stages)
        out[name] = len(one) - 16          # minus the shared header
    return out
