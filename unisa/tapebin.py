"""Canonical v1 binary container for semantic tape records.

This codec is deliberately independent of the six image writers.  It
serialises the ordered records in :mod:`unisa.tape`; the header records the
target chosen by the C front end, while the tape instruction set stays neutral.
"""

import hashlib
import struct

from .tape import REGS, SHAPE, Tape, parse


MAGIC = b"UTAPEBIN"
HEADER = struct.Struct("<8s4H3I32sI")
ENTRY = struct.Struct("<HHQQI")
OPS = tuple(SHAPE)
MAX_FILE = 1 << 30
TARGETS = ("lnx/x86_64", "lnx/arm64", "osx/x86_64", "osx/arm64",
           "win/x86_64", "win/arm64")


def target_id(name):
    try:
        return TARGETS.index(name) + 1
    except ValueError:
        raise ValueError("tapebin: unknown target " + name) from None


def check_origin(tape, target, force=False):
    origin = getattr(tape, "origin_target", 0)
    if origin and origin != target_id(target) and not force:
        raise ValueError("tapebin: origin target differs; use --force-origin to override")


def _u(value):
    if not 0 <= value <= (1 << 64) - 1:
        raise ValueError("tapebin: integer out of range")
    out = bytearray()
    while value >= 128:
        out.append((value & 127) | 128)
        value >>= 7
    out.append(value)
    return bytes(out)


def _s(value):
    if not -(1 << 63) <= value < (1 << 63):
        raise ValueError("tapebin: signed integer out of range")
    out = bytearray()
    while True:
        byte = value & 127
        value >>= 7
        done = (value == 0 and byte < 64) or (value == -1 and byte >= 64)
        out.append(byte if done else byte | 128)
        if done:
            return bytes(out)


class _Reader:
    def __init__(self, data):
        self.data = data
        self.pos = 0

    def take(self, count):
        if count < 0 or count > len(self.data) - self.pos:
            raise ValueError("tapebin: truncated input")
        value = self.data[self.pos:self.pos + count]
        self.pos += count
        return value

    def byte(self):
        return self.take(1)[0]

    def uint(self):
        start = self.pos
        value = 0
        for shift in range(0, 70, 7):
            byte = self.byte()
            value |= (byte & 127) << shift
            if byte < 128:
                if value > (1 << 64) - 1 or self.data[start:self.pos] != _u(value):
                    raise ValueError("tapebin: noncanonical ULEB128")
                return value
        raise ValueError("tapebin: ULEB128 overflow")

    def sint(self):
        start = self.pos
        value = 0
        for shift in range(0, 70, 7):
            byte = self.byte()
            value |= (byte & 127) << shift
            if byte < 128:
                width = shift + 7
                if byte & 64:
                    value -= 1 << width
                if not -(1 << 63) <= value < (1 << 63) or self.data[start:self.pos] != _s(value):
                    raise ValueError("tapebin: noncanonical SLEB128")
                return value
        raise ValueError("tapebin: SLEB128 overflow")

    def done(self):
        if self.pos != len(self.data):
            raise ValueError("tapebin: section has trailing bytes")


def _name_bytes(name):
    data = name.encode("latin-1")
    if not data or any(c in b"\x00 \t\r\n:" for c in data):
        raise ValueError("tapebin: invalid name")
    return data


def _put_pool(values):
    return _u(len(values)) + b"".join(_u(len(v)) + v for v in values)


def _get_pool(data, count):
    reader = _Reader(data)
    n = reader.uint()
    if n != count or n > len(data):
        raise ValueError("tapebin: pool count mismatch")
    values = [reader.take(reader.uint()) for _ in range(n)]
    reader.done()
    return values


def _collect(records):
    names, consts = [], []
    name_idx, const_idx = {}, {}

    def name(value):
        raw = _name_bytes(value)
        if raw not in name_idx:
            name_idx[raw] = len(names)
            names.append(raw)
        return name_idx[raw]

    def const(value):
        raw = bytes(value)
        if raw not in const_idx:
            const_idx[raw] = len(consts)
            consts.append(raw)
        return const_idx[raw]

    for rec in records:
        kind = rec[0]
        if kind in ("label", "str", "bss"):
            name(rec[1])
            if kind == "str":
                const(rec[2])
        elif kind == "insn":
            op, args = rec[1:]
            shape = SHAPE[op]
            if len(shape) != len(args):
                raise ValueError("tapebin: wrong operand count")
            for typ, arg in zip(shape, args):
                if typ in ("L", "s") and isinstance(arg, str):
                    name(arg)
        else:
            raise ValueError("tapebin: unknown record")
    return names, consts, name_idx, const_idx


