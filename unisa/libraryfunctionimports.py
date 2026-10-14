"""2b ordinary wrapper emission reference; observation only.

This narrow reference consumes one complete USLCON1 frame already validated by
librarycontract and explicit USED/DEFINED name sets from the E3 input fixture.
It covers the six-register, flat 64-bit integer/pointer wrapper witness, not a
replacement C parser or proof of all ABI shapes. Addresses/providers are absent.
The E3 model additionally owns source-prototype descriptor agreement.
"""
import struct
from .librarycontract import produce as validate

def produce(frame: bytes, used=(), defined=()) -> bytes:
    if not frame.startswith(b'USLCON1\n'):
        raise ValueError('library contract import: invalid magic')
    validate(b'USLDEC1\n' + frame[8:])
    used, defined = set(used), set(defined)
    result = bytearray()
    at = 16
    for _ in range(int.from_bytes(frame[12:16], 'little')):
        length, ident, kind, callkind, sigv, flags, nn, sn, extent, argc, variadic, _reserved = struct.unpack_from('<IIBBBBIIQIB3s', frame, at)
        name = frame[at + 36:at + 36 + nn].decode('ascii')
        signature = frame[at + 36 + nn:at + 4 + length]
        at += 4 + length
        if kind != 0 or callkind != 0 or name not in used or name in defined:
            continue
        if sigv or variadic or argc > 6:
            raise ValueError('library contract import: ordinary wrapper not covered')
        for j in range(argc + 1):
            depth, base, shape, cls, width, unsigned = struct.unpack_from('<6Q', signature, 48*j)
            if base or shape or not ((cls == 1 and width == 8 and depth == 0) or (cls == 2 and width == 8 and depth > 0)):
                raise ValueError('library contract import: reference witness ABI not covered')
        text = name + ':\n  .frame 48\n'
        text += ''.join(f'  store64 [r7+{8*j}], r{j}\n' for j in range(6))
        text += f'  .libraryfn r1, {ident}\n  mov r0, r7\n  .librarycall r1, r0\n  .frame -48\n  ret\n'
        result.extend(text.encode('ascii'))
    return bytes(result)
