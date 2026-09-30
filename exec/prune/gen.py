"""Conservative original-span prune delta. Generic executor actions only.
Input domain and capacities match unisa.prune; no compiler routing is installed.
Build: python3 exec/prune/gen.py PRIVATE.json
"""
import importlib.util, json, sys
from pathlib import Path
sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT)); sys.path.insert(0, str(ROOT / 'exec'))
from unisa.tape import SHAPE, REGS
from finite_rules import load
spec = importlib.util.spec_from_file_location('prune_assembler', ROOT/'exec/parse/gen.py')
E = importlib.util.module_from_spec(spec); spec.loader.exec_module(E)
g, P = E.g, E.P
SRCMAX, ROWMAX, NAMEMAX, EDGEMAX = 2097152, 32768, 8192, 65536
# Disjoint sparse word regions: rows, argument facts, names, units, edges.
B = {n:(i+1)*1000000 for i,n in enumerate('RS RE RK RP RO RN OWNER AV AN NUM LAB DATA FIRST START US UE ALIVE ROOTS CALLR HEAD QUEUE TO NEXT NL NS TS TE OP REG SHAPE ARITY'.split())}
OPS = {n:i+1 for i,n in enumerate(SHAPE)}

def a(p,*x): return p.a(*x)
def setv(p,d,v): return p.a(('LDI',d,v))
def cp(p,d,s): return p.a(('COPYW',d,s))
def inc(p,d,v=1): return p.a(('ALUI','add',d,d,v))
def get(p,d,n,i): return p.a(('LDX',d,i,B[n]))
def put(p,n,i,s): return p.a(('STX',i,B[n],s))
def mul(p,d,s,v): return p.a(('ALUI','mul',d,s,v))
def cmp(p,x,y,cases,other,im=False):
 p.branch(cases,other,[('CMPI' if im else 'CMP',x,y)]); return p

def test(p,x,y,yes,no,im=False): return cmp(p,x,y,{1:yes},no,im)
def state(n): return P(n)
def byte(n,cases,other,acts=()):
 done=set()
 for ks,target in cases.items():
  ks=ks if isinstance(ks,tuple) else (ks,); g.on(n,ks,target,acts);done.update(ks)
 g.on(n,[k for k in range(257) if k not in done],other,acts)
def retbad(n): state(n).a(('INPOP',),('LDI','ok',0)).ret()
def nameconst(p,s,d):
 p.a(('SBCLR',),*[('SBOUT',c) for c in s.encode()],('SBINTERN',d));return p

# Reuse the existing bounded LF line skipper unchanged, through finite_rules.
for n,row in load(ROOT/'exec/opt/scans-byte.tsv',{},bindings={'WORDMAX':15,'SIMPLE':32000000}).items():
 if n=='SKIPL':
  for k,(target,acts) in row.items():g.on(n,[k],target,acts,'b')

p=state('START').a(('XLEN','size'),('LDI','zero',0),('LDI','one',1))
cmp(p,'size',SRCMAX,{2:'FALLBACK'},'INIT',True)
p=state('INIT')
for op,oid in OPS.items():
 nameconst(p,op,'id');setv(p,'v',oid);put(p,'OP','id','v');setv(p,'v',len(SHAPE[op]));put(p,'ARITY','id','v')
 for j,k in enumerate(SHAPE[op]):
  setv(p,'idx',oid*8+j);setv(p,'v',ord(k));put(p,'SHAPE','idx','v')
for rid,reg in enumerate(REGS):
 nameconst(p,reg,'id');setv(p,'v',rid+1);put(p,'REG','id','v')
