"""Windows x86 setup, regenerated after relaxation so RIP offsets are final.
Only WINARGS_BODY is a declared byte template; encoding rules are transitions.
"""
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

def init(p):
 for key in OPS+RCS:
  p.a(('SBCLR',),[('SBOUT',c) for c in key.encode()],('SBINTERN','wi_'+key))
 for i,name in enumerate(IMPORTS):
  p.a(('SBCLR',),[('SBOUT',c) for c in name.encode()],('SBINTERN','t'),('LDI','u',i+1),('STX','t',IMP,'u'))

def reset(p):
 p.a(('LDI','gwin',0))
 for k in META:p.a(('LDI','wm_'+k,0))
 return p
BASES={name:(16+i)<<40 for i,name in enumerate(FIELDS)}


def install(E,byte,KND,SZ):
 P,g=E.P,E.g
 p=P('WX.store').branch({1:'WX.arity'},'DEAD.wx',[('CMPI','target_os',3)])
 P('WX.arity').branch({**{v:'WX.arity.'+k for k,v in IDS.items()},24:'WX.arity.gate'},'DEAD.wx',[('RLD','cls')])
 for name,n in (('winsave',1),('winrest',2),('winstdh',1),('winargs',3),('gate',0)):
  P('WX.arity.'+name).branch({1:'WX.save'},'DEAD.wx',[('CMPI','na',n)])
 p=P('WX.save')
 for name in FIELDS:p.a(('STX','npc',BASES[name],name),('COPYW','wx_'+name,name))
 # Emit once only to measure fixed size; placeholder displacements are discarded.
 p.call('WX.emit').a(('OCUT','wx_blob','omark'),('BLEN','t','wx_blob'),('STX','npc',SZ,'t'),('LDI','t',7),('STX','npc',KND,'t'),('ALUI','add','npc','npc',1)).goto('SKIPL')
 p=P('WR.win')
 for name in FIELDS:p.a(('LDX','wx_'+name,'q',BASES[name]))
 p.a(('OLEN','wx_start')).call('WX.emit').a(('OLEN','t'),('ALU','sub','t','t','wx_start'),('LDX','u','q',SZ)).branch({1:'WR.nx'},'DEAD.wx',[('CMP','t','u')])
 P('WX.emit').branch({**{v:'WX.'+k for k,v in IDS.items()},24:'WX.gate'},'DEAD.wx',[('RLD','wx_cls')])
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
 def call(p,name=None):
  if name is None:p.a(('A64I','mul','ad_v','wx_imp',8),('A64','add','ad_v','ad_v','imp_base'))
  else:p.a(('A64I','add','ad_v','imp_base',8*IMPORTS.index(name)))
  p.a(('LDI','ad_r',0),('LDI','ad_o',0x8b)).call('RIP')
  return raw(p,0xff,0xd0)
 def pre(p,extra=0):return raw(mov(p,3,4),0x48,0x83,0xe4,0xf0,0x48,0x83,0xec,32+((extra*8+15)//16)*16)
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
 # Gate metadata persists in the deferred per-instruction record.
 for key in META[:3]:
  P('WX.meta.'+key).a(('INTERN','wm_'+key,'vs','ve')).goto('META.ok')
 for key in META[3:]:
  P('WX.meta.'+key).a(('JUMP','vs'),('LDI','wn',0)).goto('WX.first.'+key)
  g.on('WX.first.'+key,range(48,58),'WX.num.'+key,[]);g.els('WX.first.'+key,'DEAD.wx',[])
  g.on('WX.num.'+key,range(48,58),'WX.bound.'+key,[('BYTE','t'),('ALUI','sub','t','t',48),('A64I','mul','wn','wn',10),('A64','add','wn','wn','t')])
  P('WX.bound.'+key).a(('LDI','limit',2147483647)).branch({2:'DEAD.wx'},'WX.adv.'+key,[('C64U','wn','limit')])
  P('WX.adv.'+key).a(('ADV',)).goto('WX.num.'+key)
  g.on('WX.num.'+key,[32,10,256],'META.ok',[('COPYW','wm_'+key,'wn')]);g.els('WX.num.'+key,'DEAD.wx',[])
 def imm(p,r,v):
  if 0<=v<2**32:
   if r>=8:raw(p,0x41)
   raw(p,0xb8+(r&7));n=4
  else:raw(p,0x48|(r>>3),0xb8+(r&7));n=8
  p.a(('LDI','lb_v',v),('LDI','lb_n',n)).call('LEBYTES')
  return p
 def stack(p,slot,v):
  return raw(p,0x48,0xc7,0x44,0x24,slot).a(('LDI','lb_v',v),('LDI','lb_n',4)).call('LEBYTES')
 P('WX.gate').a(('LDX','wx_imp','wx_wm_winimp',IMP)).branch({1:'DEAD.wx'},'WX.ops',[('CMPI','wx_imp',0)])
 p=P('WX.ops').a(('ALUI','sub','wx_imp','wx_imp',1))
 for op in OPS:
  p.branch({1:'WX.op.'+op},'WX.next.'+op,[('CMP','wx_wm_catop','wi_'+op)]);p=P('WX.next.'+op)
 p.goto('DEAD.wx')
 p=raw(P('WX.fd'),0x48,0x83,0xf9,3,0x73,11);rip(p,11,'wx_wm_hstd');raw(p,0x49,0x8b,0x0c,0xcb).ret()
 for op in ('exit','mmap','unlink'):
  post(call(pre(P('WX.op.'+op)))).goto('WX.tail')
 for op in ('read','write'):
  p=P('WX.op.'+op).call('WX.fd');rip(p,9,'wx_wm_written');pre(p,1);stack(p,32,0);post(call(p)).goto('WX.tail')
 post(call(pre(P('WX.op.close').call('WX.fd')))).goto('WX.tail')
 p=P('WX.op.mprotect');rip(p,9,'wx_wm_written');post(call(pre(p)));imm(p,1,-1)
 rip(p,2,'wx_wm_scr0',0x8b);rip(p,8,'wx_wm_scr1',0x8b);post(call(pre(p),'FlushInstructionCache')).goto('WX.tail')
 p=P('WX.op.munmap');imm(p,8,0x8000);imm(p,2,0);post(call(pre(p))).goto('WX.tail')
 p=pre(mov(P('WX.op.open'),0,8),3);raw(p,0x48,0x89,0x44,0x24,32);imm(p,8,3);imm(p,9,0)
 stack(p,40,0x80);stack(p,48,0);post(call(p)).goto('WX.tail')
 p=mov(P('WX.op.lseek'),9,8);imm(p,8,0);p.call('WX.fd');post(call(pre(p))).goto('WX.tail')
 p=imm(P('WX.op.rename'),8,1);post(call(pre(p))).goto('WX.tail')
 p=P('WX.tail').branch({1:'RET'},'WX.rcs',[('CMPI','wx_wm_retconv',0)])
 p=P('WX.rcs')
 for rc in RCS:
  p.branch({1:'WX.rc.'+rc},'WX.rcnext.'+rc,[('CMP','wx_wm_retconv','wi_'+rc)]);p=P('WX.rcnext.'+rc)
 p.goto('DEAD.wx')
 P('WX.rc.none').ret();P("WX.rc.'none").ret()
 rip(P('WX.rc.wcount'),0,'wx_wm_written',0x8b).ret()
 raw(P('WX.rc.bool_inv'),0x48,0x83,0xf8,0,0x0f,0x94,0xc0,0x48,0x0f,0xb6,0xc0).ret()
 raw(P('WX.rc.bool_neg'),0x85,0xc0,0x0f,0x94,0xc0,0x48,0x0f,0xb6,0xc0,0x48,0xf7,0xd8).ret()
 raw(P('WX.rc.dword_sx'),0x48,0x63,0xc0).ret()
 g.on('DEAD.wx',range(257),'DEAD',E.rej('not covered: Windows x86 setup contract'),'r')
