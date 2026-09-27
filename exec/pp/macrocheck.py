#!/usr/bin/env python3
"""Macro expansion: full reference bytes, host tokens and actual network execution.
The C99 examples remain sourced from ccparity, whose extraction is checked.
All children have a 60 s bound; Python is only an offline builder/test oracle.
"""
import json,os,pathlib,re,struct,subprocess,sys,tempfile
R=pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0,str(R/'exec/pp'));import sim
sys.path.insert(0,str(R));from unisa.front.pp import _pieces

def run(args):return subprocess.run(list(map(str,args)),capture_output=True,timeout=60)
def call(args):
 p=run(args);assert p.returncode==0,(p.args,p.returncode,p.stderr[-2000:]);return p.stdout

def text_of(b):
 assert b[:7]==b'UNIPP1\0';length,_,_,ns,ni=struct.unpack_from('<5I',b,7);i=27+4*ns
 for _ in range(ni):i+=12+struct.unpack_from('<I',b,i+8)[0]
 assert len(b)==i+length
 return b[i:]

def tokens(b):return [s for s in _pieces(b.decode()) if not s.isspace()]

cases={'zero': '#define F() 7\nF F() F( )\n',
 'zeroempty': '#define F()\na F() b\n',
 'va': '#define V(...) __VA_ARGS__\nV() V(1) V(1,2,(3,4))\n',
 'va_named': '#define V(x,...) x+(__VA_ARGS__)\nV(1) V(1,) V(1,2,3)\n',
 'va_string': '#define S(...) #__VA_ARGS__\nS() S(a, b, "c\\\\d")\n',
 'cross': '#define A F(\n#define F(x) x+x\nA 3)\n',
 'crossnested': '#define A F((\n#define F(x) x+x\nA 3))\n',
 'objhash': '#define H # ## #\nH\n',
 'arg_hash': '#define S(x) #x\nS(#) S(##)\n',
 'hide': '#define A A+1\n#define F(x) x+x\nF(A)\n',
 'pastehide': '#define A A\n'
              '#define AB 7\n'
              '#define CAT(x,y) x ## y\n'
              '#define OUT(x,y) CAT(x,y)\n'
              'OUT(A,B)\n',
 'empty_paste': '#define T(x,y,z) x ## y ## z\nT(,,) T(a,,b) T(,a,)\n',
 'arg_nested': '#define F(x,y) x+y\n#define G(x) x\nF(G(1),G(G(2)))\n',
 'crosszero': '#define A F(\n#define F() 9\nA )\n',
 'crosslines': '#define A F(\n#define F(x) x+x\nA\n 3)\n',
 'multi-left': '#define A A+1\n'
               '#define B B\n'
               '#define BB 7\n'
               '#define CAT(x,y) x ## y\n'
               '#define OUT(x,y) CAT(x,y)\n'
               'OUT(A B,B)\n',
 'multi-right': '#define A A+1\n'
                '#define B B\n'
                '#define BB 7\n'
                '#define CAT(x,y) x ## y\n'
                '#define OUT(x,y) CAT(x,y)\n'
                'OUT(B,B A)\n'}
examples=re.findall(r'ecase "(-E C99 6\.10\.3\.5 ex[347])" \'(.*?)\'',(R/'tests/ccparity.sh').read_text(),re.S)
assert [n.rsplit(' ',1)[-1] for n,_ in examples]==['ex3','ex4','ex7']
cases.update((n.rsplit(' ',1)[-1],s+'\n') for n,s in examples)
cases.update({
 'comma_empty': '#define P(x,...) f(x, ##__VA_ARGS__)\nP(1) P(1,2) P(1,2,3)\n',
 'comma_nested': '#define P(x,...) f(x, ##__VA_ARGS__)\n#define V(...) P(__VA_ARGS__)\nV(1) V(1,2,3)\n',
 'comma_raw': '#define E\n#define V(...) a, ##__VA_ARGS__\nV(E)\n',
 'comma_space': '#define P(x,...) f(x , ## __VA_ARGS__)\nP(1) P(1, 2)\n',
})
# The reference also elides an explicitly supplied empty final argument;
# host compilers may distinguish it from an omitted argument. Pin this
# reference contract with an independent token expectation instead.
reference_tokens={'comma_explicit': ['f','(','1',')','f','(','1',')']}
cases['comma_explicit']='#define P(x,...) f(x, ##__VA_ARGS__)\nP(1,) P(1, )\n'
assert len(cases)==25
with tempfile.TemporaryDirectory(prefix='pp-macros-') as td:
 t=pathlib.Path(td)
 call(['sh','-c','R=$1; . "$R/tests/lib.sh"; ua_ready','macrocheck',R])
 ref=os.environ.get('UA','/tmp/ua_ref')
 call([os.environ.get('EXEC_CC','cc'),'-O2',R/'exec/c/run.c','-o',t/'run'])
 for located in (False,True):
  q='loc' if located else 'plain'
  call([sys.executable,R/'exec/pp/gen.py',t/(q+'.json'),*(['--locations'] if located else [])])
  call([sys.executable,R/'exec/c/tbl.py',t/(q+'.json'),t/(q+'.tbl')])
  call([sys.executable,R/'exec/c/net.py',t/(q+'.tbl'),t/(q+'.net')])
  call([t/'run','--check-net',t/(q+'.tbl'),t/(q+'.net')])
  d=json.loads((t/(q+'.json')).read_text());loaded=sim.load(d)
  for name,source in cases.items():
   f=t/(name+'.c');f.write_text(source)
   want=call([ref,'-E',f])
   if name in reference_tokens:
    assert tokens(want)==reference_tokens[name],(name,'reference contract',want)
   else:
    host=call(['cc','-E','-P',f])
    assert tokens(want)==tokens(host),(name,'reference/host tokens',want,host)
   got=call([t/'run',t/(q+'.net'),f,f,R/'include'])
   assert (text_of(got) if located else got)==want,(name,q,got,want)
   verdict,value,_=sim.run(d,f.read_bytes(),str(f),sim.Files(),maxsteps=2000000,loaded=loaded)
   assert verdict=='accept' and value==got,(name,q,verdict,value)
  print(q,len(cases),'full outputs; 24 host-token cases, 1 explicit reference contract; simulator/network agree',flush=True)
 print('macro checks: 50 full results, both preprocessing formats')
