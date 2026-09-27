"""Windows x86 setup, regenerated after relaxation so RIP offsets are final.
Shared x86win declarations encode control; ABI/import/template facts stay bound.
"""
import json
from pathlib import Path
from unisa.catalog import REGMAP
from unisa.emit_x86 import NUM, WINARGS_BODY
from unisa.image.pe import IMPORTS
IDS={name:200+i for i,name in enumerate(('winsave','winrest','winstdh','winargs'))}
# Per-instruction records, isolated from byte-addressed image storage.
META=('catop','winimp','retconv','hstd','written','scr0','scr1')
OPS=('exit','read','write','mmap','mprotect','munmap','close','open','lseek','unlink','rename')
RCS=('none',"'none",'wcount','bool_inv','bool_neg','dword_sx')
FIELDS=('cls','a0','a1','a2')+tuple('wm_'+k for k in META)
IMP=60<<40
SEQUENCES={name:[tuple(a) for a in json.loads(actions)] for name,actions in
           (line.split('\t') for line in Path(__file__).with_name('x86win-sequences.tsv').read_text().splitlines() if not line.startswith('#'))}

def init(p):
 for key in OPS+RCS:
  p.a(('SBCLR',),[('SBOUT',c) for c in key.encode()],('SBINTERN','wi_'+key))
 for i,name in enumerate(IMPORTS):
  p.a(('SBCLR',),[('SBOUT',c) for c in name.encode()],('SBINTERN','t'),('LDI','u',i+1),('STX','t',IMP,'u'))

def reset(p):
 p.a(SEQUENCES['reset'])
 for k in META:p.a(('LDI','wm_'+k,0))
 return p
BASES={name:(16+i)<<40 for i,name in enumerate(FIELDS)}


def install(E,byte,KND,SZ):
 from finite_rules import install as install_rules
 root=Path(__file__).parent
 names=[line.split('\t') for line in (root/'x86win-names.tsv').read_text().splitlines() if not line.startswith('#')]
 sequences={'byte'+value:byte(E.P('byte.binding'),int(value)).acts for value in
            (root/'x86win-bytes.tsv').read_text().splitlines() if not value.startswith('#')}
 sequences.update(SEQUENCES)
 sequences['save_fields']=[a for name in FIELDS for a in [('STX','npc',BASES[name],name),('COPYW','wx_'+name,name)]]
 sequences['restore_fields']=[('LDX','wx_'+name,'q',BASES[name]) for name in FIELDS]
 template=E.P('winargs.binding')
 for value in WINARGS_BODY:byte(template,value)
 sequences.update(winargs=template.acts,reject=E.rej('not covered: Windows x86 setup contract'))
 facts=dict(KND=KND,SZ=SZ,IMP=IMP)
 facts.update({'import_'+name:8*IMPORTS.index(name) for name in ('GetStdHandle','GetCommandLineA','FlushInstructionCache')})
 def rules(section,entry=None,prepare='empty',**values):
  bindings={**facts,**values,'entry':entry}
  local={name:E.P(owner).fresh(kind) for selected,name,owner,kind in names if selected==section}
  bindings.update(local)
  install_rules(E.g,root,'x86win',section=section,bindings=bindings,
                sequences={**sequences,'prepare':sequences[prepare]},classes={'id_'+k:(v,) for k,v in IDS.items()})
  return next(reversed(local.values())) if local else entry
 rules('store')
 for name,count in (('winsave',1),('winrest',2),('winstdh',1),('winargs',3),('gate',0)):
  rules('arity','WX.arity.'+name,count=count)
 rules('record')
 entry=rules('save-head')
 for k,r in enumerate(REGMAP['x86_64']):entry=rules('save-register',entry,register=NUM[r],offset=8*k)
 rules('return',entry)
 entry=rules('restore-head')
 for k,r in enumerate(REGMAP['x86_64']):
  if k:entry=rules('restore-register',entry,register=NUM[r],offset=8*k)
 rules('restore-end',entry)
 entry='WX.winstdh'
 for k in range(3):entry=rules('stdh',entry,handle=-10-k,offset=8*k)
 rules('return',entry)
 rules('winargs')
 for key in META[:3]:rules('meta-name','WX.meta.'+key,field='wm_'+key)
 for key in META[3:]:
  rules('meta-number','WX.meta.'+key,first='WX.first.'+key,num='WX.num.'+key,
        bound='WX.bound.'+key,advance='WX.adv.'+key,field='wm_'+key)
 rules('gate-head')
 entry='WX.ops'
 for i,op in enumerate(OPS):
  nxt='WX.next.'+op
  rules('gate-select',entry,key='wi_'+op,yes='WX.op.'+op,next=nxt,
        prepare='first-op' if i==0 else 'empty')
  entry=nxt
 rules('gate-end',entry)
 rules('fd')
 for op in ('exit','mmap','unlink'):rules('simple-api','WX.op.'+op)
 for op in ('read','write'):rules('io-api','WX.op.'+op)
 rules('other-api')
 rules('retconv-head')
 entry='WX.rcs'
 for rc in RCS:
  nxt='WX.rcnext.'+rc
  rules('retconv-select',entry,key='wi_'+rc,yes='WX.rc.'+rc,next=nxt)
  entry=nxt
 rules('retconv-end',entry)
 rules('retconv-bodies')
