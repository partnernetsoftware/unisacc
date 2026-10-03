"""Explicit model callable introduction/typed indirect call and declaration catalog.
Disabled resource follows the exact old states. Handles never use raw callr.
SCRIPT/NATIVE origin is selected after final source definitions are known.
"""
import sys as _s,pathlib as _p
_s.path.insert(0,str(_p.Path(__file__).parents[1]/'facts'))
from load import facts as _facts
_t=lambda v:tuple(map(_t,v)) if isinstance(v,list) else v
globals().update((_r['name'],_t(_r['value'])) for _r in _facts('librarycallables'))
assert not set(range(440,446)) & (set(range(72,83))|set(range(300,358))|set(range(400,438)))
def install(E,P,b,start):
 from libraryexports import RETURNRANK, PARAMRANK
 from valueranks import LCSITERANK
 import assemble
 u64=lambda E,l,k,r,p,f:assemble.run(assemble.FACTS.parent/'modelinput-manifest.tsv',E,E.P,{},dict(entry=l,key=[('SBOUT',c) for c in k],result=r,present=p,fail=f))  # modelinput-manifest.tsv
 from libraryimports import BYNAME,ADDRESS,FORMAT,SUPPORTED,TYPEDSIG,PLAN
 from libraryvariadic import REQUESTS
 from unresolved import DEFINED
 from libraryexports import VARIADIC
 import sys
 owned={SITE_SIG,SITE_KEY,SITE_FIXED,SITE_COUNT,SITE_DEPTH,SITE_BASE,SITE_SHAPE,ALIASID,ALIASKEY,ALIASPLAN,ALIASRAW,ALIASEXTERNAL,CATALOGBLOB}
 for module in tuple(sys.modules.values()):
  if module is not None and module.__name__!=__name__:
   assert not owned.intersection(v for v in vars(module).values() if type(v) is int and v>=1<<40),module.__name__
 g=E.g
 # Stage control: librarycallables-result.tsv sections s1..s14; parent-row edits (hooks,
 # renames to LC.original.*, aliases) in librarycallables-template.tsv, interleaved below so the global fresh order is unchanged.
 # Python binds only dynamic facts: bank ids, b[...]/E descriptor codes, the start
 # continuation, the FS.CALLTYPE continuation (fpcont), and fresh labels allocated in
 # librarycallables-fresh.tsv order. Output text rows are ["@","out:TEXT"] sequences.
 import json
 from pathlib import Path
 from finite_rules import install as rules,install_template
 root=Path(__file__).parent
 fresh=[l.split('\t') for l in (root/'librarycallables-fresh.tsv').read_text().splitlines()[1:]]
 sequences={}
 for line in (root/'librarycallables-result.tsv').read_text().splitlines()[1:]:
  for action in json.loads(line.split('\t')[4]):
   if action[0]=='@' and action[1].startswith('out:'):sequences[action[1]]=E.O(action[1][4:])
 constants=dict({k:v for k,v in globals().items() if type(v) is int and v>=1<<40},
  RETURNRANK=RETURNRANK,PARAMRANK=PARAMRANK,LCSITERANK=LCSITERANK,BYNAME=BYNAME,ADDRESS=ADDRESS,FORMAT=FORMAT,
  SUPPORTED=SUPPORTED,TYPEDSIG=TYPEDSIG,PLAN=PLAN,REQUESTS=REQUESTS,DEFINED=DEFINED,VARIADIC=VARIADIC,
  E_VAR=E.VAR,E_DBL=E.DBL,E_INT=E.SZ['int'],TK_SEMI=E.TK[';'],
  **{'b_'+k:b[k] for k in ('FPS_FN','FPS_COUNT','FPS_RB','FPS_RD','FPS_RSH','FPS_VAR','SSZ','SBB')})
 classes={'uns1':[E.UNS+1],'uns2':[E.UNS+2],'bool':[b['BOOL']],'float':[b['FLT']]}
 def section(name,**extra):
  owners={};bindings=dict(constants,start=start,**extra)
  for part,key,prefix,kind in fresh:
   if part!=name:continue
   # A fresh label carries its allocating state's prefix (fpcont is not LC-owned).
   assert prefix==('LC' if 'fpcont' not in extra or prefix=='LC' else extra['fpcont'].split('.')[0])
   if prefix not in owners:owners[prefix]=P(prefix+'.fresh.librarycallables.'+name)
   bindings[key]=owners[prefix].fresh(kind)
  rules(g,root,'librarycallables',bindings,sequences,classes,section=name)
 class _Scope:
  def __init__(self,cur):self.cur=cur
 def tmpl(name,**facts):
  install_template(g,root,'librarycallables',facts,lambda kind:E.P.fresh(_Scope('LC'),kind),section=name)
 def move(old,new):tmpl('move',m=[dict(old=old,new=new)])
 def hook(name,proc):move(name,'LC.original.'+name);tmpl('hookcall',h=[dict(name=name,proc=proc)])
 def alias(new,src):tmpl('copy',c=[dict(new=new,src=src)])
 u64(E,'LC.resource',b'\0library/callables','lc_enabled','lc_present','LX.fail')
 u64(E,'LC.makeaddr',b'\0library/callablemake','lc_make','lc_makepresent','LX.fail')
 u64(E,'LC.calladdr',b'\0library/callablecall','lc_call','lc_callpresent','LX.fail')
 section('s1')
 # Caller has proved MG.compatible for this final fixed normalized binding.
 # This helper touches only lc_* scratch and preserves every LI emission reg.
 hook('FNV.y','LC.fnvalue')
 section('s2')
 # Override the continuation immediately after FS.CALLTYPE, preserving parser stack.
 continuations={a[1] for name,(mode,row) in g.st.items() for nx,q in row.values() if nx=='FS.CALLTYPE' for a in g.seqs[q] if a[0]=='PUSH'}
 assert len(continuations)==1,continuations
 name=continuations.pop();move(name,'LC.original.fpclassified')
 section('s3',fpcont=name)
 hook('CL.ok','LC.named')
 section('s4')
 # The old CL.a2 hook already captures named variadics. Capture our separate
 # indirect site after the existing conversions and before incrementing na.
 hook('CL.a2','LC.argument')
 section('s5')
 # CL.default already emitted TO.d; only the saved descriptor needs promotion.
 hook('CL.done','LC.complete')
 section('s6')
 hook('CL.vindirect','LC.indirect')
 section('s7')
 # Aggregate return expressions may contain a typed indirect call.
 hook('S.rs','LC.structreturn')
 section('s8')
 # Force the final wrapper pass even when no native imports were registered.
 move('UD.calls0','LC.original.calls0')
 section('s9')
 alias('UD.calls0','LC.calls0select')
 section('s10')
 # Emit source/native introductions only after final source-priority facts exist.
 move('LI.wrapend','LC.original.wrapend')
 section('s11')
 alias('LI.wrapend','LC.wrapendselect')
 section('s12')
 move('LX.outer','LC.original.outer')
 section('s13')
 alias('LX.outer','LC.outerselect')
 section('s14')
 return 'LC.start'
