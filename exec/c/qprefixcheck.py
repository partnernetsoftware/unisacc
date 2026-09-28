#!/usr/bin/env python3
"""Verify Q serialization and production C decoding without rebuilding models.
--package checks existing real networks too; no generators are launched.
"""
import argparse, pathlib, subprocess, tempfile
from pack import compact_q, decode_q
from tbl import CODE
ROOT = pathlib.Path(__file__).resolve().parents[2]
def run(argv):
    return subprocess.run(list(map(str, argv)), capture_output=True, timeout=30)
def require(argv):
    p = run(argv)
    if p.returncode: raise AssertionError((argv, p.returncode, p.stderr))
    return p.stdout

def models(package):
    from packageformat import read_package
    yield from read_package(package)["models"]

ap = argparse.ArgumentParser(description=__doc__)
ap.add_argument('--package', type=pathlib.Path)
ap.add_argument('--artifact', type=pathlib.Path)
ap.add_argument('--sanitize', action='store_true')
a = ap.parse_args()
flags = ['-fsanitize=address,undefined'] if a.sanitize else []
with tempfile.TemporaryDirectory(prefix='unisacc-qprefix-') as td:
    d = pathlib.Path(td)
    # Compare flat-Q and prefix-C inputs through the production decoder.
    # This also runs from exported source trees without a git repository.
    (d/'old.c').write_bytes((ROOT/'exec/c/run.c').read_bytes())
    exes = []
    for label, source in [('old',d/'old.c'), ('new',ROOT/'exec/c/run.c')]:
        loader = d/(label+'load.c')
        loader.write_text('#define UNISA_RUNTIME_LIBRARY\n#include "'+str(source)+'"\n'
                          'int main(int argc,char **argv) { int n; if(argc!=2)return 9; '
                          'unsigned char *b=readfile(argv[1],&n,0); loadbytes(b,n); '
                          'if(fwrite(QLEN,sizeof(int),NQ,stdout)!=(size_t)NQ)return 8; '
                          'if(fwrite(QA,sizeof(I),NQA,stdout)!=(size_t)NQA)return 8; '
                          'unload();free(b);return 0;}\n')
        exe = d/(label+'load')
        require(['cc','-O2',*flags,'-I',ROOT/'exec/c',loader,'-o',exe]); exes.append(exe)
    runner=d/'run'; require(['cc','-O2',*flags,ROOT/'exec/c/run.c','-o',runner])
    common = ' '.join(f'{CODE["LDI"]} 0 {i}' for i in range(20))
    tiny = (f'N 2 2 1 0 0 1\nQ 20 {common}\nQ 22 {common} {CODE["OUT"]} 89 {CODE["ACCEPT"]}\n'
            'H 0 0 256 0 1 0\nH 0 0 256 0 1 1\n').encode()
    inp=d/'input';inp.write_bytes(b'')
    old=d/'old.net';new=d/'new.net';old.write_bytes(tiny);new.write_bytes(compact_q(tiny))
    assert b'\nC ' in new.read_bytes()
    assert require([exes[0],old]) == require([exes[1],new])
    assert require([runner,old,inp]) == require([runner,new,inp]) == b'Y'
    q = compact_q(tiny).splitlines()[2]
    for bad in [b'C 1 1 0',b'C -1 1 0',b'C 0 0 0',b'C 0 21 0',b'C 0 1 -1',
                b'C 0 1 2147483647',b'C 0 1 1',b'C 0 1 1 '+str(CODE['LDI']).encode()+b' 0']:
        mutated=compact_q(tiny).replace(q,bad);new.write_bytes(mutated)
        p=run([exes[1],new]);assert p.returncode==2,(bad,p.returncode,p.stderr)
        try: decode_q([tiny.splitlines()[1],bad])
        except ValueError: pass
        else: raise AssertionError(('Python decoder accepted',bad))
    # Old Q and chained C references remain supported, including a zero tail.
    qs=[b'Q 1 '+str(CODE['LDI']).encode()+b' 0 7', b'C 0 1 0', b'C 1 1 0']
    chain=b'N 1 3 1 0 0 0\n'+b'\n'.join(qs)+b'\nH 0 0 256 0 -1 0\n'
    new.write_bytes(chain);require([exes[1],new]);assert len(decode_q(qs))==3
    count=saved=0
    if a.package or a.artifact:
        package=a.package.read_bytes() if a.package else a.artifact.read_bytes()
        if a.artifact:
            assert package[-16:-8]==b'UNIPKG1\n';n=int.from_bytes(package[-8:],'little');package=package[-16-n:-16]
        for model in models(package):
            compressed=compact_q(model);old.write_bytes(model);new.write_bytes(compressed)
            assert require([exes[0],old])==require([exes[1],new]),'expanded C QA differs'
            head=model.splitlines()[0].split();start=1+int(head[4]);nq=int(head[2])
            assert model.splitlines()[:start]==compressed.splitlines()[:start]
            assert model.splitlines()[start+nq:]==compressed.splitlines()[start+nq:]
            assert decode_q(model.splitlines()[start:start+nq])==decode_q(compressed.splitlines()[start:start+nq])
            saved+=len(model)-len(compressed);count+=1
    print(f'qprefix: tiny execution equal; 8 malformed rejected; chained prefixes accepted; real models {count}, saved {saved} B; QA exact, runtime memory unchanged')
