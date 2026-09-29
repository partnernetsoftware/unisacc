"""Complete declaration-owned callback graph serializer in ordinary delta.
Record-local first traversal IDs are registered before child descriptors.
Only signature declaration facts are consumed; no callable bridge is claimed.
"""
SIGWIREID,SIGWIREMARK,SIGFRAME=(i<<40 for i in range(420,423))
assert not set(range(420,423)) & (set(range(72,83))|set(range(300,358))|set(range(400,414))|set(range(430,438)))
def install(E,P,b,frame):
 from libraryexports import VARIADIC,PARAMMARK,PARAMDEPTH,PARAMBASE,PARAMSHAPE,SIGEPOCH
 regs=('lcg_sig','lcg_count','lcg_epoch','lcg_wireid','lcg_i','lcg_parent_support','lcg_var','lcg_mode')
 def sigframe(p,op):
  for i,r in enumerate(regs):
   p.a(('ALUI','mul','lcg_slot','lx_recursion',16),('ALUI','add','lcg_slot','lcg_slot',i))
   p.a((op,'lcg_slot',SIGFRAME,r) if op=='STX' else (op,r,'lcg_slot',SIGFRAME))
  return p
 def child(p):
  frame(p,'STX');sigframe(p,'STX');p.call('LX.descriptor');sigframe(p,'LDX');return frame(p,'LDX')
 def word(p,r):return p.a(('COPYW','lx_v',r)).call('LX.u64')
 P('LCG.begin').a(('ALUI','add','lcg_generation','lcg_generation',1),('LDI','lcg_definitions',0)).ret()
 P('LCG.payload').a(('COPYW','lcg_sig','d_base'),('LDX','lcg_mark','d_base',SIGWIREMARK),('LDI','lcg_byte',1),('OUTW','lcg_byte')).branch({1:'LCG.reference'},'LCG.define',[('CMP','lcg_mark','lcg_generation')])
 p=P('LCG.reference').a(('LDI','lcg_byte',1),('OUTW','lcg_byte'),('LDX','lcg_wireid','lcg_sig',SIGWIREID));word(p,'lcg_wireid').goto('LTY.out')
 P('LCG.define').a(('ALUI','add','lcg_definitions','lcg_definitions',1)).branch({2:'LX.fail'},'LCG.register',[('CMPI','lcg_definitions',1024)])
 p=P('LCG.register').a(('COPYW','lcg_wireid','lcg_definitions'),('STX','lcg_sig',SIGWIREID,'lcg_wireid'),('STX','lcg_sig',SIGWIREMARK,'lcg_generation'),('LDI','lcg_byte',0),('OUTW','lcg_byte'))
 word(p,'lcg_wireid').a(('LDX','lcg_count','lcg_sig',b['FPS_COUNT']),('LDX','lcg_epoch','lcg_sig',SIGEPOCH),('LDX','lcg_var','lcg_sig',VARIADIC),('LDX','lcg_mode','lcg_sig',b['FPS_VAR']),('COPYW','lcg_parent_support','lx_supported'),('LDI','lx_supported',1)).call('LX.support').branch({2:'LX.fail'},'LCG.var',[('CMPI','lcg_count',1024)])
 P('LCG.var').branch({1:'LCG.unsupportedvar'},'LCG.fields',[('CMPI','lcg_var',1)])
 P('LCG.unsupportedvar').a(('LDI','lx_supported',0)).goto('LCG.fields')
 p=P('LCG.fields').a(('OUTW','lcg_var'),('OUTW','lcg_mode'));word(p,'lcg_count')
 p.a(('LDX','lx_depth','lcg_sig',b['FPS_RD']),('LDX','lx_base','lcg_sig',b['FPS_RB']),('LDX','lx_shape','lcg_sig',b['FPS_RSH']),('LDI','lx_array',0),('LDI','lx_arraybytes',0),('LDI','lx_arraydimension',0),('LDI','lx_return',1));child(p);word(p,'lcg_count').a(('LDI','lcg_i',0)).goto('LCG.params')
 P('LCG.params').branch({1:'LCG.done'},'LCG.param',[('CMP','lcg_i','lcg_count')])
 P('LCG.param').a(('ALUI','mul','lcg_key','lcg_sig',1024),('ALU','add','lcg_key','lcg_key','lcg_i'),('LDX','lcg_mark','lcg_key',PARAMMARK)).branch({1:'LCG.paramtype'},'LX.fail',[('CMP','lcg_mark','lcg_epoch')])
 p=P('LCG.paramtype').a(('LDX','lx_depth','lcg_key',PARAMDEPTH),('LDX','lx_base','lcg_key',PARAMBASE),('LDX','lx_shape','lcg_key',PARAMSHAPE),('LDI','lx_array',0),('LDI','lx_arraybytes',0),('LDI','lx_arraydimension',0),('LDI','lx_return',0));child(p).a(('ALUI','add','lcg_i','lcg_i',1)).goto('LCG.params')
 P('LCG.done').a(('OUTW','lx_supported'),('COPYW','lx_supported','lcg_parent_support')).goto('LTY.out')
