"""Reader-only fixed-two-int witness oracle; observation, not native ABI proof.

Explicit source facts (used/defined/prototype-compatible) come from frozen E1
fixtures. This does not parse C or certify the entire typed signature domain.
The reader commit leaves the old wrapper tail unconnected (dispatch literal 0).
"""
import struct
from .librarycontract import produce as validate

def produce(frame, selection, mode, source_state):
    def fail(): raise ValueError("not covered: library typed contract import")
    validate(b"USLDEC1\n" + frame[8:])
    if frame[:8] != b"USLCON1\n" or frame[12:16] != b"\1\0\0\0" or mode != b"\1"+bytes(7): fail()
    length, ident, kind, callkind, version, flags, nn, sn, extent, count, var, reserved = struct.unpack_from("<IIBBBBIIQIB3s", frame, 16)
    if selection[:12] != b"USLSEL1\n"+struct.pack("<I",1) or len(selection)!=17 or int.from_bytes(selection[12:16],"little")!=ident: fail()
    owner=selection[16]
    if owner==2 and kind==0 and (callkind in (2,3) or callkind==1 and version!=3): return b""
    if owner!=1 or (ident,kind,callkind,version,flags,extent,count,var) != (256,0,1,3,0,0,2,0): fail()
    name=frame[52:52+nn].decode("ascii"); sig=frame[52+nn:]
    if sig[:8]!=b"USLSIG3\n" or struct.unpack_from("<Q",sig,8)[0]!=1: fail()
    z=struct.unpack_from("<Q",sig,16)[0]
    if sig[24:24+z]!=name.encode() or sig[24+z:28+z]!=b"\0\1\0\0" or struct.unpack_from("<Q",sig,28+z)[0]!=2: fail()
    # Three scalar descriptors: result followed by two fixed parameters.
    at=36+z
    for j in range(3):
        if j:
            if j==1:
                if struct.unpack_from("<Q",sig,at)[0]!=2: fail()
                at+=8
        fields=struct.unpack_from("<7Q",sig,at); facts=struct.unpack_from("<BBQBBBBQ",sig,at+56)
        if fields!=(0,0,0,1,4,0,4) or facts!=(0,0,4,0,3,2,0,0): fail()
        at+=78
    if sig[at:]!=b"\0": fail()
    if source_state=="source-mismatch": raise ValueError("not covered: library import binding or signature")
    if source_state in ("unused","defined"):return b""
    if source_state!="used":fail()
    text=name+":\n  .frame 8\n  store64 [r7+0], r6\n  mov r6, r7\n  .frame 72\n"
    text+="  store64 [r7+48], r0\n  store64 [r7+56], r1\n  imm r1, 0\n  store64 [r7+0], r1\n"
    text+="  imm r1, 48\n  add64 r1, r7, r1\n  store64 [r7+8], r1\n  imm r1, 64\n  add64 r1, r7, r1\n  store64 [r7+16], r1\n"
    text+="  imm r1, 2\n  store64 [r7+24], r1\n  imm r1, 0\n  store64 [r7+32], r1\n  imm r1, 0\n  store64 [r7+40], r1\n"
    text+="  imm r1, 0\n  mov r0, r7\n  .librarycall r1, r0\n  load64 r1, [r7+16]\n  load64 r0, [r1+0]\n"
    text+="  .frame -72\n  load64 r6, [r7+0]\n  .frame -8\n  ret\n"
    return text.encode()

if __name__=="__main__":
    import sys
    from pathlib import Path
    try: out=produce(*(Path(x).read_bytes() for x in sys.argv[1:4]),sys.argv[4])
    except (ValueError,struct.error,UnicodeError) as e:print(e,file=sys.stderr);sys.exit(1)
    sys.stdout.buffer.write(out)
