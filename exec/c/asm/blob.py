#!/usr/bin/env python3
"""Offline assembly/linking only. Extract a position-independent kernel blob.
No compiler model, source program or expected answer is read here. The linked
Mach-O is a seed-tool container, not the OS format of the returned machine code.
"""
import argparse,pathlib,struct,subprocess,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[3]
UNITS=('transition','arith','buffer','memory','intern','bytes','format','stack','action','run')
IMPORTS=('calloc','realloc','free','memcpy','memcmp','memset','strlen','core_host_fetch','core_host_panic')
def run(cmd):
    r=subprocess.run(list(map(str,cmd)),capture_output=True,timeout=60)
    if r.returncode: raise RuntimeError(f'{cmd}: {r.returncode}: {r.stderr.decode(errors="replace")}')
    return r.stdout

def build(arch):
    with tempfile.TemporaryDirectory(prefix='unisa-kernel-') as td:
        d=pathlib.Path(td);objects=[]
        for unit in UNITS+('bridge',):
            source=ROOT/'exec/c/asm'/f'{unit}_{arch}.S';obj=d/(unit+'.o')
            defs=[] if unit=='bridge' else ['-D'+s+'=bridge_'+s for s in IMPORTS]
            run(['cc','-arch',arch,*defs,'-c',source,'-o',obj]);objects.append(obj)
        image=d/'kernel'
        run(['cc','-arch',arch,'-nostdlib','-Wl,-static','-Wl,-e,_kernel_entry','-Wl,-no_uuid',
             '-Wl,-no_fixup_chains',*objects,'-o',image])
        if run(['nm','-u',image]).strip(): raise ValueError('unresolved kernel imports')
        raw=image.read_bytes()
        if struct.unpack_from('<I',raw)[0]!=0xfeedfacf:raise ValueError('not Mach-O 64')
        ncmd=struct.unpack_from('<I',raw,16)[0];at=32;text=None;end=0;symbols=None
        for _ in range(ncmd):
            cmd,size=struct.unpack_from('<II',raw,at)
            if size<8 or at+size>len(raw):raise ValueError('bad load command')
            if cmd==0x19:
                name=raw[at+8:at+24].rstrip(b'\0')
                va,vs,off,fs=struct.unpack_from('<4Q',raw,at+24)
                if name==b'__TEXT':
                    if text is not None or off!=0 or va%16384:raise ValueError('bad text base')
                    text=(va,fs)
                    ns=struct.unpack_from('<I',raw,at+64)[0]
                    for i in range(ns):
                        pos=at+72+80*i
                        addr,length=struct.unpack_from('<QQ',raw,pos+32)
                        sec_off=struct.unpack_from('<I',raw,pos+48)[0]
                        if addr-va!=sec_off or sec_off+length>fs:raise ValueError('nonlinear text')
                        end=max(end,sec_off+length)
                elif name not in (b'__PAGEZERO',b'__LINKEDIT'):raise ValueError('unexpected mutable segment')
            elif cmd==2: symbols=struct.unpack_from('<4I',raw,at+8)
            elif cmd in (0x22,0x80000022):
                # No rebasing, binding or lazy imports may be needed at runtime.
                fields=struct.unpack_from('<10I',raw,at+8)
                if any(fields[i] for i in (1,3,5,7)):raise ValueError('runtime relocation required')
            elif cmd==0x80000034:raise ValueError('chained fixups unsupported')
            at+=size
        if text is None or symbols is None or end<=0:raise ValueError('missing kernel text/symbols')
        symoff,count,stroff,strsize=symbols;found={}
        for i in range(count):
            ix,typ,sect,desc,value=struct.unpack_from('<IBBHQ',raw,symoff+i*16)
            if ix>=strsize:raise ValueError('bad string index')
            name=raw[stroff+ix:stroff+strsize].split(b'\0',1)[0]
            if name in (b'_kernel_entry',b'_kernel_service'):
                if name in found or (typ&0x0e)!=0x0e:raise ValueError('bad exported binding')
                found[name]=value-text[0]
        entry=found[b'_kernel_entry'];slot=found[b'_kernel_service']
        if not 0<=entry<end or not 0<=slot<=end-8 or slot%8 or any(raw[slot:slot+8]):
            raise ValueError('bad entry or import slot')
        return b'UNIKERN1'+struct.pack('<4Q',1 if arch=='arm64' else 2,entry,slot,end)+raw[:end]

if __name__=='__main__':
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('arch',choices=('arm64','x86_64'));ap.add_argument('output',type=pathlib.Path)
    a=ap.parse_args();data=build(a.arch);a.output.write_bytes(data)
    print(f'kernel blob {a.arch}: {len(data)} B, entry/slot/text {struct.unpack_from("<3Q",data,16)}')
