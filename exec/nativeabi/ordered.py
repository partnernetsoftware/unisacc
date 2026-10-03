"""Ordered complete SIG3 facts -> finite natural <=16B V2 carriers.
No source allocator or host ABI classifier. Actual bit intervals stay separate
from container extent. Anonymous SysV lanes qualify only if both known unnamed
bitfield policies agree. BANK, modifiers and incomplete facts reject.
"""
import json,pathlib
from finite_rules import install as install_rules
from modelgraphequality import EXTRA,ENTRYEXTRA,EDGES,LAYOUT
# Additional values saved by NL's existing isolated recursive frame.
REGS=('nl_bitoffset','nl_bitwidth','nl_entrykind','nl_entryalign','nl_cursor',
      'nl_accblo','nl_accbhi','nl_accanon','nl_acccount','nl_hcountchild')
# ol_packed is deliberately NOT frame-saved: one packed node anywhere marks the whole object.
def install(E,field,word,constword,blob,extents,alignments):
 """Stage control lives in ordered-result.tsv (section main: metadata, interval, aggregate
 walk, family/recipe checks; section recipe: the emission loop). Python binds only dynamic
 facts: graph constants, helper-built field reads, rules.tsv extents/alignments, fresh labels;
 the recipe header/element writers stay here because they call the word/constword helpers."""
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
 p=P('OL.recipeheader')
 for v in (0,0,0,5):constword(p,v)
 word(p,'nc_width');constword(p,0);word(p,'nc_alignment')
 p.a(('LDI','nc_v',1),('OUTW','nc_v'),('OLEN','nc_payloadcut'),('ALU','div','ol_emitcount','nc_width','nc_alignment'),('LDI','ol_emitindex',0),('LDI','ol_emitoffset',0));word(p,'ol_emitcount').goto('OL.recipeloop')
 section('recipe')
 p=P('OL.recipewrite');word(p,'ol_emitoffset');constword(p,0);constword(p,0);word(p,'nc_alignment')
 for v in (0,0,0):constword(p,v)
 word(p,'ol_emitkind');word(p,'nc_alignment');word(p,'ol_emitunsigned');word(p,'nc_alignment')
 p.a(('OUTW','nc_zero'));constword(p,0).a(('ALUI','add','ol_emitindex','ol_emitindex',1),('ALU','add','ol_emitoffset','ol_emitoffset','nc_alignment')).goto('OL.recipeloop')
