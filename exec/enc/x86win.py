"""Windows x86 setup, regenerated after relaxation so RIP offsets are final.
Only WINARGS_BODY is a declared byte template; encoding rules are transitions.
"""
from unisa.catalog import REGMAP
from unisa.emit_x86 import NUM, WINARGS_BODY
from unisa.image.pe import IMPORTS
IDS={name:200+i for i,name in enumerate(('winsave','winrest','winstdh','winargs'))}
# Per-instruction records, isolated from byte-addressed image storage.
FIELDS=('cls','a0','a1','a2')
BASES={name:(16+i)<<40 for i,name in enumerate(FIELDS)}


def install(E,byte,KND,SZ):
 P,g=E.P,E.g
 p=P('WX.store').branch({1:'WX.arity'},'DEAD.wx',[('CMPI','target_os',3)])
 P('WX.arity').branch({v:'WX.arity.'+k for k,v in IDS.items()},'DEAD.wx',[('RLD','cls')])
 for name,n in (('winsave',1),('winrest',2),('winstdh',1),('winargs',3)):
  P('WX.arity.'+name).branch({1:'WX.save'},'DEAD.wx',[('CMPI','na',n)])
 p=P('WX.save')
 for name in FIELDS:p.a(('STX','npc',BASES[name],name),('COPYW','wx_'+name,name))
 # Emit once only to measure fixed size; placeholder displacements are discarded.
 p.call('WX.emit').a(('OCUT','wx_blob','omark'),('BLEN','t','wx_blob'),('STX','npc',SZ,'t'),('LDI','t',7),('STX','npc',KND,'t'),('ALUI','add','npc','npc',1)).goto('SKIPL')
 p=P('WR.win')
 for name in FIELDS:p.a(('LDX','wx_'+name,'q',BASES[name]))
 p.a(('OLEN','wx_start')).call('WX.emit').a(('OLEN','t'),('ALU','sub','t','t','wx_start'),('LDX','u','q',SZ)).branch({1:'WR.nx'},'DEAD.wx',[('CMP','t','u')])
 P('WX.emit').branch({v:'WX.'+k for k,v in IDS.items()},'DEAD.wx',[('RLD','wx_cls')])
 def put(p,r,v):return p.a(('LDI' if isinstance(v,int) else 'COPYW',r,v))
 def raw(p,*bs):
  for b in bs:byte(p,b)
  return p
 def mov(p,d,s):
  for r,v in (('al_o',0x89),('al_d',d),('al_s',s)):put(p,r,v)
  return p.call('ALU')
 def rip(p,r,arg,op=0x8d):
  put(p,'ad_r',r);put(p,'ad_v',arg);put(p,'ad_o',op)
  return p.a(('A64','add','ad_v','ad_v','data_shift')).call('RIP')
 def mem(p,op,r,b,d):
  for k,v in (('me_o1',op),('me_two',0),('me_w',1),('me_66',0),('me_r',r),('me_b',b),('me_d',d)):put(p,k,v)
  return p.call('MEM')
 def call(p,name):
  p.a(('LDI','ad_r',0),('LDI','ad_o',0x8b),('A64I','add','ad_v','imp_base',8*IMPORTS.index(name))).call('RIP')
  return raw(p,0xff,0xd0)
 def pre(p):return raw(mov(p,3,4),0x48,0x83,0xe4,0xf0,0x48,0x83,0xec,32)
 def post(p):return mov(p,4,3)
 p=rip(P('WX.winsave'),11,'wx_a0')
 for k,r in enumerate(REGMAP['x86_64']):mem(p,0x89,NUM[r],11,8*k)
 p.ret()
 P('WX.winrest').branch({1:'WX.restore'},'DEAD.wx',[('CMPI','wx_a1',0)])
 p=rip(mov(P('WX.restore'),11,0),3,'wx_a0')
 for k,r in enumerate(REGMAP['x86_64']):
  if k:mem(p,0x8b,NUM[r],3,8*k)
 mov(p,0,11).ret()
 p=P('WX.winstdh')
 for k in range(3):
  raw(p,0x48,0xb9).a(('LDI','lb_v',-10-k),('LDI','lb_n',8)).call('LEBYTES')
  post(call(pre(p),'GetStdHandle'));rip(p,11,'wx_a0');mem(p,0x89,0,11,8*k)
 p.ret()
 p=post(call(pre(P('WX.winargs')),'GetCommandLineA'))
 raw(p,0x48,0x89,0xc6,0x48,0x89,0xc7,0x31,0xc9);rip(p,8,'wx_a2');raw(p,*WINARGS_BODY)
 rip(p,1,'wx_a0',0x89);rip(p,8,'wx_a1',0x89);p.ret()
 g.on('DEAD.wx',range(257),'DEAD',E.rej('not covered: Windows x86 setup contract'),'r')
