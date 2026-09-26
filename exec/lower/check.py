#!/usr/bin/env python3
"""Data-pass checks: true tape inputs, independent of the runtime generator.
The Python parser/zero_last are referees only, never fed into the delta.
"""
import pathlib,subprocess,sys,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT))
from unisa.tape import parse
from unisa.lower import zero_last, SCRATCH, PRINTMAX
from exec.enc.tins import parse as parse_header


def main():
    if len(sys.argv)<5:raise SystemExit('usage: check.py RUN TABLE JSON TAPE...')
    cases=[('escapes','.bss z 24\n.str s "a;\\x00\\xff\\\\\\\""\n.str empty ""\n_start:\n  ret\n'),
           ('empty','_start:\n  ret\n'),('zero',' .bss a 19\n.str b "\\x00"\n  ret\n'),
           ('alias','.str a ""\n.str b "ab"\n.str c ""\n.bss z 8\n.str end ""\nret\n'),
           ('end-padding','.bss z 8\n.str s "abc"\n.str end ""\nret\n')]
    cases += [('alias-nonzero','.str first "abc"\n.bss z 16\n.str empty ""\n.str next "q"\nret\n'),
              ('duplicate','.str first "a"\n.str first "WRONG"\n.bss first 100\n.str other "b"\nret\n')]
    builtin=len(cases)
    for f in sys.argv[4:]:cases.append((f,pathlib.Path(f).read_text()))
    with tempfile.TemporaryDirectory() as d:
        path=pathlib.Path(d)/'tape'
        for i,(name,text) in enumerate(cases):
            path.write_text(text);t=parse(text)
            expected,syms=zero_last(t.data,t.syms,256)
            expected+=bytes((-len(expected))%8+SCRATCH+PRINTMAX)
            commands=[[sys.argv[1],sys.argv[2],str(path)]]
            if i<builtin:commands.append([sys.executable,'exec/pp/sim.py',sys.argv[3],str(path)])
            for cmd in commands:
                r=subprocess.run(cmd,capture_output=True,timeout=60)
                if r.returncode:raise RuntimeError((name,r.returncode,r.stderr))
                x=parse_header(r.stdout.decode())
                body='\n'.join(l for l in r.stdout.decode().splitlines() if not l.startswith('@'))
                back=parse(body)
                assert len(x.data)<=len(expected) and x.data==bytes(expected[:len(x.data)]) and not any(expected[len(x.data):]) and x.syms==syms,(name,'data/symbol mismatch')
                assert x.data_len==len(expected) and x.bss==0 and x.relocs==[]
                if name=='alias-nonzero':assert x.syms=={'first':256,'z':272,'empty':264,'next':264}
                if name=='duplicate':assert x.data==b'ab' and x.syms=={'first':256,'other':257}
                assert back.labels==t.labels and [(i.op,i.args) for i in back.code]==[(i.op,i.args) for i in t.code],(name,'code changed')
            print('lower data',name,len(expected),'bytes',len(syms),'symbols; executors',len(commands),flush=True)
if __name__=='__main__':main()
