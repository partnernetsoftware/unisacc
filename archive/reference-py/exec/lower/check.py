#!/usr/bin/env python3
"""Data-pass checks: true tape inputs, independent of the runtime generator.
The Python parser/zero_last are referees only, never fed into the delta.
"""
import os,pathlib,subprocess,sys,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT))
from unisa.tape import parse
from unisa.lower import zero_last, SCRATCH, PRINTMAX, WIN_EXTRA, WIN_STACK
from tests.enc.tins import parse as parse_header


def main():
    if len(sys.argv)<5:raise SystemExit('usage: check.py RUN TABLE JSON TAPE...')
    target=os.environ.get('LOWER_TARGET','lnx/x86_64')
    win=target.startswith('win/')
    cases=[('escapes','.bss z 24\n.str s "a;\\x00\\xff\\\\\\\""\n.str empty ""\n_start:\n  ret\n'),
           ('empty','_start:\n  ret\n'),('zero',' .bss a 19\n.str b "\\x00"\n  ret\n'),
           ('alias','.str a ""\n.str b "ab"\n.str c ""\n.bss z 8\n.str end ""\nret\n'),
           ('end-padding','.bss z 8\n.str s "abc"\n.str end ""\nret\n')]
    cases += [('alias-nonzero','.str first "abc"\n.bss z 16\n.str empty ""\n.str next "q"\nret\n'),
              ('duplicate','.str first "a"\n.str first "WRONG"\n.bss first 100\n.str other "b"\nret\n')]
    cases += [('thread-name-near-miss','.bss g___unisa_thread 4\n_start:\n  ret\n')]
    builtin=len(cases)
    for f in sys.argv[4:]:cases.append((f,pathlib.Path(f).read_text()))
    with tempfile.TemporaryDirectory() as d:
        path=pathlib.Path(d)/'tape'
        for i,(name,text) in enumerate(cases):
            path.write_text(text);t=parse(text)
            expected,syms,_=zero_last(t.data,t.syms,256)
            expected_len=len(expected)+(-len(expected))%8+(WIN_EXTRA if win else SCRATCH+PRINTMAX)
            commands=[[sys.argv[1],sys.argv[2],str(path)]]
            if i<builtin:commands.append([sys.executable,'exec/pp/sim.py',sys.argv[3],str(path)])
            for cmd in commands:
                r=subprocess.run(cmd,capture_output=True,timeout=60)
                if r.returncode:raise RuntimeError((name,r.returncode,r.stderr))
                x=parse_header(r.stdout.decode())
                assert x.os+'/'+x.arch==os.environ.get('LOWER_TARGET','lnx/x86_64'),(name,'target mismatch')
                body='\n'.join(l for l in r.stdout.decode().splitlines() if not l.startswith('@'))
                back=parse(body)
                assert len(x.data)<=len(expected) and x.data==bytes(expected[:len(x.data)]) and not any(expected[len(x.data):]) and x.syms==syms,(name,'data/symbol mismatch')
                assert x.data_len==expected_len and x.bss==(WIN_STACK if win else 0) and x.relocs==[]
                if name=='alias-nonzero':assert x.syms=={'first':256,'z':272,'empty':264,'next':264}
                if name=='duplicate':assert x.data==b'ab' and x.syms=={'first':256,'other':257}
                assert back.labels==t.labels and [(i.op,i.args) for i in back.code]==[(i.op,i.args) for i in t.code],(name,'code changed')
            print('lower data',name,expected_len,'bytes',len(syms),'symbols; executors',len(commands),flush=True)
        for name,text in [('thread-bss','.bss g___unisa_threads 4\n_start:\n  ret\n'),('thread-str','.str g___unisa_threads "x"\n_start:\n  ret\n')]:
            path.write_text(text)
            for cmd in ([sys.argv[1],sys.argv[2],str(path)],[sys.executable,'exec/pp/sim.py',sys.argv[3],str(path)]):
                r=subprocess.run(cmd,capture_output=True,timeout=60)
                # H3d S1: thread mode is accepted; the header carries @threads 1 and the data is the plain layout
                assert r.returncode==0,(name,cmd,r.returncode,r.stderr)
                x=parse_header(r.stdout.decode())
                assert getattr(x,'threads',0)==1 and 'g___unisa_threads' in x.syms,(name,'thread header/symbol')
            print('lower data',name,'@threads 1; executors 2',flush=True)
        path.write_text(cases[0][1])
        r=subprocess.run([sys.argv[1],sys.argv[2],str(path)],capture_output=True,timeout=60)
        assert b'@threads' not in r.stdout,'threads header on a program without threads'
if __name__=='__main__':main()
