"""2c1 data-path observation reference; finite lower model checked, API wiring pending.
No provider inputs. USLCON1 is canonical, reserved header byte 11 is zero.
The provenance witness is straight-line register/explicit-stack flow only.
"""
import struct
from .tape import parse
from .librarycontract import produce
from .libraryfnlower import frame

def contracts(blob, selected, target=1):
    if not blob or blob[:8]!=b'USLCON1\n': raise ValueError('library data: contract missing')
    produce(b'USLDEC1\n'+blob[8:])
    if blob[8]!=target or target not in (1,2,3,4): raise ValueError('library data: target mismatch')
    if selected[:8]!=b'USLSEL1\n' or len(selected)<12: raise ValueError('library data: selection malformed')
    n=int.from_bytes(selected[8:12],'little')
    if n>4096 or len(selected)!=12+5*n or n!=int.from_bytes(blob[12:16],'little'): raise ValueError('library data: selection malformed')
    owners={}
    for i in range(n):
        ident,own=struct.unpack_from('<IB',selected,12+5*i)
        if ident!=256+i or own not in (1,2,4): raise ValueError('library data: owner invalid')
        owners[ident]=own
    out={};at=16
    for i in range(n):
        size,ident,kind,callkind,sigv,flags,name_n,sig_n,extent,nargs,variadic,reserved=struct.unpack_from('<IIBBBBIIQIB3s',blob,at)
        name=blob[at+36:at+36+name_n].decode('ascii')
        typ=struct.unpack_from('<6Q',blob,at+36+name_n) if sigv==0 else None
        out[name]=(ident,kind,owners[ident],flags,extent,typ,sigv,callkind,nargs,variadic)
        at+=4+size
    return out

def fragment(tape,blob,selected,registers,target=1):
    declared=contracts(blob,selected,target); text=bytearray();spans=[];regs={};stack={};sp=0;aux={};history=set()
    def unsupported(meta=None):
        meta=meta or next(iter(regs.values()),None) or next(iter(stack.values()),None)
        suffix=(' symbol %d name %s'%(meta[0],meta[1])) if meta else ''
        raise ValueError('not covered: library data provenance'+suffix)
    witness=any(v[1]==1 and v[2]==4 and not(v[3]&1) for v in declared.values())
    parsed=parse(tape)
    for rec in parsed.records:
        if rec[0]=='label':
            if regs or stack: unsupported()
            aux.clear();sp=0
            continue
        if rec[0]!='insn':continue
        _,op,a=rec
        if op=='.libraryaddr':
            reg,symbol,depth,cls,width,uns=a
            if symbol in parsed.syms:
                regs.pop(reg,None);aux.pop(reg,None)
                continue # source data definition owns this reference, no external span
            name=symbol[2:] if symbol.startswith('g_') else symbol
            if name not in declared:raise ValueError('library data: missing name '+name)
            ident,kind,owner,flags,extent,typ,sigv,callkind,nargs,variadic=declared[name]
            if kind!=1:raise ValueError('library data: kind mismatch')
            if owner!=4:raise ValueError('library data: owner mismatch')
            if sigv!=0 or callkind or nargs or variadic or typ is None or typ[1:3]!=(0,0):raise ValueError('library data: type unsupported')
            if (typ[0],typ[3],typ[4],typ[5])!=(depth,cls,width,uns):raise ValueError('library data: type mismatch')
            if not ((cls==1 and depth==0 and width in (1,2,4,8) and uns in (0,1)) or (cls==2 and depth>0 and width==8 and uns==0)) or extent<width:raise ValueError('library data: type unsupported')
            meta=(ident,name,bool(flags&1),extent,cls)
            # Only readonly origins require a write-prohibition witness. Tracking
            # writable pointers here would turn ordinary valid control flow red.
            aux.pop(reg,None)
            if flags&1:regs.pop(reg,None)
            else:regs[reg]=meta
            prefix=('setreg '+registers[reg]+', imm ').encode();text.extend(prefix);spans.append((len(text),ident));text.extend(b'0\n')
        elif not witness:continue
        elif op=='.frame':
            old=sp;sp+=a[0]
            if sp<old:stack={k:v for k,v in stack.items() if k>=-sp}
        elif op=='mov':
            src=('S',-sp) if a[1]=='r7' else aux.get(a[1])
            if src:aux[a[0]]=src
            else:aux.pop(a[0],None)
            if a[1] in regs:regs[a[0]]=regs[a[1]]
            elif src and src[0]=='S':regs.pop(a[0],None)
            elif a[0] in regs:
                old=regs[a[0]];regs[a[0]]=(old[0],old[1],None,old[3],old[4])
            else:regs.pop(a[0],None)
        elif op in ('add64','sub64'):
            if a[1] in regs:unsupported(regs[a[1]])
            if a[2] in regs:unsupported(regs[a[2]])
            x=('S',-sp) if a[1]=='r7' else aux.get(a[1]); y=('S',-sp) if a[2]=='r7' else aux.get(a[2])
            if x and x[0]=='S':
                if not y or y[0]!='C':unsupported()
                v=x[1]+(y[1] if op=='add64' else -y[1])
                if not -sp<=v<0:unsupported()
                aux[a[0]]=('S',v)
            elif y and y[0]=='S':unsupported()
            else:aux.pop(a[0],None)
            regs.pop(a[0],None)
        elif op in ('imm','.lea'):
            regs.pop(a[0],None)
            if op=='imm' and -1048576<=a[1]<=1048576:aux[a[0]]=('C',a[1])
            else:aux.pop(a[0],None)
        elif op in ('.ld','load64','.st','store64'):
            load=op in ('.ld','load64');width=a[3] if op in ('.ld','.st') else 8
            dest,base,off=(a[0],a[1],a[2]) if load else (None,a[0],a[1]);value=None if load else a[2]
            if not load and value in aux and aux[value][0]=='S':unsupported()
            baseaux=aux.get(base)
            if load:aux.pop(dest,None)
            if base in regs:
                if regs[base][2] is None:unsupported(regs[base])
                ident,name,writable,extent,cls=regs[base]
                if not load and not writable:raise ValueError(f'library data: readonly-write symbol {ident} name {name}')
                if off<0 or width not in (1,2,4,8) or off+width>extent:unsupported()
                if not load and value in regs:unsupported()
                if load:
                    if cls==2:regs[dest]=(ident,name,None,extent,cls)
                    else:regs.pop(dest,None)
            elif base=='r7' or (baseaux and baseaux[0]=='S'):
                slot=off-sp if base=='r7' else baseaux[1]+off
                if base!='r7' and not(-sp<=slot and slot+width<=0):unsupported()
                if load:
                    if slot in stack:
                        if width!=8:unsupported()
                        regs[dest]=stack[slot]
                    else:
                        if slot in history or any(k<slot+width and slot<k+8 for k in stack):unsupported()
                        regs.pop(dest,None)
                else:
                    if value in regs:
                        if width!=8:unsupported()
                        stack[slot]=regs[value];history.add(slot)
                    else:
                        for k in list(stack):
                            if k<slot+width and slot<k+8:
                                if width!=8 or k!=slot:unsupported()
                                old=stack[k];stack[k]=(old[0],old[1],None,old[3],old[4])
            elif stack or value in regs or (value in aux and aux[value][0]=='S'):unsupported()
            elif load:regs.pop(dest,None)
        elif op in ('nop','ret'):pass
        elif regs or stack:unsupported()
    return frame(bytes(text),spans)
