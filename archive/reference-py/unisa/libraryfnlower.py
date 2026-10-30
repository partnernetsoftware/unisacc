"""2b ordinary-function fragment observation reference (not a product adapter).
Input is tapecheck-accepted Tape and USLSEL1 model-owned partition bytes.
Only .libraryfn materialization is covered; legacy bridge addresses are not migrated.
"""
import struct
from .tape import parse

def selection(frame):
    if len(frame)<12 or frame[:8]!=b'USLSEL1\n': raise ValueError('not covered: libraryfn contract selection')
    count=int.from_bytes(frame[8:12],'little')
    if count>4096 or len(frame)!=12+5*count: raise ValueError('not covered: libraryfn contract selection')
    result={}
    for i in range(count):
        ident,owner=struct.unpack_from('<IB',frame,12+5*i)
        if ident!=256+i or owner not in (1,2): raise ValueError('not covered: libraryfn contract selection')
        result[ident]=owner
    return result

def frame(text,spans):
    return b'UNILOW1\n'+struct.pack('<QI',len(text),len(spans))+text+b''.join(struct.pack('<QIIB3x',offset,1,ident,0) for offset,ident in spans)

def fragment(tape,selected,registers,library_mode=True,contract=None,target=1):
    """Return only new materialization instructions, with exact placeholder spans.
    registers is an explicit target rule mapping (no host/provider inputs).
    Full lower output, including legacy bridges, is deliberately outside this witness.
    """
    check_provenance(tape)
    owners=selection(selected); out=bytearray(); spans=[]
    for insn in parse(tape).code:
        if insn.op!='.libraryfn': continue
        if not library_mode: raise ValueError('not covered: libraryfn requires library mode')
        reg,ident=insn.args
        if owners.get(ident)!=1: raise ValueError('not covered: libraryfn contract selection')
        supported(contract,ident,target)
        prefix=('setreg '+registers[reg]+', imm ').encode('ascii')
        out.extend(prefix);spans.append((len(out),ident));out.extend(b'0\n')
    return frame(bytes(out),spans)

def supported(frame_bytes, ident, target=1):
    """Check selected ordinary flat ABI after the upstream contract validator.
    This is coverage, never provider/availability or signature-mismatch evidence.
    """
    from .librarycontract import produce
    if not isinstance(frame_bytes, bytes) or frame_bytes[:8] != b'USLCON1\n':
        raise ValueError('not covered: libraryfn ordinary ABI')
    produce(b'USLDEC1\n'+frame_bytes[8:])
    if target not in (1,2,3,4) or frame_bytes[8]!=target:
        raise ValueError('not covered: libraryfn ordinary ABI')
    at=16
    while at<len(frame_bytes):
        size, found, kind, callkind, sigv, flags, name_n, sig_n, extent, nargs, variadic, reserved=struct.unpack_from('<IIBBBBIIQIB3s',frame_bytes,at)
        if found==ident:
            if (kind,callkind,sigv,variadic)!=(0,0,0,0) or nargs>6 or sig_n!=48*(nargs+1): break
            sig=at+36+name_n
            for i in range(nargs+1):
                depth,base,shape,cls,width,uns=struct.unpack_from('<6Q',frame_bytes,sig+48*i)
                ok=(i==0 and cls==0 and depth==width==uns==0) or (cls==1 and depth==0 and width in (1,2,4,8) and uns in (0,1)) or (cls==2 and depth>0 and width==8 and uns==0)
                if not ok: break
            else: return True
            break
        at+=4+size
    raise ValueError('not covered: libraryfn ordinary ABI')

def check_provenance(tape):
    """Narrow template: poison survives labels; consumed clears only at labels.
    Does not authorize legacy calls or make any full-program dataflow claim.
    """
    phase=dirty=0;callee=None
    for record in parse(tape).records:
        if record[0]=='label':
            if phase: phase,dirty=0,1
            elif dirty==2: dirty=0
            continue
        if record[0]!='insn': continue
        _,op,args=record
        if phase:
            if op=='mov' and phase==1 and args==('r0','r7') and callee!='r0': phase=2
            elif op=='.librarycall' and phase==2: pass
            else: phase,dirty=0,1
        if op=='.libraryfn': phase,dirty,callee=1,0,args[0]
        elif op=='.librarycall':
            if phase==2:
                if args!=(callee,'r0'): raise ValueError('not covered: libraryfn call provenance')
                phase,dirty=0,2
            elif dirty and args[0]==callee: raise ValueError('not covered: libraryfn call provenance')
    return True
