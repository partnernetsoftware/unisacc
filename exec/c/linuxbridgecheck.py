#!/usr/bin/env python3
"""Cross-assemble both ELF bridges and compare instruction bytes with Mach-O.
This is format/symbol evidence, not native Linux execution evidence.
"""
import argparse, json, os, pathlib, shutil, struct, subprocess, tempfile
ROOT=pathlib.Path(__file__).resolve().parents[2]
def call(args,ok=True):
    p=subprocess.run(list(map(str,args)),capture_output=True,timeout=15)
    if ok:assert p.returncode==0,(args,p.returncode,p.stderr)
    return p

def text(blob):
    if blob[:4]==b'\x7fELF':
        shoff=struct.unpack_from('<Q',blob,40)[0];ents,n,si=struct.unpack_from('<HHH',blob,58)
        headers=[struct.unpack_from('<IIQQQQIIQQ',blob,shoff+i*ents) for i in range(n)]
        strings=headers[si];names=blob[strings[4]:strings[4]+strings[5]]
        for h in headers:
            name=names[h[0]:].split(b'\0')[0]
            if name==b'.text':return blob[h[4]:h[4]+h[5]]
    else:
        assert blob[:4]==bytes.fromhex('cffaedfe')
        pos=32
        for _ in range(struct.unpack_from('<I',blob,16)[0]):
            cmd,size=struct.unpack_from('<II',blob,pos)
            if cmd==25:
                for j in range(struct.unpack_from('<I',blob,pos+64)[0]):
                    at=pos+72+j*80
                    if blob[at:at+16].split(b'\0')[0]==b'__text':
                        n=struct.unpack_from('<Q',blob,at+40)[0];off=struct.unpack_from('<I',blob,at+48)[0]
                        return blob[off:off+n]
            pos+=size
    raise AssertionError('missing text')

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--evidence',type=pathlib.Path);a=ap.parse_args()
    llvm=pathlib.Path(os.environ.get('LLVM_BIN','/opt/homebrew/opt/llvm/bin'))
    readelf=shutil.which('llvm-readelf') or str(llvm/'llvm-readelf')
    objdump=shutil.which('llvm-objdump') or str(llvm/'llvm-objdump')
    rows=[]
    with tempfile.TemporaryDirectory(prefix='unisacc-linuxbridge-check-') as name:
        d=pathlib.Path(name)
        for arch,target in [('arm64','aarch64-linux-gnu'),('x86_64','x86_64-linux-gnu')]:
            src=ROOT/'exec/c'/('librarycall_'+arch+'.S');elf=d/(arch+'.elf.o');mac=d/(arch+'.mac.o')
            call(['clang','-target',target,'-c',src,'-o',elf]);call(['clang','-arch',arch,'-c',src,'-o',mac])
            old=d/(arch+'-old.S');old.write_bytes(call(['git','-C',ROOT,'show','HEAD:exec/c/'+src.name]).stdout)
            baseline=d/(arch+'-old.o');call(['clang','-arch',arch,'-c',old,'-o',baseline])
            assert text(elf.read_bytes())==text(mac.read_bytes())==text(baseline.read_bytes())
            symbols=call([readelf,'-sSW',elf]).stdout.decode()
            assert 'FUNC    GLOBAL DEFAULT' in symbols and ' us_library_bridge_raw' in symbols
            assert ' _us_library_bridge_raw' not in symbols
            stack=next(s for s in symbols.splitlines() if '.note.GNU-stack' in s)
            assert ' X ' not in stack and ' AX ' not in stack
            asm=call([objdump,'-d',elf]).stdout.decode();assert '<us_library_bridge_raw>:' in asm
            for windows in ('aarch64-windows-msvc','x86_64-windows-msvc'):
                bad=call(['clang','-target',windows,'-c',src,'-o',d/'bad.o'],ok=False)
                assert bad.returncode!=0 and b'does not implement the Windows host ABI' in bad.stderr
            rows.append(dict(arch=arch,text_bytes=len(text(elf.read_bytes())),symbols='ELF GLOBAL FUNC',instruction_bytes='unchanged Mach-O baseline',gnu_stack='non-executable',windows='rejected'))
    if a.evidence:a.evidence.write_text(json.dumps(rows,indent=2)+'\n')
    print(json.dumps(rows))
if __name__=='__main__':main()
