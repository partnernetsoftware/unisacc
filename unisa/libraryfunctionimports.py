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


def data_fragment(frame: bytes, references=(), *, selection: bytes, target: int, occupied_names=()) -> bytes:
    """Observation of model-owned six-operand data references, no provider bytes.

    references is an explicit sequence of (register, name, depth, class, width,
    unsigned) captured from the stage input; this helper is not a source parser.
    """
    validate(b'USLDEC1\n' + frame[8:])
    if not frame.startswith(b'USLCON1\n') or frame[8] != target:
        raise ValueError('not covered: library data target conflict')
    count = int.from_bytes(frame[12:16], 'little')
    if selection[:8] != b'USLSEL1\n' or len(selection) != 12 + 5*count or int.from_bytes(selection[8:12], 'little') != count:
        raise ValueError('not covered: library contract import')
    records = {}
    at = 16
    for index in range(count):
        fields = struct.unpack_from('<IIBBBBIIQIB3s',frame,at)
        length,ident,kind,callkind,sigv,flags,nn,sn,extent,argc,var,_ = fields
        sid = int.from_bytes(selection[12+5*index:16+5*index], 'little')
        owner = selection[16+5*index]
        if owner == 0:
            raise ValueError('not covered: missing-owner library contract')
        if owner in (3, 5, 6, 7):
            raise ValueError('not covered: duplicate-owner library contract')
        if sid != ident or owner not in (1, 2, 4):
            raise ValueError('not covered: library contract import')
        if owner != (4 if kind == 1 else (1 if callkind == 0 else 2)):
            raise ValueError('not covered: wrong-owner library contract')
        name = frame[at+36:at+36+nn].decode('ascii')
        if name in occupied_names:
            raise ValueError('not covered: duplicate-owner library contract')
        signature = frame[at+36+nn:at+4+length]
        if kind == 1:
            if callkind or sigv or argc or var or sn != 48:
                raise ValueError('not covered: library contract import')
            descriptor=struct.unpack('<6Q',signature)
            records[name]=(descriptor,extent)
        at+=4+length
    out=bytearray()
    for reg,name,depth,cls,width,unsigned in references:
        if name not in records: raise ValueError('not covered: library contract import')
        descriptor,extent=records[name]
        if (descriptor[0],descriptor[3],descriptor[4],descriptor[5]) != (depth,cls,width,unsigned) or extent < width:
            raise ValueError('not covered: library contract import')
        out.extend(f'  .libraryaddr {reg}, g_{name}, {depth}, {cls}, {width}, {unsigned}\n'.encode('ascii'))
    return bytes(out)
