"""Retain ordered parser layout facts without changing SMEM/tape/USLSIG2.
These are actual parser projections, not complete modifier/packing declarations.
Enabled by explicit u64 \\0library/layoutfacts=1 or library symbols presence.
Entry key = sid*128+ordinal; count/union/flatten/seen are keyed only by sid.
"""
import sys
COUNT,KIND,BASE,DEPTH,ARRAY,OFFSET,BITOFFSET,BITWIDTH,STORAGE,ALIGN,CHILD,MEMBER,SHAPE,SIGNED,UNION,FLATTEN,SEEN=(i<<40 for i in range(600,617))
RESERVED_BANKS=tuple(i<<40 for i in range(600,620))
ENTRY_STRIDE=128
RECORD_LIMIT=128
ENTRY_LIMIT=128
ORDINARY,NAMED_BITFIELD,ANONYMOUS_BITFIELD,ZERO_WIDTH_BARRIER,ANONYMOUS_AGGREGATE=range(5)
ENTRY_FIELDS=(('kind',KIND),('base',BASE),('depth',DEPTH),('array',ARRAY),('offset',OFFSET),('bitoffset',BITOFFSET),('bitwidth',BITWIDTH),('storage',STORAGE),('align',ALIGN),('child',CHILD),('member',MEMBER),('shape',SHAPE),('signed',SIGNED))
def entry_key(sid,ordinal):
 assert 1<=sid<=RECORD_LIMIT and 0<=ordinal<ENTRY_LIMIT
 return sid*ENTRY_STRIDE+ordinal

def install(E,P,b,start):
 from modelinput import u64
 g=E.g
 assert not set(RESERVED_BANKS).intersection(v for m in tuple(sys.modules.values()) if m is not None and m.__name__!=__name__ for v in vars(m).values() if type(v) is int and v>=1<<40)
 u64(E,'LF.resource',b'\0library/layoutfacts','lf_flag','lf_present','LF.fail')
 P('LF.start').call('LF.resource').branch({(0,1):start},'LF.fail',[('RLD','lf_flag')])
 P('LF.fail').a(E.rej('not covered: ordered source layout facts limit or resource')).goto('DEAD')
 def hook(state,proc):
  orig='LF.original.'+state;assert orig not in g.st
  g.st[orig]=g.st.pop(state);g.labels.add(orig)
  # Existing state actions/observations are executed after fact capture unchanged.
  alias='LF.entry.'+state
  P(alias).branch({1:'LF.enabled.'+state},'LF.library.'+state,[('CMPI','lf_flag',1)])
  P('LF.library.'+state).branch({1:orig},'LF.enabled.'+state,[('CMPI','lx_present',0)])
  P('LF.enabled.'+state).call(proc).goto(orig)
  g.st[state]=g.st[alias]
 def load(p,register,pool):return p.a(('LDX','lf_'+register,'k',b[pool]))
 def zero(p):
  for name,_ in ENTRY_FIELDS:p.a(('LDI','lf_'+name,0))
  return p
 P('LF.sid').a(('COPYW','lf_sid','sid')).branch({(0,1):'LF.fail'},'LF.sidbound',[('CMPI','lf_sid',0)])
 P('LF.sidbound').branch({2:'LF.fail'},'RET',[('CMPI','lf_sid',RECORD_LIMIT)])
 P('LF.reset').call('LF.sid').a(('LDI','lf_zero',0),('LDI','lf_one',1),('STX','lf_sid',COUNT,'lf_zero'),('STX','lf_sid',FLATTEN,'lf_zero'),('STX','lf_sid',SEEN,'lf_one'),('STX','lf_sid',UNION,'sun_n')).ret()
 P('LF.append').call('LF.sid').a(('LDX','lf_n','lf_sid',COUNT)).branch({0:'LF.write'},'LF.fail',[('CMPI','lf_n',ENTRY_LIMIT)])
 p=P('LF.write').a(('ALUI','mul','lf_key','lf_sid',ENTRY_STRIDE),('ALU','add','lf_key','lf_key','lf_n'))
 for name,bank in ENTRY_FIELDS:p.a(('STX','lf_key',bank,'lf_'+name))
 p.a(('ALUI','add','lf_n','lf_n',1),('STX','lf_sid',COUNT,'lf_n')).ret()
 P('LF.named').a(('LDX','lf_suppressed','sid',FLATTEN)).branch({1:'LF.namedread'},'RET',[('CMPI','lf_suppressed',0)])
 p=zero(P('LF.namedread'))
 for name,pool in (('base','MBS'),('depth','MPT'),('array','MAR'),('offset','MOF'),('bitwidth','BFW'),('storage','MSZ')):load(p,name,pool)
 p.a(('COPYW','lf_member','k'),('COPYW','lf_align','mal'),('A64I','add','lf_shapekey','k',b['SHAPE_IDS']),('LDX','lf_shape','lf_shapekey',b['SHAPE'])).branch({1:'LF.namedchild'},'LF.namedbits',[('CMPI','lf_bitwidth',0)])
 P('LF.namedbits').a(('LDI','lf_kind',NAMED_BITFIELD),('LDX','lf_bitoffset','k',b['BFO']),('LDX','lf_signed','k',b['BFS'])).goto('LF.namedchild')
 P('LF.namedchild').branch({1:'LF.namedbase'},'LF.namedappend',[('CMPI','lf_depth',0)])
 P('LF.namedbase').branch({(1,2):'LF.namedrecord'},'LF.namedappend',[('CMPI','lf_base',b['SBB'])])
 P('LF.namedrecord').a(('ALUI','sub','lf_child','lf_base',b['SBB'])).goto('LF.namedappend')
 P('LF.namedappend').call('LF.append').ret()
 p=zero(P('LF.anonymousbit')).a(('COPYW','lf_base','tb'),('COPYW','lf_depth','mtd'),('COPYW','lf_bitwidth','bf_width'),('COPYW','lf_storage','bf_unit'),('COPYW','lf_align','mal'),('COPYW','lf_signed','bf_signed'),('ALUI','mul','lf_bits','bf_unit',8),('ALU','div','lf_offset','bf_pos','lf_bits'),('ALU','mul','lf_offset','lf_offset','bf_unit'),('ALU','rem','lf_bitoffset','bf_pos','lf_bits')).branch({1:'LF.barrier'},'LF.anonymouspositive',[('CMPI','bf_width',0)])
 P('LF.barrier').a(('LDI','lf_kind',ZERO_WIDTH_BARRIER)).call('LF.append').ret()
 P('LF.anonymouspositive').a(('LDI','lf_kind',ANONYMOUS_BITFIELD)).call('LF.append').ret()
 p=zero(P('LF.anonymousrecord')).a(('LDI','lf_kind',ANONYMOUS_AGGREGATE),('ALUI','add','lf_base','an_sid',b['SBB']),('COPYW','lf_offset','an_base'),('COPYW','lf_storage','msz'),('COPYW','lf_align','mal'),('COPYW','lf_child','an_sid')).call('LF.append').a(('LDI','lf_one',1),('STX','sid',FLATTEN,'lf_one')).ret()
 P('LF.anonymousend').a(('LDI','lf_zero',0),('STX','sid',FLATTEN,'lf_zero')).ret()
 for state,proc in (('SB.go','LF.reset'),('SB.memberput','LF.named'),('BF.padding','LF.anonymousbit'),('SB.anonbegin','LF.anonymousrecord'),('SB.anonend','LF.anonymousend')):hook(state,proc)
 return 'LF.start'
