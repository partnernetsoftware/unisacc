"""TIns metadata and tagged argument contracts, plus non-address setup forms."""
from tins import META
KEYS=75000000
SEEN=76000000


def init(p):
    for key in ('imm','reg','mem','addr','setreg','gate','jump','jumpz','call','true','false','svc0','svc80','svc','arm','winapi','arm19','arm26'):
        p.a(('SBCLR',),[('SBOUT',c) for c in key.encode()],('SBINTERN','id_'+key))
    for n,key in enumerate(META,1):
        p.a(('SBCLR',),[('SBOUT',c) for c in key.encode()],('SBINTERN','t'),('LDI','u',n),('STX','t',KEYS,'u'))


def install(E,word):
    P,g=E.P,E.g
    P('TAG.check').branch({1:'TAG.pos'},'BOOL.check',[('CMP','oid','id_setreg')])
    P('TAG.pos').branch({1:'TAG.unset'},'REG.value',[('CMPI','n',1)])
    P('TAG.unset').branch({1:'TAG.match'},'REG.value',[('CMPI','stag',0)])
    P('TAG.match').branch({1:'TAG.imm'},'TAG.reg',[('CMP','t','id_imm')])
    P('TAG.imm').a(('LDI','stag',2),('LDI','tagkind',2)).goto('ARG')
    P('TAG.reg').branch({1:'TAG.regok'},'TAG.mem',[('CMP','t','id_reg')])
    P('TAG.regok').a(('LDI','stag',1),('LDI','tagkind',1)).goto('ARG')
    P('TAG.mem').branch({1:'TAG.memok'},'TAG.addr',[('CMP','t','id_mem')])
    P('TAG.memok').a(('LDI','stag',4),('LDI','tagkind',2)).goto('ARG')
    P('TAG.addr').branch({1:'TAG.addrok'},'FAIL',[('CMP','t','id_addr')])
    P('TAG.addrok').a(('LDI','stag',3),('LDI','tagkind',2)).goto('ARG')
    P('BOOL.check').branch({1:'BOOL.true'},'BOOL.other',[('CMP','t','id_true')])
    P('BOOL.other').branch({1:'BOOL.false'},'REG.value',[('CMP','t','id_false')])
    P('BOOL.true').a(('LDI','v',1),('LDI','kind',4)).goto('PUT')
    P('BOOL.false').a(('LDI','v',0),('LDI','kind',4)).goto('PUT')
    g.on('META.key',[61],'META.begin',[('MARK','end'),('ADV',)])
    g.on('META.key',[32,10,256],'FAIL',[])
    g.els('META.key','META.key',[('ADV',)])
    P('META.begin').a(('INTERN','mk','start','end'),('LDX','known','mk',KEYS),('LDX','seen','mk',SEEN)).branch({1:'FAIL'},'META.known',[('CMP','seen','lnum')])
    P('META.known').branch({1:'FAIL'},'META.value',[('CMPI','known',0)])
    g.on('META.value',[32,10,256],'FAIL',[])
    g.els('META.value','META.scan',[('MARK','vs'),('STX','mk',SEEN,'lnum')])
    g.on('META.scan',[32,10,256],'META.end',[('MARK','ve')])
    g.els('META.scan','META.scan',[('ADV',)])
    semantic={META.index(k)+1:'META.'+k for k in ('reloc','form','gate','carry')}
    P('META.end').a(('INTERN','mv','vs','ve')).branch(semantic,'AFTER',[('RLD','known')])
    P('META.reloc').branch({1:'META.rel19'},'META.relother',[('CMP','oid','id_jumpz')])
    P('META.rel19').branch({1:'AFTER'},'FAIL',[('CMP','mv','id_arm19')])
    P('META.relother').branch({1:'META.rel26'},'META.relcall',[('CMP','oid','id_jump')])
    P('META.relcall').branch({1:'META.rel26'},'FAIL',[('CMP','oid','id_call')])
    P('META.rel26').branch({1:'AFTER'},'FAIL',[('CMP','mv','id_arm26')])
    P('META.form').branch({1:'META.gform'},'AFTER',[('CMP','oid','id_gate')])
    P('META.gform').branch({1:'AFTER'},'FAIL',[('CMP','mv','id_svc')])
    for key in ('gate','carry'):
        P('META.'+key).branch({1:'META.g'+key},'FAIL',[('CMP','oid','id_gate')])
    P('META.ggate').branch({1:'AFTER'},'META.svc80',[('CMP','mv','id_svc0')])
    P('META.svc80').branch({1:'META.set80'},'FAIL',[('CMP','mv','id_svc80')])
    P('META.set80').a(('LDI','gkind',1)).goto('AFTER')
    P('META.gcarry').branch({1:'AFTER'},'META.true',[('CMP','mv','id_false')])
    P('META.true').branch({1:'META.setcarry'},'FAIL',[('CMP','mv','id_true')])
    P('META.setcarry').a(('LDI','gcarry',1)).goto('AFTER')
    P('EMIT.25').branch({1:'E.movalias',2:'EMIT.2',3:'AD.addr',4:'AD.mem'},'FAIL',[('RLD','stag')])
    P('E.movalias').goto('EMIT.1')
    P('E.immalias').branch({1:'EMIT.2'},'FAIL',[('CMPI','stag',2)])
    word(P('EMIT.26').a(('ALUI','or','w','a0',0x910003E0))).goto('LINE')
    P('EMIT.27').branch({1:'GATE.80'},'GATE.0',[('CMPI','gkind',1)])
    word(P('GATE.80').a(('LDI','w',0xD4001001))).goto('GATE.carry')
    word(P('GATE.0').a(('LDI','w',0xD4000001))).goto('GATE.carry')
    P('GATE.carry').branch({1:'GATE.fix'},'LINE',[('CMPI','gcarry',1)])
    p=P('GATE.fix')
    for v in (0x54000043,0xCB0003E0):word(p.a(('LDI','w',v)))
    p.goto('LINE')