for n in ['_start','main','__init','__main_ret']:nameconst(p,n,'ID'+n)
p.goto('DOMAIN')
byte('DOMAIN',{(13,11,12):'FALLBACK',256:'BEGIN'},'DOMAINADV')
state('DOMAINADV').a(('ADV',)).goto('DOMAIN')
state('BEGIN').a(('JUMP','zero'),('LDI','row',0),('LDI','pc',0),('LDI','names',0)).goto('ROW')
byte('ROW',{256:'PARSED'},'ROWNEW')
p=state('ROWNEW');cmp(p,'row',ROWMAX,{1:'FALLBACK',2:'FALLBACK'},'ROWSET',True)
p=state('ROWSET').a(('MARK','v'));put(p,'RS','row','v');put(p,'RP','row','pc');setv(p,'nt',0);p.goto('SPACE')
byte('SPACE',{(32,9,44,91,93):'SPACEADV',(10,256):'ROWEND',59:'COMMENT',34:'QUOTESTART'},'TOKENSTART')
state('SPACEADV').a(('ADV',)).goto('SPACE')
# Token spans include quotes, so .str can be validated without formatting.
state('QUOTESTART').a(('MARK','tb'),('LDI','isr',0),('ADV',)).goto('QUOTE')
byte('QUOTE',{34:'QUOTEEND',92:'ESCENTER',(10,256):'FALLBACK'},'QUOTEADV')
state('QUOTEADV').a(('ADV',)).goto('QUOTE')
state('ESCENTER').a(('ADV',)).goto('ESC')
byte('ESC',{(10,256):'FALLBACK'},'ESCADV')
state('ESCADV').a(('ADV',)).goto('QUOTE')
state('QUOTEEND').a(('ADV',)).goto('TOKENEND')
state('TOKENSTART').a(('MARK','tb'),('BYTE','c')).goto('TOKENR')
test(state('TOKENR'),'c',114,'TOKR','TOKOTHER',True)
state('TOKR').a(('LDI','isr',1),('ADV',)).goto('TOKEN')
state('TOKOTHER').a(('LDI','isr',0),('ADV',)).goto('TOKEN')
byte('TOKEN',{(32,9,44,91,93,10,256,59):'TOKENEND',(43,45):'OFFS'},'TOKENADV')
state('TOKENADV').a(('ADV',)).goto('TOKEN')
test(state('OFFS'),'isr',1,'TOKENEND','TOKENADV',True)
p=state('TOKENEND').a(('MARK','te'));cmp(p,'nt',16,{1:'FALLBACK',2:'FALLBACK'},'SAVETOKEN',True)
p=state('SAVETOKEN');put(p,'TS','nt','tb');put(p,'TE','nt','te');inc(p,'nt');p.goto('SPACE')
# SKIPL handles comments through LF; marks retain the entire original row.
state('COMMENT').call('SKIPL').a(('MARK','v')).goto('ENDMARK')
byte('ROWEND',{10:'NLADV',256:'EOFMARK'},'FALLBACK')
state('NLADV').a(('ADV',),('MARK','v')).goto('ENDMARK')
state('EOFMARK').a(('MARK','v')).goto('ENDMARK')
p=state('ENDMARK');put(p,'RE','row','v');test(p,'nt',0,'ROWNEXT','CLASS',True)
p=state('CLASS');setv(p,'ti',0);p.call('TOKGET');p.a(('INTERN','id','tb','te'));get(p,'op','OP','id');test(p,'op',0,'LABEL','OPCLASS',True)
p=state('LABEL');test(p,'nt',1,'LABELTOKEN','FALLBACK',True)
p=state('LABELTOKEN').a(('COPYW','v','te'),('ALUI','sub','v','v',1),('INPUSHXE','v','te'),('BYTE','c'),('INPOP',));test(p,'c',58,'LABELNAME','FALLBACK',True)
p=state('LABELNAME').a(('ALUI','sub','te','te',1)).call('NAME');test(p,'ok',1,'LABELSAVE','FALLBACK',True)
p=state('LABELSAVE');get(p,'v','LAB','id');test(p,'v',0,'LABELSTORE','FALLBACK',True)
p=state('LABELSTORE');setv(p,'v',2);put(p,'RK','row','v');put(p,'RN','row','id');cp(p,'v','row');inc(p,'v');put(p,'LAB','id','v');get(p,'v','FIRST','pc');test(p,'v',0,'FIRSTSAVE','ROWNEXT',True)
p=state('FIRSTSAVE');cp(p,'v','row');inc(p,'v');put(p,'FIRST','pc','v');p.goto('ROWNEXT')
p=state('OPCLASS');put(p,'RO','row','op');test(p,'op',OPS.get('.bss',-1),'FALLBACK','DATAOP',True)
# .bss/.str are data directives, absent from SHAPE: handled as first-token IDs.
# Amend CLASS to test their intern IDs before opcode lookup (constructed below).
# Instruction operands use schema declarations, not a duplicated opcode map.
p=state('INSSTART');get(p,'argc','ARITY','id');cp(p,'v','nt');inc(p,'v',-1);test(p,'v','argc','ARGSTART','FALLBACK')
p=state('ARGSTART');setv(p,'ai',0);setv(p,'ti',1);p.goto('ARGLOOP')
p=state('ARGLOOP');test(p,'ai','argc','INSDONE','ARGONE')
p=state('ARGONE');p.call('TOKGET');mul(p,'si','op',8);p.a(('ALU','add','si','si','ai'));get(p,'kind','SHAPE','si');mul(p,'argi','row',8);p.a(('ALU','add','argi','argi','ai'));put(p,'AN','argi','zero');test(p,'kind',ord('r'),'ARGREG','ARGNUM',True)
p=state('ARGREG').a(('INTERN','id','tb','te'));get(p,'v','REG','id');test(p,'v',0,'FALLBACK','ARGREGSAVE',True)
p=state('ARGREGSAVE');inc(p,'v',-1);put(p,'AV','argi','v');p.goto('ARGNEXT')
p=state('ARGNUM').a(('LDI','allowzeros',0)).call('NUMBER');test(p,'ok',1,'ARGNUMSAVE','ARGNAME',True)
p=state('ARGNUMSAVE');put(p,'AV','argi','nv');put(p,'NUM','argi','one');p.goto('ARGNEXT')
p=state('ARGNAME');test(p,'kind',ord('i'),'FALLBACK','ARGNAMECALL',True)
p=state('ARGNAMECALL').call('NAME');test(p,'ok',1,'ARGNAMESAVE','FALLBACK',True)
p=state('ARGNAMESAVE');put(p,'AN','argi','id');p.goto('ARGNEXT')
p=state('ARGNEXT');inc(p,'ai');inc(p,'ti');p.goto('ARGLOOP')
p=state('INSDONE');p.branch({(OPS['.hostcall'],OPS['.hostaddr'],OPS['.librarycall']):'FALLBACK', (OPS['.ld'],OPS['.st']):'WIDTHCHECK'},'INSVALID',[('RLD','op')])
p=state('WIDTHCHECK');mul(p,'argi','row',8);p.a(('ALU','add','argi','argi','argc'),('ALUI','sub','argi','argi',1));get(p,'v','AV','argi');p.branch({(1,2,4,8):'INSVALID'},'FALLBACK',[('RLD','v')])
p=state('INSVALID');setv(p,'v',1);put(p,'RK','row','v');inc(p,'pc');cmp(p,'pc',ROWMAX,{1:'FALLBACK',2:'FALLBACK'},'ROWNEXT',True)
p=state('ROWNEXT');inc(p,'row');p.goto('ROW')
# Token-span accessor; NAME interns only valid names and tracks distinct names.
p=state('TOKGET');get(p,'tb','TS','ti');get(p,'te','TE','ti');p.ret()
state('NAME').a(('INPUSHXE','tb','te'),('LDI','ok',1)).goto('NAMEFIRST')
letters=tuple(list(range(65,91))+list(range(97,123))+[95]); tail=letters+tuple(range(48,58))+(46,36)
byte('NAMEFIRST',{letters:'NAMEADV'},'NAMEBAD')
state('NAMEADV').a(('ADV',)).goto('NAMETAIL')
byte('NAMETAIL',{tail:'NAMEADV',256:'NAMEEND'},'NAMEBAD')
retbad('NAMEBAD')
p=state('NAMEEND').a(('INPOP',),('INTERN','id','tb','te'));get(p,'v','NS','id');test(p,'v',0,'NAMENEW','NAMERET',True)
p=state('NAMENEW');cmp(p,'names',NAMEMAX,{1:'FALLBACK',2:'FALLBACK'},'NAMEADD',True)
p=state('NAMEADD');put(p,'NL','names','id');inc(p,'names');put(p,'NS','id','one');p.goto('NAMERET')
state('NAMERET').ret()
# Signed64 canonical number, checked before multiplication at each digit.
state('NUMBER').a(('INPUSHXE','tb','te'),('LDI','neg',0),('LDI','nv',0),('LDI','digits',0),('LDI','base',10),('LDI','leadzero',0),('LDI','nonzero',0)).goto('SIGN')
byte('SIGN',{45:'NEG',43:'PLUS'},'NUMPREFIX')
state('NEG').a(('LDI','neg',1),('ADV',)).goto('NUMPREFIX')
state('PLUS').a(('ADV',)).goto('NUMPREFIX')
byte('NUMPREFIX',{48:'NUMZERO'},'NUMLIMIT')
state('NUMZERO').a(('LDI','leadzero',1),('LDI','digits',1),('ADV',)).goto('HEXTEST')
byte('HEXTEST',{(120,88):'HEXSTART'},'NUMLIMIT')
state('HEXSTART').a(('LDI','base',16),('LDI','leadzero',0),('LDI','digits',0),('ADV',)).goto('NUMLIMIT')
test(state('NUMLIMIT'),'neg',1,'NEGLIMIT','POSLIMIT',True)
state('NEGLIMIT').a(('LDI','limit',-9223372036854775808)).goto('DIGIT')
state('POSLIMIT').a(('LDI','limit',9223372036854775807)).goto('DIGIT')
byte('DIGIT',{256:'NUMEND',tuple(range(48,58)):'DECDIG',tuple(range(65,71)):'UPDIG',tuple(range(97,103)):'LOWDIG'},'NUMBAD')
state('DECDIG').a(('BYTE','d'),('ALUI','sub','d','d',48)).goto('DIGBOUND')
state('UPDIG').a(('BYTE','d'),('ALUI','sub','d','d',55)).goto('DIGBOUND')
state('LOWDIG').a(('BYTE','d'),('ALUI','sub','d','d',87)).goto('DIGBOUND')
cmp(state('DIGBOUND'),'d','base',{0:'DIGLIMIT'},'NUMBAD')
p=state('DIGLIMIT').a(('A64','sub','bound','limit','d'),('A64','udiv','bound','bound','base'))
p.branch({2:'NUMBAD'},'DIGACC',[('C64U','nv','bound')])
state('DIGACC').a(('A64','mul','nv','nv','base'),('A64','add','nv','nv','d'),('ADV',),('ALUI','add','digits','digits',1)).goto('DIGZERO')
test(state('DIGZERO'),'d',0,'DIGIT','DIGSET',True)
state('DIGSET').a(('LDI','nonzero',1)).goto('DIGIT')
test(state('NUMEND'),'digits',0,'NUMBAD','NUMCANON',True)
test(state('NUMCANON'),'leadzero',1,'NUMLEAD','NUMSIGN',True)
test(state('NUMLEAD'),'allowzeros',1,'NUMSIGN','NUMLEADSTRICT',True)
test(state('NUMLEADSTRICT'),'nonzero',1,'NUMBAD','NUMSIGN',True)
test(state('NUMSIGN'),'neg',1,'NUMNEGATE','NUMOK',True)
state('NUMNEGATE').a(('A64','sub','nv','zero','nv')).goto('NUMOK')
state('NUMOK').a(('INPOP',),('LDI','ok',1)).ret()
retbad('NUMBAD')
# Data directive classification occurs before generic opcode lookup.
# All accepted data rows survive output even inside dead units.
p=state('DATANAME');test(p,'nt',3,'DATANAMETOK','FALLBACK',True)
p=state('DATANAMETOK');setv(p,'ti',1);p.call('TOKGET').call('NAME');test(p,'ok',1,'DATASAVE','FALLBACK',True)
p=state('DATASAVE');put(p,'DATA','id','one');put(p,'RN','row','id');setv(p,'v',3);put(p,'RK','row','v');setv(p,'ti',2);p.call('TOKGET');test(p,'datakind',1,'BSSCHECK','STRCHECK',True)
p=state('BSSCHECK').a(('LDI','allowzeros',1)).call('NUMBER');test(p,'ok',1,'BSSRANGE','FALLBACK',True)
p=state('BSSRANGE');p.branch({0:'FALLBACK'},'BSSDIGITS',[('C64','nv','zero')])
# Range [0,32MiB]; decimal digits only (no signed/hex spelling).
p=state('BSSDIGITS');p.branch({2:'FALLBACK'},'BSSVIEW',[('LDI','limit',33554432),('C64','nv','limit')])
state('BSSVIEW').a(('INPUSHXE','tb','te')).goto('BSSBYTE')
byte('BSSBYTE',{tuple(range(48,58)):'BSSADV',256:'BSSDONE'},'FALLBACK')
state('BSSADV').a(('ADV',)).goto('BSSBYTE')
state('BSSDONE').a(('INPOP',)).goto('ROWNEXT')
# QUOTE scanner guaranteed exact escape-balanced token, require quote endpoints.
state('STRCHECK').a(('INPUSHXE','tb','te'),('BYTE','c'),('INPOP',)).goto('STRFIRST')
test(state('STRFIRST'),'c',34,'STRLAST','FALLBACK',True)
state('STRLAST').a(('ALUI','sub','v','te',1),('INPUSHXE','v','te'),('BYTE','c'),('INPOP',)).goto('STREND')
test(state('STREND'),'c',34,'ROWNEXT','FALLBACK',True)
# Finish name directory, detect code/data ambiguity, and mark positive entries.
state('PARSED').a(('COPYW','nrows','row'),('LDI','ni',0)).goto('NAMELOOP')
test(state('NAMELOOP'),'ni','names','ANCHORCALL','NAMEFACT')
p=state('NAMEFACT');get(p,'id','NL','ni');get(p,'lr','LAB','id');test(p,'lr',0,'NAMENEXT','NAMECODE',True)
p=state('NAMECODE');get(p,'v','DATA','id');test(p,'v',1,'FALLBACK','PROSTART',True)
p=state('PROSTART');cp(p,'pr','lr');setv(p,'pn',0);p.goto('PROLOOP')
p=state('PROLOOP');cmp(p,'pr','nrows',{0:'PROFACT'},'NAMENEXT')
p=state('PROFACT');get(p,'v','RK','pr');p.branch({3:'NAMENEXT',1:'PROINS'},'PROADV',[('RLD','v')])
p=state('PROINS');put(p,'TS','pn','pr');inc(p,'pn');test(p,'pn',4,'PROTEST','PROADV',True)
state('PROADV').a(('ALUI','add','pr','pr',1)).goto('PROLOOP')
p=state('PROTEST')
for j,op,vals in [(0,'.frame',[8]),(1,'store64',[7,0,6]),(2,'mov',[6,7]),(3,'.frame',[])]:
 setv(p,'v',j);get(p,'pr','TS','v');get(p,'v','RO','pr');yes=p.fresh('pro');test(p,'v',OPS[op],yes,'NAMENEXT',True);p=state(yes)
 mul(p,'argi','pr',8)
 for k,val in enumerate(vals):
  get(p,'v','AV','argi');yes=p.fresh('proarg');test(p,'v',val,yes,'NAMENEXT',True);p=state(yes);inc(p,'argi')
