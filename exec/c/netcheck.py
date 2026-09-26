#!/usr/bin/env python3
"""Exercise production inference, including holes and negative output weights.
Optional .tbl arguments are exhaustively checked too; no generated model cache.
Each compiler/executor invocation has its own 60-second bound.
"""
import pathlib, subprocess, sys, tempfile
from net import convert
from tbl import CODE
ROOT = pathlib.Path(__file__).resolve().parents[2]

def run(args):
    return subprocess.run(list(map(str,args)), capture_output=True, timeout=60)

def require(r):
    assert r.returncode == 0, (r.args,r.returncode,r.stdout,r.stderr)
    return r

with tempfile.TemporaryDirectory(prefix='unisacc-net-') as td:
    d=pathlib.Path(td); exe=d/'run'
    require(run(['cc','-O2','-Wall','-Wextra','-o',exe,ROOT/'exec/c/run.c']))
    # Byte -> non-state stack symbol -> register observation -> acceptance.
    # Unspecified byte keys stay missing, even between real transitions.
    src=('T 4 4 1 0 0\n'
         f'Q 1 {CODE["PUSH"]} 9\n'
         f'Q 2 {CODE["LDI"]} 0 2 {CODE["RLD"]} 0\n'
         f'Q 2 {CODE["OUT"]} 89 {CODE["ACCEPT"]}\n'
         f'Q 1 {CODE["ACCEPT"]}\n'
         'R 0 2 -1 0 65 1 0 256 3 3\n'
         'R 1 2 -1 0 -1 3 3 9 2 1\n'
         'R 2 1 -1 0 2 3 2\n'
         'R 0 0 -1 0\n')
    table=d/'tiny.tbl';table.write_text(src)
    for i,path in enumerate([table,*map(pathlib.Path,sys.argv[1:])]):
        out,_,_=convert(path.read_text());net=d/(str(i)+'.net');net.write_text(out)
        r=require(run([exe,'--check-net',path,net]));sys.stdout.write(r.stdout.decode())
        if i: continue
        for data,status,stdout in [(b'A',0,b'Y'),(b'B',2,b''),(b'',0,b'')]:
            inp=d/'input';inp.write_bytes(data)
            a=run([exe,table,inp]);b=run([exe,net,inp])
            assert (a.returncode,a.stdout,a.stderr)==(b.returncode,b.stdout,b.stderr)
            assert b.returncode==status and b.stdout==stdout
        lines=out.splitlines();index=next(j for j,s in enumerate(lines) if s.startswith('H '))
        words=lines[index].split();words[5]=str(int(words[5])+1);lines[index]=' '.join(words)
        bad=d/'bad.net';bad.write_text('\n'.join(lines)+'\n')
        assert run([exe,'--check-net',table,bad]).returncode != 0,'mutated weight passed'
    print('network check: ok (execution, missing transitions, mutated bias)')
