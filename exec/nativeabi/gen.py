#!/usr/bin/env python3
"""Standalone model-certified FFI_CARRIER first slice; raw USLSIG2 or complete ordered USLSIG3 input.
Explicit resource \\0cli/target selects one of six exact profile rules.
MS.canonical validates the entire graph; MG.root indexes its canonical form.
No host ABI classifier or new executor action. Rule/native qualification are
separate: this stage proves its finite rule, not six-platform native execution.
"""
import csv,importlib.util,json,pathlib,sys
ROOT=pathlib.Path(__file__).resolve().parents[2]
sys.path[:0]=[str(ROOT/'exec'),str(ROOT)]
from modelgraphequality import install as graph_install,FIELDS,EDGES,LAYOUT
from finite_rules import install as install_rules
from ordered import install as ordered_install,REGS as ORDERED_REGS
SIGWIRE,SIGMARK,FRAME=(i<<40 for i in range(480,483))

def install(E):
 P=E.P;graph_install(E)
 owned={SIGWIRE,SIGMARK,FRAME}
 for module in tuple(sys.modules.values()):
  if module is not None and module.__name__!=__name__:
   assert not owned.intersection(v for v in vars(module).values() if type(v) is int and v>=1<<40),module.__name__
 lines=(pathlib.Path(__file__).parent/'rules.tsv').read_text().splitlines()
 rows=list(csv.DictReader(lines[:7],delimiter='\t'))
 declarations=[x.split('\t') for x in lines[7:] if not x.startswith('#')]
 leafclasses={int(x[1]):'NSI'.index(x[2]) for x in declarations if x[0]=='leaf'}
 joins={(x[1],x[2]):x[3] for x in declarations if x[0]=='merge'}
 assert all(joins[a,b]=='NSI'[max('NSI'.index(a),'NSI'.index(b))] for a in 'NSI' for b in 'NSI')
 recipes={(int(x[1]),x[2]):x[3] for x in declarations if x[0]=='recipe'}
 assert len(recipes)==8 and leafclasses=={1:2,2:2,3:1}
 profiles={r['profile'] for r in rows}
 assert profiles=={f'{os}/{arch}' for os in ('osx','lnx','win') for arch in ('arm64','x86_64')} and len(rows)==6
 policy={r['profile']:(int(r['integer_min_width']),int(r['homogeneous_fp_carrier_kind'])) for r in rows}
 # Target long double representation: 2 IEEE64 (Apple arm64, Windows), 3 x87 80-bit, 4 IEEE128 (Linux arm64).
 ldformat={r['profile']:int(r['long_double_format']) for r in rows}
 assert set(ldformat.values())<={2,3,4}
 assert all(r['rule']=='natural_scalar_union_v2' and int(r['mixed_width'])==8 for r in rows)
 assert all(lo in (1,8) and fp in (1,3) for lo,fp in policy.values())
 def word(p,r):return p.a(('COPYW','ms_value',r)).call('MS.write64')
 def constword(p,v):return word(p.a(('LDI','nc_v',v)),'nc_v')
 def blob(p,r,n):return p.a(('INPUSH',r),('SPAN2','nc_zero',n),('INPOP',))
 def field(p,index,out='nc_v',node='nc_node'):return p.a(('ALUI','mul','nc_key',node,16),('ALUI','add','nc_key','nc_key',index),('LDX',out,'nc_key',FIELDS))
 # Stage control lives in gen-result.tsv (sections head/mid/tail); Python keeps only the profile
 # trie (states per rules.tsv profile prefix) and the union16 recipe writers (rules.tsv recipes).
 root=pathlib.Path(__file__).parent
 fresh=[l.split('\t') for l in (root/'gen-fresh.tsv').read_text().splitlines()[1:]]
 bindings=dict(FIELDS=FIELDS,EDGES=EDGES,FRAME=FRAME,SIGWIRE=SIGWIRE,SIGMARK=SIGMARK)
 def section(name):
  for part,key,prefix,kind in fresh:
   if part==name:bindings[key]=P(prefix+'.fresh').fresh(kind)
  install_rules(E.g,root,'gen',bindings,{},{},name)
 P('NC.unsupported').a(E.rej('not covered: native ABI carrier rule')).goto('DEAD')
 P('NC.targetfail').a(E.rej('not covered: native ABI target profile')).goto('DEAD')
 section('head')
 # Exact byte trie: no prefix, unknown suffix or absent-resource fallback.
 prefixes={b''}
 for profile in profiles:
  raw=profile.encode();prefixes.update(raw[:i] for i in range(1,len(raw)+1))
 names={p:'NC.profile'+('' if not p else '.'+p.hex()) for p in prefixes}
 for prefix in sorted(prefixes):
  if prefix.decode() in profiles:
   P(names[prefix]).a(('LDI','nc_intminimum',policy[prefix.decode()][0]),('LDI','nc_fpcarrier',policy[prefix.decode()][1]),('LDI','nc_ldformat',ldformat[prefix.decode()]),('LDI','nc_family',int(next(r['family'] for r in rows if r['profile']==prefix.decode()))),('MARK','nc_pos')).branch({1:'NC.profileok'},'NC.targetfail',[('C64U','nc_pos','nc_targetlen')])
  else:
   choices={p[len(prefix)]:names[p] for p in prefixes if len(p)==len(prefix)+1 and p.startswith(prefix)}
   P(names[prefix]).a(('MARK','nc_pos')).branch({0:names[prefix]+'.read'},'NC.targetfail',[('C64U','nc_pos','nc_targetlen')])
   P(names[prefix]+'.read').a(('BYTE','nc_byte'),('ADV',)).branch(choices,'NC.targetfail',[('RLD','nc_byte')])
 section('mid')
 for alignment in (4,8):
  choices={3*'NSI'.index(c[0])+'NSI'.index(c[1]):'NC.recipe'+str(alignment)+c for c in ('II','IS','SI','SS')}
  P('NC.union16recipes'+str(alignment)).branch(choices,'NC.unsupported',[('RLD','n16_code')])
  for classes in ('II','IS','SI','SS'):
   elements=recipes[alignment,classes];p=P('NC.recipe'+str(alignment)+classes)
   for value in (0,0,0,5,16,0,alignment):constword(p,value)
   p.a(('LDI','nc_v',1),('OUTW','nc_v'),('OLEN','nc_payloadcut'));constword(p,len(elements))
   for index,cls in enumerate(elements):
    for value in (index*alignment,0,0,alignment):constword(p,value)
    for value in (0,0,0,1 if cls=='I' else 3,alignment,int(cls=='I'),alignment):constword(p,value)
    p.a(('OUTW','nc_zero'));constword(p,0)
   p.goto('NC.cbfinish')
 section('tail')
 extents=tuple(int(x[1]) for x in declarations if x[0]=='ordered_extent')
 alignments=tuple(int(x[1]) for x in declarations if x[0]=='ordered_alignment')
 assert extents==tuple(range(1,17)) and alignments==(1,2,4,8)
 assert [x[1] for x in declarations if x[0]=='ordered_anon_policy']==['invariant_integer_lanes']
 ordered_install(E,field,word,constword,blob,extents,alignments)
 return 'NC.START'

def build():
 spec=importlib.util.spec_from_file_location('nativecarrierbase',ROOT/'exec/parse/gen.py');E=importlib.util.module_from_spec(spec);spec.loader.exec_module(E)
 start=install(E);E.g.finish()
 return {'start':start,'states':{n:[m,{str(k):v for k,v in row.items()}] for n,(m,row) in E.g.st.items()},'seqs':[list(map(list,s)) for s in E.g.seqs]}
if __name__=='__main__':
 d=build();pathlib.Path(sys.argv[1]).write_text(json.dumps(d,separators=(',',':')));print('native carrier states',len(d['states']))
