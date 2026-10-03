"""Ordered complete SIG3 facts -> finite natural <=16B V2 carriers.
No source allocator or host ABI classifier. Actual bit intervals stay separate
from container extent. Anonymous SysV lanes qualify only if both known unnamed
bitfield policies agree. BANK, modifiers and incomplete facts reject.
"""
import json,pathlib
from finite_rules import install as install_rules,install_template
# Additional values saved by NL's existing isolated recursive frame.
from facts.load import facts
_G={r['name']:r['value'] for r in facts('top-modelgraphequality-banks')}  # exec/facts/top-modelgraphequality-banks.tsv
EXTRA,ENTRYEXTRA,EDGES,LAYOUT=_G['EXTRA'],_G['ENTRYEXTRA'],_G['EDGES'],_G['LAYOUT']
REGS=tuple(facts('nativeabi-ordered-regs'))
# ol_packed is deliberately NOT frame-saved: one packed node anywhere marks the whole object.
def install(E,field,word,constword,blob,extents,alignments):
 """Stage control lives in ordered-result.tsv (section main: metadata, interval, aggregate
 walk, family/recipe checks; section recipe: the emission loop). Python binds only dynamic
 facts: graph constants, helper-built field reads, rules.tsv extents/alignments, fresh labels.
 The recipe header/element writers are MS.write64 call chains in ordered-template.tsv."""
 P=E.P;root=pathlib.Path(__file__).parent
 fresh=[l.split('\t') for l in (root/'ordered-fresh.tsv').read_text().splitlines()[1:]]
 bindings=dict(EXTRA=EXTRA,ENTRYEXTRA=ENTRYEXTRA,EDGES=EDGES,LAYOUT=LAYOUT)
 classes=dict(extents=list(extents),alignments=list(alignments))
 sequences={}
 for line in (root/'ordered-result.tsv').read_text().splitlines()[1:]:
  for action in json.loads(line.split('\t')[4]):
   if action[0]=='@' and action[1] not in sequences:
    kind,index,reg,node=action[1].split()
    if kind=='field':sequences[action[1]]=field(P('OL.bindings'),int(index),reg,node).acts
    else:sequences[action[1]]=[('ALUI','mul','nc_key',node,8),('ALUI','add','nc_key','nc_key',int(index)),('LDX',reg,'nc_key',EXTRA)]
 def section(name):
  p=P('OL.fresh')
  for part,key,kind in fresh:
   if part==name:bindings[key]=p.fresh(kind)
  install_rules(E.g,root,'ordered',bindings,sequences,classes,name)
 section('main')
 install_template(E.g,root,'ordered',{},P('OL.recipeheader').fresh,section='header')
 section('recipe')
 install_template(E.g,root,'ordered',{},P('OL.recipewrite').fresh,section='write')