def _encode_records(records, name_idx, const_idx):
    out = bytearray(_u(len(records)))
    for rec in records:
        kind = rec[0]
        if kind == "label":
            out.append(0)
            out.extend(_u(name_idx[_name_bytes(rec[1])]))
        elif kind == "str":
            out.append(1)
            out.extend(_u(name_idx[_name_bytes(rec[1])]))
            out.extend(_u(const_idx[bytes(rec[2])]))
        elif kind == "bss":
            out.append(2)
            out.extend(_u(name_idx[_name_bytes(rec[1])]))
            out.extend(_u(rec[2]))
        elif kind == "insn":
            op, args = rec[1:]
            shape = SHAPE[op]
            out.extend((3, OPS.index(op)))
            regs = [REGS.index(arg) for typ, arg in zip(shape, args) if typ == "r"]
            for i in range(0, len(regs), 2):
                out.append(regs[i] | ((regs[i + 1] if i + 1 < len(regs) else 15) << 4))
            for typ, arg in zip(shape, args):
                if typ == "i":
                    value = int(arg)
                    if value < (1 << 63):
                        out.append(0)
                        out.extend(_s(value))
                    else:
                        out.append(1)
                        out.extend(_u(value))
                elif typ in ("L", "s"):
                    if isinstance(arg, str):
                        out.append(1)
                        out.extend(_u(name_idx[_name_bytes(arg)]))
                    else:
                        out.append(0)
                        out.extend(_s(int(arg)))
        else:
            raise ValueError("tapebin: unknown record")
    return bytes(out)


def encode(tape, origin_target=None, source_sha=None):
    """Encode a Tape, or a text tape, into canonical v1 bytes."""
    if isinstance(tape, str):
        tape = parse(tape)
    if not isinstance(tape, Tape):
        raise TypeError("tapebin: expected Tape or text")
    if origin_target is None:
        origin_target = getattr(tape, "origin_target", 0)
    if not 0 <= origin_target <= 6:
        raise ValueError("tapebin: invalid origin target")
    if source_sha is None:
        source_sha = getattr(tape, "source_sha", None)
    if source_sha is not None and len(source_sha) != 32:
        raise ValueError("tapebin: SOURCE_SHA must be 32 bytes")
    records = tape.records
    names, consts, name_idx, const_idx = _collect(records)
    sections = [(1, _put_pool(names), len(names)),
                (2, _put_pool(consts), len(consts)),
                (3, _encode_records(records, name_idx, const_idx), len(records))]
    if source_sha is not None:
        sections.append((4, bytes(source_sha), 1))
    directory = bytearray()
    payload = bytearray()
    pos = 64 + len(sections) * ENTRY.size
    for kind, data, count in sections:
        pad = -pos % 8
        payload.extend(b"\x00" * pad)
        pos += pad
        directory.extend(ENTRY.pack(kind, 1 if kind <= 3 else 0, pos, len(data), count))
        payload.extend(data)
        pos += len(data)
    body = bytes(directory + payload)
    if len(body) > MAX_FILE:
        raise ValueError("tapebin: file too large")
    head = HEADER.pack(MAGIC, 1, 0, 1, 0, 0, len(sections), 64,
                       hashlib.sha256(body).digest(), origin_target)
    return head + body