get(p,'v','AV','argi');p.branch({0:'NAMENEXT'},'MARKENTRY',[('C64','v','zero')])
p=state('MARKENTRY');cp(p,'v','lr');inc(p,'v',-1);get(p,'v','RP','v');get(p,'v','FIRST','v');inc(p,'v',-1);put(p,'START','v','one');p.goto('NAMENEXT')
state('NAMENEXT').a(('ALUI','add','ni','ni',1)).goto('NAMELOOP')
p=state('ANCHORCALL');setv(p,'ri',0);p.goto('ANCHORLOOP')
test(state('ANCHORLOOP'),'ri','nrows','SPECIAL','ANCHORFACT')
p=state('ANCHORFACT');get(p,'v','RK','ri');test(p,'v',1,'ANCHOROP','ANCHORNEXT',True)
p=state('ANCHOROP');get(p,'v','RO','ri');test(p,'v',OPS['call'],'ANCHORTARGET','ANCHORNEXT',True)
p=state('ANCHORTARGET');mul(p,'argi','ri',8);get(p,'id','AN','argi');get(p,'lr','LAB','id');test(p,'lr',0,'FALLBACK','ANCHORMARK',True)
p=state('ANCHORMARK');cp(p,'v','lr');inc(p,'v',-1);get(p,'v','RP','v');get(p,'v','FIRST','v');inc(p,'v',-1);put(p,'START','v','one');p.goto('ANCHORNEXT')
state('ANCHORNEXT').a(('ALUI','add','ri','ri',1)).goto('ANCHORLOOP')
p=state('SPECIAL')
for name in ['_start','main','__init','__main_ret']:
 get(p,'lr','LAB','ID'+name);nxt=p.fresh('special');yes=p.fresh('mark');test(p,'lr',0,nxt,yes,True);p=state(yes);cp(p,'v','lr');inc(p,'v',-1);get(p,'v','RP','v');get(p,'v','FIRST','v');inc(p,'v',-1);put(p,'START','v','one');p.goto(nxt);p=state(nxt)
