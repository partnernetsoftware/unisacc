"""UNS1 weight blobs. [Q-1] [Q-2] [Q-3]

header 16B : 'UNS1' | dtype u8 | flags u8 | nTensors u16 | nParams u32 | acc f32
tensor     : name[16] | rows u16 | cols u16 | scale f32 | payload | pad to 4
"""
import struct

MAGIC = b"UNS1"
DT_I8, DT_F16, DT_F32, DT_Q4, DT_Q2 = 0, 1, 2, 3, 4
DT_NAME = {DT_I8: "i8", DT_F16: "f16", DT_F32: "f32", DT_Q4: "q4", DT_Q2: "q2"}
NAME_DT = {v: k for k, v in DT_NAME.items()}


def _pad4(b):
    return b + b"\x00" * ((-len(b)) % 4)


def _quant_i8(w):
    m = max((abs(x) for x in w), default=0.0)
    s = (m / 127.0) if m > 0 else 1.0
    inv = 1.0 / s
    q = bytearray()
    for x in w:
        v = int(round(x * inv))
        v = -127 if v < -127 else (127 if v > 127 else v)
        q.append(v & 0xFF)
    return s, bytes(q)


def _f16(x):
    """round-to-nearest-even f16, as raw u16"""
    import math
    if x == 0.0:
        return 0
    sign = 0x8000 if math.copysign(1.0, x) < 0 else 0
    x = abs(x)
    if x >= 65504.0:
        return sign | 0x7BFF
    e = math.floor(math.log2(x))
    if e < -14:                                   # subnormal
        m = int(round(x / 2.0 ** -24))
        return sign | min(m, 0x3FF)
    m = int(round(x / 2.0 ** e * 1024)) - 1024
    if m > 1023:
        m = 0
        e += 1
    return sign | (((e + 15) & 0x1F) << 10) | (m & 0x3FF)


def _from_f16(h):
    s = -1.0 if h & 0x8000 else 1.0
    e = (h >> 10) & 0x1F
    m = h & 0x3FF
    if e == 0:
        return s * m * 2.0 ** -24
    return s * (1.0 + m / 1024.0) * 2.0 ** (e - 15)


def _rowscale(w, rows, cols, levels):
    """per-row scale = maxabs/levels  [Q-3] [Q-4]"""
    sc = []
    for r in range(rows):
        m = max((abs(x) for x in w[r * cols:(r + 1) * cols]), default=0.0)
        sc.append((m / levels) if m > 0 else 1.0)
    return sc


def _packsub(w, rows, cols, sc, levels, bits):
    """low bits first, row-major, each row starts on a byte boundary [Q-5]"""
    per = 8 // bits
    out = bytearray()
    for r in range(rows):
        inv = 1.0 / sc[r]
        acc = 0
        n = 0
        for c in range(cols):
            v = int(round(w[r * cols + c] * inv))
            v = max(-levels, min(levels, v))
            acc |= (v & ((1 << bits) - 1)) << (bits * n)
            n += 1
            if n == per:
                out.append(acc)
                acc, n = 0, 0
        if n:
            out.append(acc)
    return bytes(out)


def dumps(tensors, dtype=DT_F32, acc=0.0, flags=0):
    """tensors: [(name, rows, cols, list[float])]"""
    body = b""
    nparams = 0
    for (name, rows, cols, w) in tensors:
        nparams += len(w)
        if dtype == DT_F32:
            scale, payload = 1.0, struct.pack("<%df" % len(w), *w)
        elif dtype == DT_I8:
            scale, payload = _quant_i8(w)
        elif dtype == DT_F16:
            scale = 1.0
            payload = b"".join(struct.pack("<H", _f16(x)) for x in w)
        elif dtype in (DT_Q4, DT_Q2):
            levels, bits = (7, 4) if dtype == DT_Q4 else (1, 2)
            sc = _rowscale(w, rows, cols, levels)
            scale = max(sc)
            payload = b"".join(struct.pack("<f", x) for x in sc) + \
                _packsub(w, rows, cols, sc, levels, bits)
        else:
            raise NotImplementedError("dtype %d" % dtype)
        nm = name.encode()[:16].ljust(16, b"\x00")
        body += nm + struct.pack("<HHf", rows, cols, scale) + _pad4(payload)
    head = MAGIC + struct.pack("<BBHIf", dtype, flags, len(tensors),
                               nparams, acc)
    return head + body


def loads(blob):
    assert blob[:4] == MAGIC, "not UNS1"
    dtype, flags, nt, nparams, acc = struct.unpack("<BBHIf", blob[4:16])
    off = 16
    out = []
    for _ in range(nt):
        name = blob[off:off + 16].rstrip(b"\x00").decode()
        rows, cols, scale = struct.unpack("<HHf", blob[off + 16:off + 24])
        off += 24
        n = rows * cols
        if dtype == DT_F32:
            w = list(struct.unpack("<%df" % n, blob[off:off + 4 * n]))
            raw = 4 * n
        elif dtype == DT_I8:
            w = [struct.unpack("<b", blob[off + i:off + i + 1])[0] * scale
                 for i in range(n)]
            raw = n
        elif dtype == DT_F16:
            w = [_from_f16(struct.unpack("<H", blob[off + 2 * i:off + 2 * i + 2])[0])
                 for i in range(n)]
            raw = 2 * n
        elif dtype in (DT_Q4, DT_Q2):
            bits = 4 if dtype == DT_Q4 else 2
            per = 8 // bits
            sc = [struct.unpack("<f", blob[off + 4 * r:off + 4 * r + 4])[0]
                  for r in range(rows)]
            q = off + 4 * rows
            w = []
            rb = (cols + per - 1) // per
            for r in range(rows):
                for c in range(cols):
                    byte = blob[q + r * rb + c // per]
                    v = (byte >> (bits * (c % per))) & ((1 << bits) - 1)
                    if v >= (1 << (bits - 1)):
                        v -= (1 << bits)
                    w.append(v * sc[r])
            raw = 4 * rows + rb * rows
        else:
            raise NotImplementedError
        off += raw + ((-raw) % 4)
        out.append((name, rows, cols, w))
    return {"dtype": dtype, "flags": flags, "acc": acc, "tensors": out}


def save_net(net, path, dtype=DT_F32):
    ts = [(t.name, t.rows, t.cols, t.w) for t in net.T]
    blob = dumps(ts, dtype=dtype, acc=net.acc)
    with open(path, "wb") as f:
        f.write(blob)
    return len(blob)


def load_into(net, path):
    with open(path, "rb") as f:
        d = loads(f.read())
    assert len(d["tensors"]) == len(net.T)
    for t, (name, rows, cols, w) in zip(net.T, d["tensors"]):
        assert t.name == name and t.rows == rows and t.cols == cols, \
            "shape mismatch %s" % name
        t.w = list(w)
    net.acc = d["acc"]
    return net