def _decode_records(data, count, names, consts):
    reader = _Reader(data)
    n = reader.uint()
    if n != count or n > len(data):
        raise ValueError("tapebin: record count mismatch")
    records = []
    for _ in range(n):
        kind = reader.byte()
        if kind <= 2:
            idx = reader.uint()
            if idx >= len(names):
                raise ValueError("tapebin: name index out of range")
            name = names[idx]
            if kind == 0:
                records.append(("label", name))
            elif kind == 1:
                idx = reader.uint()
                if idx >= len(consts):
                    raise ValueError("tapebin: const index out of range")
                records.append(("str", name, consts[idx]))
            else:
                records.append(("bss", name, reader.uint()))
            continue
        if kind != 3:
            raise ValueError("tapebin: unknown record kind")
        opcode = reader.byte()
        if opcode >= len(OPS):
            raise ValueError("tapebin: unknown opcode")
        op = OPS[opcode]
        shape = SHAPE[op]
        nr = shape.count("r")
        packed = reader.take((nr + 1) // 2)
        regs = []
        for i in range(nr):
            value = (packed[i // 2] >> (4 * (i % 2))) & 15
            if value >= 8:
                raise ValueError("tapebin: invalid register")
            regs.append(REGS[value])
        if nr & 1 and packed[-1] >> 4 != 15:
            raise ValueError("tapebin: invalid register padding")
        args, ri = [], 0
        for typ in shape:
            if typ == "r":
                args.append(regs[ri])
                ri += 1
            else:
                tag = reader.byte()
                if tag not in (0, 1):
                    raise ValueError("tapebin: invalid operand tag")
                if typ == "i":
                    args.append(reader.sint() if tag == 0 else reader.uint())
                elif tag == 0:
                    args.append(reader.sint())
                else:
                    idx = reader.uint()
                    if idx >= len(names):
                        raise ValueError("tapebin: name index out of range")
                    args.append(names[idx])
        records.append(("insn", op, tuple(args)))
    reader.done()
    return records


def decode(blob):
    """Validate and decode a canonical v1 package to a Tape."""
    if len(blob) < 64 or len(blob) > MAX_FILE:
        raise ValueError("tapebin: invalid file length")
    magic, major, minor, opset, flags, target, n, hlen, digest, origin = HEADER.unpack_from(blob)
    if (magic, major, minor, opset, flags, target, hlen) != (MAGIC, 1, 0, 1, 0, 0, 64) or origin > 6:
        raise ValueError("tapebin: unsupported header")
    if n < 3 or n > 5 or 64 + n * ENTRY.size > len(blob):
        raise ValueError("tapebin: invalid section count")
    if hashlib.sha256(blob[64:]).digest() != digest:
        raise ValueError("tapebin: hash mismatch")
    sections = {}
    pos = 64 + n * ENTRY.size
    for i in range(n):
        kind, sflags, off, length, count = ENTRY.unpack_from(blob, 64 + i * ENTRY.size)
        if kind in sections or kind not in (1, 2, 3, 4) or sflags != (1 if kind <= 3 else 0):
            raise ValueError("tapebin: unsupported section")
        if off != pos + (-pos % 8) or off + length > len(blob) or any(blob[pos:off]):
            raise ValueError("tapebin: noncanonical section layout")
        sections[kind] = (blob[off:off + length], count)
        pos = off + length
    if pos != len(blob) or tuple(sections) not in ((1, 2, 3), (1, 2, 3, 4)):
        raise ValueError("tapebin: noncanonical section order")
    names = _get_pool(*sections[1])
    if any(_name_bytes(x.decode("latin-1")) != x for x in names) or len(set(names)) != len(names):
        raise ValueError("tapebin: invalid names pool")
    consts = _get_pool(*sections[2])
    if len(set(consts)) != len(consts):
        raise ValueError("tapebin: duplicate constant")
    records = _decode_records(*sections[3], [x.decode("latin-1") for x in names], consts)
    tape = Tape()
    tape.records = records
    tape.origin_target = origin
    if 4 in sections:
        sha, count = sections[4]
        if len(sha) != 32 or count != 1:
            raise ValueError("tapebin: invalid SOURCE_SHA")
        tape.source_sha = sha
    # The canonical print path retains record order; parsing populates the
    # legacy code/data views that the existing Python back end consumes.
    restored = parse(tape.to_canonical_text())
    if restored.records != records:
        raise ValueError("tapebin: record/text mismatch")
    restored.origin_target = origin
    if 4 in sections:
        restored.source_sha = tape.source_sha
    if encode(restored) != blob:
        raise ValueError("tapebin: noncanonical encoding")
    return restored
