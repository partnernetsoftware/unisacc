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
        else:
            raise NotImplementedError("dtype %s lands in M2" % dtype)
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