put(p,'START','zero','one');setv(p,'ri',0);setv(p,'un',0);setv(p,'unit',0);p.goto('UNITLOOP')
test(state('UNITLOOP'),'ri','nrows','UNITDONE','UNITFACT')
p=state('UNITFACT');get(p,'v','START','ri');test(p,'v',1,'UNITNEW','UNITOWNER',True)
p=state('UNITNEW');cp(p,'unit','un');inc(p,'un');put(p,'US','unit','ri');setv(p,'v',-1);put(p,'HEAD','unit','v');p.goto('UNITOWNER')
p=state('UNITOWNER');put(p,'OWNER','ri','unit');inc(p,'ri');p.goto('UNITLOOP')
p=state('UNITDONE');get(p,'lr','LAB','ID_start');test(p,'lr',0,'FIRSTROOT','STARTROOT',True)
p=state('STARTROOT');inc(p,'lr',-1);get(p,'u','OWNER','lr');put(p,'ROOTS','u','one');p.goto('OTHERROOT')
state('FIRSTROOT').a(('LDI','ri',0)).goto('FIRSTROOTLOOP')
test(state('FIRSTROOTLOOP'),'ri','nrows','OTHERROOT','FIRSTROOTFACT')
p=state('FIRSTROOTFACT');get(p,'v','RK','ri');test(p,'v',1,'FIRSTROOTMARK','FIRSTROOTNEXT',True)
p=state('FIRSTROOTMARK');get(p,'u','OWNER','ri');put(p,'ROOTS','u','one');p.goto('OTHERROOT')
state('FIRSTROOTNEXT').a(('ALUI','add','ri','ri',1)).goto('FIRSTROOTLOOP')
p=state('OTHERROOT')
for name in ['main','__init','__main_ret']:
 get(p,'lr','LAB','ID'+name);nxt=p.fresh('root');yes=p.fresh('rootmark');test(p,'lr',0,nxt,yes,True);p=state(yes);inc(p,'lr',-1);get(p,'u','OWNER','lr');put(p,'ROOTS','u','one');p.goto(nxt);p=state(nxt)
