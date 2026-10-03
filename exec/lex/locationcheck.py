#!/usr/bin/env python3
"""E2 -> located E1 framing; compare all token bytes and retained metadata.
The E2 reference-map test and E1 tpos test independently check the contents.
This test checks their join and rejects malformed framing without output.
"""
import json,os,pathlib,struct,subprocess,sys,tempfile
R=pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0,str(R/'exec/pp'));import sim
import sys as _sys, pathlib as _pl; _sys.path.insert(0, next(str(_p / 'tests') for _p in _pl.Path(__file__).resolve().parents if (_p / 'tests/checklib.py').is_file()))
from checklib import run




def call(args):
    p=run(args);assert p.returncode==0,(p.args,p.returncode,p.stderr[-1000:]);return p.stdout


def text_of(b):
    assert b[:7]==b'UNIPP1\0';length,_,_,ns,ni=struct.unpack_from('<5I',b,7)
    i=27+4*ns
    for _ in range(ni):
        n=struct.unpack_from('<I',b,i+8)[0];i+=12+n
    assert len(b)==i+length
    return b[i:]


with tempfile.TemporaryDirectory(prefix='lex-location-chain-') as td:
    t=pathlib.Path(td)
    call([os.environ.get('EXEC_CC','cc'),'-O2',R/'exec/c/run.c','-o',t/'run'])
    for name,script,flags in [('pp','pp',['--locations']),('lex','lex',['--locations']),('pos','lex',['--positions'])]:
        call([sys.executable,R/'exec/build/gen.py',script,t/(name+'.json'),*flags])
        call([sys.executable,R/'exec/c/tbl.py',t/(name+'.json'),t/(name+'.tbl')])
        call([sys.executable,R/'exec/c/net.py',t/(name+'.tbl'),t/(name+'.net')])
    d=json.loads((t/'lex.json').read_text());d['states']['HALT']=['b',{}]
    d['seqs']=[[["ALUI","add",a[1],a[1],1] if a[0]=='INC' else a for a in seq] for seq in d['seqs']]
    loaded=sim.load(d)
    (t/'inner.h').write_text('int header;\n')
    (t/'outer.h').write_text('#include "inner.h"\n#define VALUE 7\n')
    cases=['', 'int x;\n', '#define V 12345\nint x=V;\n',
           'int x=1+\\\n2;\n', '#include "outer.h"\nint x=VALUE;\n',
           'int main(void){printf("ok\\n");return 0;}\n']
    for i,src in enumerate(cases):
        f=t/'source.c';f.write_text(src)
        pp=call([t/'run',t/'pp.net',f,f,R/'include']);(t/'pp').write_bytes(pp)
        text=text_of(pp);(t/'text').write_bytes(text)
        want=b'UNITOK1\0'+struct.pack('<I',len(pp))+pp+call([t/'run',t/'pos.net',t/'text'])
        got=call([t/'run',t/'lex.net',t/'pp']);assert got==want,i
        result,val,_=sim.run(d,pp,str(f),loaded=loaded,maxsteps=3000000)
        assert result=='accept' and val==want,(i,result)
        print('located lexer chain',i,'matches direct token positions',flush=True)
    good=b'UNIPP1\0'+struct.pack('<5I',0,0,0,0,0)
    bad=[b'',good[:6],good[:26],b'BADMAG!'+good[7:],good+b'x',
         good[:7]+struct.pack('<5I',1,0,0,0,0),
         good[:7]+struct.pack('<5I',0,0,0,1,0),
         good[:7]+struct.pack('<5I',0,0,0,1,0)+struct.pack('<I',1),
         good[:7]+struct.pack('<5I',0,0,0,0,1)+struct.pack('<3I',1,1,63)+b'x'*63,
         good[:7]+struct.pack('<5I',0,0,0,0,1)+struct.pack('<3I',1,1,2)+b'x',
         good[:7]+struct.pack('<5I',2**31,0,0,0,0)]
    for i,b in enumerate(bad):
        f=t/'bad';f.write_bytes(b);p=run([t/'run',t/'lex.net',f])
        assert p.returncode==1 and not p.stdout and b'malformed preprocessing locations' in p.stderr,(i,p.returncode,p.stderr)
    print('located lexer: 6 joined cases, 11 malformed frames rejected')
