#!/usr/bin/env python3
"""Macro expansion: full reference bytes, host tokens and actual network execution.
The C99 examples remain sourced from ccparity, whose extraction is checked.
All children have a 60 s bound; Python is only an offline builder/test oracle.
"""
import json,os,pathlib,re,struct,subprocess,sys,tempfile
R=pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0,str(R/'exec/pp'));import sim
sys.path.insert(0,str(R));from unisa.front.pp import _pieces
import sys as _sys, pathlib as _pl; _sys.path.insert(0, next(str(_p / 'tests') for _p in _pl.Path(__file__).resolve().parents if (_p / 'tests/checklib.py').is_file()))
from checklib import run

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
pragma_cases={
 'pragma_hidden_call': ('#define _Pragma _Pragma\n#define F(x) x\nF(_Pragma)("x") z\n', ['z']),
 'pragma_hidden_arg': ('#define _Pragma _Pragma\n#define F(x) x\nF(_Pragma("x")) z\n', ['z']),
 'pragma_defined_arg': ('#define _Pragma 4\n#define F(x) x\nF(_Pragma) z\n', ['4','z']),
 'pragma_root': ('a _Pragma("pack(1)") b\n', ['a','b']),
 'pragma_bare': ('_Pragma\n', ['_Pragma']),
 'pragma_lines': ('_Pragma\n("x")\nb\n', ['b']),
 'pragma_object': ('#define P _Pragma("x")\na P b\n', ['a','b']),
 'pragma_stringize': ('#define P(x) _Pragma(#x)\na P(pack(1)) b\n', ['a','b']),
 'pragma_nameframe': ('#define P _Pragma\na P("x") b\n', ['a','b']),
 'pragma_callframe': ('#define P _Pragma(\na P "x") b\n', ['a','b']),
 'pragma_argument': ('#define F(x) x\na F(_Pragma("x")) b\n', ['a','b']),
 'pragma_nocall': ('#define P _Pragma\nP + 1\n', ['_Pragma','+','1']),
 'pragma_end': ('#define P _Pragma\nP\n', ['_Pragma']),
 'pragma_framelines': ('#define P _Pragma\nP\n("x") z\n', ['z']),
 # Reference ignores balanced contents, even forms outside C99's operand grammar.
 'pragma_balanced': ('_Pragma((a),"x)",\'q\')z\n', ['z']),
 'pragma_unexpanded': ('#define F(x) x\n_Pragma(F(a,b))z\n', ['z']),
 'pragma_defined': ('#define _Pragma 4\n_Pragma\n', ['4']),
 'pragma_definedframe': ('#define _Pragma 4\n#define P _Pragma\nP\n', ['4']),
}
for name,(source,want) in pragma_cases.items():
 cases[name]=source;reference_tokens[name]=want
assert len(cases)==43
# Independent expected truth values, exercising every integer reduction family.
expressions = ['2+3*4==14', '17/5==3 && 17%5==2', '(1<<5)==32 && (32>>3)==4',
 '(6&3)==2 && (4|1)==5 && (7^3)==4', '1<2 && 2<=2 && 3>2 && 3>=3 && 1!=2',
 '!0 && !(!1) && (~0)==-1 && -3+ +4==1', '0 && (1/0)', '1 || (1/0)',
 '(1 ? 7 : 1/0)==7', '(0 ? 1/0 : 9)==9', '(1 ? 0 : 1) ? 0 : 1',
 'defined(ONE) && defined TWO && !defined(NOPE)', 'TWO==2 && UNDECLARED==0']
truth = [True, True, True, True, True, True, False, True, True, True, True, True, True]
cases['integer_reductions'] = '#define ONE 1\n#define TWO ONE+1\n' + ''.join(
 '#if '+expr+'\nYES'+str(i)+'\n#else\nNO'+str(i)+'\n#endif\n'
 for i,expr in enumerate(expressions))
integer_want = [('YES' if yes else 'NO')+str(i) for i,yes in enumerate(truth)]

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
   if name=='integer_reductions': assert tokens(want)==integer_want,(name,want)
   if name in reference_tokens:
    assert tokens(want)==reference_tokens[name],(name,'reference contract',want)
   else:
    host=call(['cc','-E','-P',f])
    assert tokens(want)==tokens(host),(name,'reference/host tokens',want,host)
   got=call([t/'run',t/(q+'.net'),f,f,R/'include'])
   assert (text_of(got) if located else got)==want,(name,q,got,want)
   verdict,value,_=sim.run(d,f.read_bytes(),str(f),sim.Files(),maxsteps=2000000,loaded=loaded)
   assert verdict=='accept' and value==got,(name,q,verdict,value)
  for name,source in {'pragma_unclosed':'_Pragma("x"\n',
                      'pragma_badliteral':'_Pragma("x\n'}.items():
   f=t/(name+'.c');f.write_text(source)
   result=run([t/'run',t/(q+'.net'),f,f,R/'include'])
   assert result.returncode!=0 and b'not covered: unterminated _Pragma' in result.stderr,(name,q,result)
   verdict,value,_=sim.run(d,f.read_bytes(),str(f),sim.Files(),maxsteps=2000000,loaded=loaded)
   assert verdict!='accept' and 'unterminated _Pragma' in str(value),(name,q,verdict,value)
  print(q,len(cases),'full outputs;',len(cases)-len(reference_tokens),'host-token cases;',len(reference_tokens),'explicit reference contracts; 2 named refusals; simulator/network agree',flush=True)
 print('macro checks:',2*len(cases),'full results and 4 refusals, both preprocessing formats')
