#!/usr/bin/env python3
"""Check E2 -> E1 -> E3 location framing without changing generated tape.
All generation, compilation and execution children have their own 60s bound.
"""
import json,os,pathlib,struct,subprocess,sys,tempfile
import sys as _sys, pathlib as _pl; _sys.path.insert(0, next(str(_p / 'tests') for _p in _pl.Path(__file__).resolve().parents if (_p / 'tests/checklib.py').is_file()))
from checklib import run
R=pathlib.Path(__file__).resolve().parents[2]

def call(args):
    p=run(args);assert p.returncode==0,(p.args,p.returncode,p.stderr[-1500:]);return p.stdout

with tempfile.TemporaryDirectory(prefix='parse-locations-') as td:
    t=pathlib.Path(td)
    call([os.environ.get('EXEC_CC','cc'),'-O2',R/'exec/c/run.c','-o',t/'run'])
    for name,script,flags in [('pp','build/gen.py',['pp','--locations']),('lex','build/gen.py',['lex','--locations']),
                            ('typed','build/gen.py',['lex','--typed']),('parse','parse2/gen2.py',['--locations']),
                            ('plain','parse2/gen2.py',[])]:
        call([sys.executable,R/'exec'/script,t/(name+'.json'),*flags])
        call([sys.executable,R/'exec/c/tbl.py',t/(name+'.json'),t/(name+'.tbl')])
        call([sys.executable,R/'exec/c/net.py',t/(name+'.tbl'),t/(name+'.net')])
    # Test-only model continuation reads every retained map field back out.
    # It replaces the grammar entry, not the location decoder being tested.
    from tokenlocations import SPLICES, INCLUDE_LINE, INCLUDE_LINES, INCLUDE_NAME
    probe=json.loads((t/'parse.json').read_text())
    def state(name,nxt,acts):
        ix=len(probe['seqs']);probe['seqs'].append(acts)
        probe['states'][name]=['b',{str(k):[nxt,ix] for k in range(257)}]
    def branch(name,cases):
        probe['states'][name]=['r',{str(k):[cases[k],len(probe['seqs'])] for k in cases}]
        probe['seqs'].append([])
    def word(reg):
        return sum(([['ALUI','sar','probe_byte',reg,s],['OUTW','probe_byte']] for s in (0,8,16,24)),[])
    # All START transitions are bypassed only in this probe model.
    for pair in probe['states']['DL.ready'][1].values():
        assert pair[0]=='START';pair[0]='PROBE.fields'
    state('PROBE.fields','PROBE.sp', [['OUT',b] for b in b'UNIPP1\0']+
          sum((word('diag_'+f) for f in ['textlen','forced','auto','nsplice','ninclude']),[])+
          [['LDI','probe_i',0],['CMP','probe_i','diag_nsplice']])
    branch('PROBE.sp',{0:'PROBE.splice',1:'PROBE.inc0',2:'PROBE.inc0'})
    state('PROBE.splice','PROBE.sp',[['LDX','probe_v','probe_i',SPLICES]]+word('probe_v')+
          [['ALUI','add','probe_i','probe_i',1],['CMP','probe_i','diag_nsplice']])
    state('PROBE.inc0','PROBE.inc',[['LDI','probe_i',0],['CMP','probe_i','diag_ninclude']])
    branch('PROBE.inc',{0:'PROBE.include',1:'PROBE.text',2:'PROBE.text'})
    state('PROBE.include','PROBE.inc', [['LDX','probe_v','probe_i',INCLUDE_LINE]]+word('probe_v')+
          [['LDX','probe_v','probe_i',INCLUDE_LINES]]+word('probe_v')+
          [['LDX','probe_name','probe_i',INCLUDE_NAME],['BLEN','probe_v','probe_name']]+word('probe_v')+
          [['LDI','probe_zero',0],['INPUSH','probe_name'],['SPAN2','probe_zero','probe_v'],['INPOP'],
           ['ALUI','add','probe_i','probe_i',1],['CMP','probe_i','diag_ninclude']])
    state('PROBE.text','DEAD',[['LDI','probe_zero',0],['INPUSH','diag_source'],
           ['SPAN2','probe_zero','diag_textlen'],['INPOP'],['ACCEPT']])
    (t/'probe.json').write_text(json.dumps(probe,separators=(',',':')))
    call([sys.executable,R/'exec/c/tbl.py',t/'probe.json',t/'probe.tbl'])
    call([sys.executable,R/'exec/c/net.py',t/'probe.tbl',t/'probe.net'])
    (t/'local.h').write_text('static int twice(int n){return n*2;}\n')
    cases=[('small','int main(void){return 3;}\n'),
           ('macro','#define VALUE 12345\nint main(void){return VALUE;}\n'),
           ('splice','int main(void){return 1+\\\n2;}\n'),
           ('header','#include "local.h"\nint main(void){return twice(3);}\n'),
           ('strings','int main(void){char a[4]="xy"; char *p="a" "b";return a[1]+p[0];}\n'),
           ('printf','int main(void){printf("%d %s\\n",3,"ok");return 0;}\n')]
    cases.append(('static-ordinals','int f(void){static int x=3;return x++;} int main(void){static int y=7;return f()+y;}\n'))
    cases.append(('paren-declarator',
                  'int (twice)(int x){return x*2;} int (value)=5; int main(void){return twice(21)+value;}\n'))
    cases += [(p.stem,p.read_text()) for p in [R/'examples/fib.c',R/'tests/c/b_strderef.c']]
    for name,src in cases:
        f=t/'source.c';f.write_text(src)
        pp=call([t/'run',t/'pp.net',f,f,R/'include']);(t/'pp').write_bytes(pp)
        tokens=call([t/'run',t/'lex.net',t/'pp']);(t/'tokens').write_bytes(tokens)
        n=struct.unpack_from('<I',pp,7)[0];(t/'text').write_bytes(pp[-n:] if n else b'')
        plain=call([t/'run',t/'typed.net',t/'text']);(t/'plain').write_bytes(plain)
        want=call([t/'run',t/'plain.net',t/'plain'])
        got=call([t/'run',t/'parse.net',t/'tokens']);assert got==want,name
        assert call([t/'run',t/'probe.net',t/'tokens'])==pp,(name,'retained map')
        print('located parser',name,'tape unchanged',flush=True)
    # Perturb a valid joined frame; errors are rejected, with no partial tape.
    ppstart=12;ppend=ppstart+len(pp)
    bad=[b'',tokens[:7],tokens[:11],b'BADMAG!!'+tokens[8:],
         tokens[:8]+struct.pack('<I',len(tokens))+tokens[12:],
         tokens[:ppstart]+b'BADMAG!'+tokens[ppstart+7:],
         tokens[:ppstart+7]+struct.pack('<I',2**31)+tokens[ppstart+11:],
         tokens[:ppend]+b'!'+tokens[ppend+1:],
         tokens[:ppend+1]+struct.pack('<I',n+1)+tokens[ppend+5:],
         tokens[:ppend+5]+b'!'+tokens[ppend+6:]]
    for i,b in enumerate(bad):
        f=t/'bad';f.write_bytes(b);p=run([t/'run',t/'parse.net',f])
        assert p.returncode==1 and not p.stdout and b'malformed token locations' in p.stderr,(i,p.returncode,p.stderr)
    print('located parser:',len(cases),'tape comparisons, 10 malformed frames rejected')