setv(p,'ri',0);setv(p,'edges',0);setv(p,'prev',0);setv(p,'lastop',0);p.goto('EDGELOOP')
test(state('EDGELOOP'),'ri','nrows','QUEUEST','EDGEFACT')
p=state('EDGEFACT');get(p,'u','OWNER','ri');test(p,'u','prev','EDGEROW','FALLTEST')
p=state('FALLTEST');p.branch({(OPS['ret'],OPS['jump'],OPS['.exit']):'FALLDONE'},'FALLEDGE',[('RLD','lastop')])
p=state('FALLEDGE');cp(p,'from','prev');cp(p,'to','u');p.call('EDGE');p.goto('FALLDONE')
state('FALLDONE').a(('COPYW','prev','u'),('LDI','lastop',0)).goto('EDGEROW')
p=state('EDGEROW');get(p,'v','RK','ri');test(p,'v',1,'EDGEOP','EDGENEXT',True)
p=state('EDGEOP');get(p,'op','RO','ri');cp(p,'lastop','op');p.branch({(OPS['call'],OPS['jump'],OPS['jumpz']):'DIRECT',OPS['.lea']:'ADDRESS',(OPS['callr'],OPS['callm']):'INDIRECT'},'EDGENEXT',[('RLD','op')])
p=state('DIRECT');mul(p,'argi','ri',8);test(p,'op',OPS['jumpz'],'DIRECTZ','DIRECTGET',True)
state('DIRECTZ').a(('ALUI','add','argi','argi',1)).goto('DIRECTGET')
p=state('DIRECTGET');get(p,'id','AN','argi');get(p,'lr','LAB','id');test(p,'lr',0,'FALLBACK','DIRECTEDGE',True)
p=state('DIRECTEDGE');inc(p,'lr',-1);get(p,'to','OWNER','lr');cp(p,'from','u');p.call('EDGE');p.goto('EDGENEXT')
p=state('ADDRESS');mul(p,'argi','ri',8);inc(p,'argi');get(p,'id','AN','argi');test(p,'id',0,'EDGENEXT','ADDRESSNAME',True)
p=state('ADDRESSNAME');get(p,'lr','LAB','id');test(p,'lr',0,'ADDRESSDATA','ADDRESSROOT',True)
p=state('ADDRESSDATA');get(p,'v','DATA','id');test(p,'v',1,'EDGENEXT','FALLBACK',True)
p=state('ADDRESSROOT');inc(p,'lr',-1);get(p,'to','OWNER','lr');put(p,'ROOTS','to','one');p.goto('EDGENEXT')
p=state('INDIRECT');put(p,'CALLR','u','one');p.goto('EDGENEXT')
state('EDGENEXT').a(('ALUI','add','ri','ri',1)).goto('EDGELOOP')
p=state('EDGE');cmp(p,'edges',EDGEMAX,{1:'FALLBACK',2:'FALLBACK'},'EDGEADD',True)
p=state('EDGEADD');put(p,'TO','edges','to');get(p,'v','HEAD','from');put(p,'NEXT','edges','v');put(p,'HEAD','from','edges');inc(p,'edges');p.ret()
state('QUEUEST').a(('LDI','u',0),('LDI','tail',0)).goto('ROOTLOOP')
test(state('ROOTLOOP'),'u','un','CLOSEST','ROOTFACT')
p=state('ROOTFACT');get(p,'v','ROOTS','u');test(p,'v',1,'ROOTADD','ROOTNEXT',True)
p=state('ROOTADD');put(p,'ALIVE','u','one');put(p,'QUEUE','tail','u');inc(p,'tail');p.goto('ROOTNEXT')
state('ROOTNEXT').a(('ALUI','add','u','u',1)).goto('ROOTLOOP')
state('CLOSEST').a(('LDI','pos',0)).goto('CLOSELOOP')
test(state('CLOSELOOP'),'pos','tail','OUTPUTST','CLOSEFACT')
p=state('CLOSEFACT');get(p,'u','QUEUE','pos');inc(p,'pos');get(p,'v','CALLR','u');test(p,'v',1,'FALLBACK','CLOSEHEAD',True)
p=state('CLOSEHEAD');get(p,'e','HEAD','u');p.goto('ADJLOOP')
cmp(state('ADJLOOP'),'e',0,{0:'CLOSELOOP'},'ADJFACT',True)
p=state('ADJFACT');get(p,'to','TO','e');get(p,'v','ALIVE','to');test(p,'v',0,'ADJADD','ADJNEXT',True)
p=state('ADJADD');put(p,'ALIVE','to','one');put(p,'QUEUE','tail','to');inc(p,'tail');p.goto('ADJNEXT')
p=state('ADJNEXT');get(p,'e','NEXT','e');p.goto('ADJLOOP')
state('OUTPUTST').a(('LDI','ri',0)).goto('OUTPUTLOOP')
test(state('OUTPUTLOOP'),'ri','nrows','ACCEPT','OUTPUTFACT')
p=state('OUTPUTFACT');get(p,'v','RK','ri');test(p,'v',3,'OUTPUTROW','OUTPUTOWNER',True)
p=state('OUTPUTOWNER');get(p,'u','OWNER','ri');get(p,'v','ALIVE','u');test(p,'v',1,'OUTPUTROW','OUTPUTNEXT',True)
p=state('OUTPUTROW');get(p,'s','RS','ri');get(p,'e','RE','ri');p.a(('SPAN2','s','e')).goto('OUTPUTNEXT')
state('OUTPUTNEXT').a(('ALUI','add','ri','ri',1)).goto('OUTPUTLOOP')
state('ACCEPT').a(('ACCEPT',)).goto('DEAD')
# Fallback may occur with a token reader pushed: reset to original frame view.
state('FALLBACK').a(('OCLR',),('INPUSHXE','zero','size'),('SPAN2','zero','size'),('ACCEPT',)).goto('DEAD')
# Replace CLASS body with data tests, retaining ordinary instruction states.
# P generated a total CLASS row; its entry action is identical for all r values.
nameconstacts=lambda s:[('SBCLR',)]+[('SBOUT',c) for c in s.encode()]+[('SBINTERN','dataid')]
p=state('DATAOP');p.goto('INSSTART')
# CLASS now computes token intern once and tests the data IDs before OP map.
# Rebuild that state with the same outgoing state/action declarations.
g.st.pop('CLASS')
p=state('CLASS');setv(p,'ti',0);p.call('TOKGET');p.a(('INTERN','id','tb','te'));cp(p,'savedid','id');nameconst(p,'.bss','dataid');test(p,'savedid','dataid','ISBSS','CHECKSTR')
state('ISBSS').a(('LDI','datakind',1)).goto('DATANAME')
p=state('CHECKSTR');nameconst(p,'.str','dataid');test(p,'savedid','dataid','ISSTR','NORMALOP')
state('ISSTR').a(('LDI','datakind',2)).goto('DATANAME')
p=state('NORMALOP');cp(p,'id','savedid');get(p,'op','OP','id');test(p,'op',0,'LABEL','OPCLASS',True)

def construct():
 from libraryroots import install
 install(E,P,B,NAMEMAX)
 g.finish()
 return {'start':'START','states':{n:[mode,{str(k):v for k,v in row.items()}] for n,(mode,row) in g.st.items()},'seqs':g.seqs}

if __name__=='__main__':
 if len(sys.argv)!=2: raise SystemExit('usage: gen.py PRIVATE_DELTA.json')
 d=construct();Path(sys.argv[1]).write_text(json.dumps(d,separators=(',',':'))+'\n')
 print(json.dumps({'states':len(d['states']),'sequences':len(d['seqs']),'schema_sha256':__import__('hashlib').sha256((ROOT/'unisa/tape.py').read_bytes()).hexdigest(),'capacities':[SRCMAX,ROWMAX,NAMEMAX,EDGEMAX]}))
