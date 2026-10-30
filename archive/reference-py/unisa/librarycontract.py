"""2a address-free declaration-stream reference (observation only).

Input USLDEC1 is a sorted canonical declaration frame. Output USLCON1 is
byte-identical after its eight-byte magic. Typed USLSIG2/3 descriptions are
opaque, previously validated API observations; this stage does not compare ABI.
No address/provider information belongs in either frame.
"""
import struct


def produce(frame: bytes) -> bytes:
    """Validate one complete bounded canonical frame and replace its magic."""
    def fail():
        raise ValueError("library contract: malformed declaration stream")

    if not isinstance(frame, bytes) or len(frame) < 16:
        fail()
    if frame[:8] != b"USLDEC1\n" or frame[8] not in (1, 2, 3, 4) or frame[9:12] != b"\x01\0\0":
        fail()
    count = int.from_bytes(frame[12:16], "little")
    if count > 4096:
        fail()
    at, previous = 16, b""
    for index in range(count):
        if len(frame) - at < 36:
            fail()
        length, ident, kind, callkind, sigv, flags, name_n, sig_n, extent, nargs, variadic, reserved = struct.unpack_from("<IIBBBBIIQIB3s", frame, at)
        at += 36
        if length != 32 + name_n + sig_n or ident != 256 + index:
            fail()
        if kind not in (0, 1) or callkind not in (0, 1, 2, 3) or sigv not in (0, 2, 3) or flags & ~3:
            fail()
        if not 1 <= name_n <= 1024 or sig_n > 1048576 or nargs > 1024 or variadic not in (0, 1) or reserved != b"\0\0\0":
            fail()
        if kind == 0 and (extent != 0 or flags & 1):
            fail()
        if kind == 1 and (callkind != 0 or nargs != 0 or variadic != 0):
            fail()
        if name_n + sig_n > len(frame) - at:
            fail()
        name = frame[at:at + name_n]
        first = lambda c: c == 95 or 65 <= c <= 90 or 97 <= c <= 122
        if not first(name[0]) or any(not (first(c) or 48 <= c <= 57) for c in name[1:]) or name <= previous:
            fail()
        previous = name
        at += name_n
        signature = frame[at:at + sig_n]
        at += sig_n
        if sigv == 0:
            if sig_n != 48 * (1 + nargs) or any(signature[i + 8:i + 24] != bytes(16) for i in range(0, sig_n, 48)):
                fail()
        elif sig_n < 16 or signature[:8] != (b"USLSIG2\n" if sigv == 2 else b"USLSIG3\n"):
            fail()
    if at != len(frame):
        fail()
    return b"USLCON1\n" + frame[8:]
