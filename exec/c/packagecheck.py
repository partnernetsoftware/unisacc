#!/usr/bin/env python3
"""Production P1/P2/P3 execution, binary loading and corrupt package controls."""
import argparse, os, pathlib, subprocess, tempfile, zlib
from pack import build, compact_q, compressed_model
from networkformat import encode, decode, varint
from tbl import CODE
R = pathlib.Path(__file__).resolve().parents[2]

def call(argv):
    return subprocess.run(list(map(str,argv)),capture_output=True,timeout=15)

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--ua',required=True,type=pathlib.Path)
    ap.add_argument('--sanitize',action='store_true')
    a=ap.parse_args()
    with tempfile.TemporaryDirectory(prefix='unisacc-package-') as td:
        d=pathlib.Path(td); exes=[]
        for name,cmd in [('cc',['cc','-O2']),('ua',[a.ua,'-O2'])]:
            if name=='cc' and a.sanitize:cmd=['cc','-O1','-g','-fsanitize=address,undefined']
            exe=d/name;p=call([*cmd,R/'exec/c/run.c','-o',exe]);assert p.returncode==0,(p.returncode,p.stderr)
            exes.append(exe)
        common=' '.join(f'{CODE["LDI"]} 0 {i}' for i in range(20))
        tiny=(f'N 2 2 1 1 0 1\nS 61200aff\nQ 20 {common}\nQ 22 {common} {CODE["OUT"]} 89 {CODE["ACCEPT"]}\n'
              'H 0 0 256 0 1 0 \nH 0 0 256 0 1 1 \n').encode()
        n=d/'tiny.net';n.write_bytes(tiny)
        manifest=d/'routes.tsv';manifest.write_text('r\te3\tbytes\tbytes\ttiny.net\n')
        hdr=d/'hdr';hdr.mkdir();(hdr/'x.h').write_bytes(b'test resource')
        packages=[build([manifest]),build([manifest],[('006864722f',hdr)]),build([manifest],[('006864722f',hdr)],compressed=True)]
        inp=d/'input';inp.write_bytes(b''); pkg=d/'test.pkg'
        for body in packages:
            pkg.write_bytes(body)
            for exe in exes:
                p=call([exe,'--bundle',pkg,'r',inp]);assert (p.returncode,p.stdout,p.stderr)==(0,b'Y',b''),(exe,p.returncode,p.stderr)
        raw=compact_q(tiny);binary=encode(raw);assert decode(binary)==raw
        n.write_bytes(binary)
        for exe in exes:
            p=call([exe,n,inp]);assert (p.returncode,p.stdout,p.stderr)==(0,b'Y',b'')
        # Reframe corrupted compressed bodies so extent parsing cannot mask the codec checks.
        body=packages[-1];pos=body.index(b'\n',body.index(b'\n')+1)+1
        end=body.index(b'\n',pos);mh=body[pos:end].split();blob=body[end+1:end+1+int(mh[1])];tail=body[end+1+len(blob):]
        def reframe(b, size=int(mh[2]), crc=int(mh[4]), codec=1):
            return body[:pos]+f'M {len(b)} {size} {codec} {crc}\n'.encode()+b+tail
        c=zlib.compressobj(9,zlib.DEFLATED,-15);wrong=c.compress(b'wrong model')+c.flush()
        changed=bytearray(blob);changed[len(changed)//2]^=128
        bad=[body.replace(b'P 3 ',b'P 99 ',1),body[:-1],reframe(blob[:-1]),reframe(blob+b'X'),
             reframe(blob,size=int(mh[2])+1),reframe(blob,size=int(mh[2])-1),
             reframe(blob,crc=int(mh[4])^1),reframe(blob,codec=99),reframe(bytes(changed)),
             reframe(wrong,size=11,crc=zlib.crc32(b'wrong model'))]
        for i,b in enumerate(bad):
            pkg.write_bytes(b)
            for exe in exes:
                p=call([exe,'--bundle',pkg,'r',inp]);assert p.returncode==2 and not p.stdout and p.stderr.startswith(b'run:'),(i,exe,p.returncode,p.stderr)
        # Binary varint/count/tag controls are parsed by the production reader.
        binaries=[binary[:10], b'UNINETB1N'+b'\x80'*10, b'UNINETB1N'+b'\x82\x00'+binary[10:],
                  b'UNINETB1N'+varint(2147483647)+binary[10:], binary+b'X']
        for b in binaries:
            n.write_bytes(b)
            for exe in exes:
                p=call([exe,n,inp]);assert p.returncode==2 and not p.stdout,(exe,p.returncode,p.stderr)
        previous=os.environ.get('UNISACC_MODEL_CACHE');os.environ['UNISACC_MODEL_CACHE']=str(d/'cache')
        try:
            want=compressed_model(raw,cache=False);assert compressed_model(raw)==want
            cached=[p for p in (d/'cache').iterdir() if not p.name.endswith('.lock')];assert len(cached)==1
            cached[0].write_bytes(b'corrupt');assert compressed_model(raw)==want
            assert compressed_model(raw)==want
        finally:
            if previous is None:os.environ.pop('UNISACC_MODEL_CACHE')
            else:os.environ['UNISACC_MODEL_CACHE']=previous
        print('package: P1/P2/P3 equal; 10 corrupt packages and 5 bad binaries rejected; cache poison rebuilt')
if __name__=='__main__':main()
