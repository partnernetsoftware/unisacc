"""Object header and ELF symbol-plan delta. All runtime work is generic actions."""
OBJ_RELOCS = 250 << 40
OBJ_NREL = 'obj_nrel'
NAMES, IDS, SEEN, FLAGS, KIND, POS, STRINGS, SYMBOLS, TEXT, TAPE = (i << 40 for i in range(251,261))

def install(E, byte, OFF, LABD, SYM, PRESENT, arch='x86_64', direct_labels=False):
    from elfimage import DATA
    from elfobject import install as writer
    g=E.g
    class P(E.P):
        def b(self,cases,other):
            used=set()
            for k,t in cases.items():
                ks=list(k) if isinstance(k,tuple) else [k]
                g.on(self.cur,ks,t,self.acts,'b');used.update(ks)
            g.on(self.cur,set(range(257))-used,other,self.acts,'b')
            self.cur=None;self.acts=[]
            return self
    def move(s):
        n='OBJ.original.'+s;assert n not in g.st
        g.st[n]=g.st.pop(s);g.labels.add(n);return n
    start=move('START')
    p=P('START').a(('LDI','obj_names',0),('LDI',OBJ_NREL,0),('LDI','obj_hasnz',0),('LDI','obj_hastape',0),('LDI','obj_unit',1),('LDI','obj_hasunit',0))
    keys={}
    for key in ('@obj_name','@obj_nzend','@obj_tape','_start','@obj_unit'):
        r='obj_key'+str(len(keys));keys[key]=r
        p.a(('SBCLR',),[('SBOUT',c) for c in key.encode()],('SBINTERN',r))
    p.goto(start)
    if direct_labels:
        seen=move('HDR.seen')
        P('HDR.seen').branch({1:'OBJ.name'},seen,[('CMP','hk',keys['@obj_name'])])
    old=move('HDR.next.argv');p=P('HDR.next.argv')
    for key,dest in (('@obj_name','OBJ.name'),('@obj_nzend','OBJ.nz'),('@obj_tape','OBJ.raw'),('@obj_unit','OBJ.unit')):
        n=p.fresh('r');p.branch({1:dest},n,[('CMP','hk',keys[key])]);p.label(n)
    p.goto(old)
    P('OBJ.unit').a(('CMPI','obj_hasunit',0)).branch({1:'OBJ.unitstart'},'OBJ.fail')
    P('OBJ.unitstart').a(('LDI','obj_hasunit',1),('JUMP','hv')).goto('OBJ.unitread')
    P('OBJ.unitread').b({48:'OBJ.unit0',49:'OBJ.unit1'},'OBJ.fail')
    for unit in (0,1):
        P('OBJ.unit'+str(unit)).a(('LDI','obj_unit',unit),('ADV',)).goto('OBJ.unittail')
    P('OBJ.unittail').b({(10,256):'SKIPL'},'OBJ.fail')
    P('OBJ.name').a(('JUMP','hv'),('MARK','obj_ns')).goto('OBJ.nameword')
    letters=tuple(range(65,91))+tuple(range(97,123))+(95,46,36)+tuple(range(48,58))
    P('OBJ.nameword').b({letters:'OBJ.namechar',32:'OBJ.nameend'},'OBJ.fail')
    P('OBJ.namechar').a(('ADV',)).goto('OBJ.nameword')
    P('OBJ.nameend').a(('MARK','obj_ne'),('CMP','obj_ns','obj_ne')).branch({1:'OBJ.fail'},'OBJ.nameid')
    P('OBJ.nameid').a(('INTERN','obj_id','obj_ns','obj_ne'),('LDX','obj_seen','obj_id',SEEN)).branch({0:'OBJ.nameflag'},'OBJ.fail',[('RLD','obj_seen')])
    P('OBJ.nameflag').a(('BLOBSAVE','obj_blob','obj_ns','obj_ne'),('STX','obj_names',NAMES,'obj_blob'),('STX','obj_names',IDS,'obj_id'),('LDI','obj_one',1),('STX','obj_id',SEEN,'obj_one'),('ADV',)).goto('OBJ.flagread')
    P('OBJ.flagread').b({48:'OBJ.local',49:'OBJ.global'},'OBJ.fail')
    P('OBJ.local').a(('LDI','obj_global',0),('ADV',)).goto('OBJ.flagsep')
    P('OBJ.global').a(('LDI','obj_global',1),('ADV',)).goto('OBJ.flagsep')
    P('OBJ.flagsep').b({32:'OBJ.flagspace'},'OBJ.fail')
    P('OBJ.flagspace').a(('ADV',)).goto('OBJ.flagread2')
    P('OBJ.flagread2').b({(48,49):'OBJ.flagend'},'OBJ.fail')
    P('OBJ.flagend').a(('ADV',)).goto('OBJ.flagtail')
    P('OBJ.flagtail').b({(10,256):'OBJ.namestore'},'OBJ.fail')
    P('OBJ.namestore').a(('STX','obj_id',FLAGS,'obj_global'),('ALUI','add','obj_names','obj_names',1)).goto('SKIPL')
    for tag,flag,blob,nextstate in [('nz','obj_hasnz','obj_nzblob','OBJ.nznum'),('raw','obj_hastape','obj_rawblob','SKIPL')]:
        P('OBJ.'+tag).a(('CMPI',flag,0)).branch({1:'OBJ.'+tag+'save'},'OBJ.fail')
        P('OBJ.'+tag+'save').a(('LDI',flag,1),('BLOBSAVE',blob,'hv','hend')).goto(nextstate)
    P('OBJ.nznum').a(('LDI','extent_max',2147483647),('INPUSH','obj_nzblob')).call('EM.num').a(('COPYW','obj_nzend','mn')).goto('SKIPL')
    move('EH.elfwrite')
    P('EH.elfwrite').a(('CMPI','obj_hasnz',1)).branch({1:'OBJ.checktape'},'OBJ.fail')
    P('OBJ.checktape').a(('CMPI','obj_hastape',1)).branch({1:'OBJ.plan'},'OBJ.fail')
    P('OBJ.plan').a(('COPYW','eo_nzend','obj_nzend'),('COPYW','eo_datalen','vlen'),('COPYW','eo_textlen','endo'),('COPYW','eo_nrel',OBJ_NREL),('LDI','obj_i',0)).goto('OBJ.classloop')
    P('OBJ.classloop').a(('CMP','obj_i','obj_names')).branch({0:'OBJ.class'},'OBJ.relmark')
    P('OBJ.class').a(('LDX','obj_id','obj_i',IDS),('LDX','obj_def','obj_id',LABD),('LDX','obj_data','obj_id',PRESENT),('ALU','or','obj_def','obj_def','obj_data')).branch({0:'OBJ.classundef'},'OBJ.classdef',[('RLD','obj_def')])
    P('OBJ.classundef').a(('LDI','obj_kind',0)).goto('OBJ.classstore')
    P('OBJ.classdef').a(('LDX','obj_global','obj_id',FLAGS),('CMP','obj_id',keys['_start'])).branch({1:'OBJ.classglobal'},'OBJ.classflag')
    P('OBJ.classflag').a(('RLD','obj_global')).branch({1:'OBJ.classglobal'},'OBJ.classlocal')
    P('OBJ.classglobal').a(('LDI','obj_kind',2)).goto('OBJ.classstore')
    P('OBJ.classlocal').a(('LDI','obj_kind',1)).goto('OBJ.classstore')
    P('OBJ.classstore').a(('STX','obj_id',KIND,'obj_kind'),('ALUI','add','obj_i','obj_i',1)).goto('OBJ.classloop')
    P('OBJ.relmark').a(('LDI','obj_i',0)).goto('OBJ.markloop')
    P('OBJ.markloop').a(('CMP','obj_i',OBJ_NREL)).branch({0:'OBJ.mark'},'OBJ.orderstart')
    P('OBJ.mark').a(('ALUI','mul','obj_row','obj_i',4),('ALUI','add','obj_ix','obj_row',2),('LDX','obj_ref','obj_ix',OBJ_RELOCS),('CMPI','obj_ref',1000)).branch({(1,2):'OBJ.markname'},'OBJ.marknext')
    P('OBJ.markname').a(('ALUI','sub','obj_id','obj_ref',1000),('LDX','obj_seen','obj_id',SEEN)).branch({1:'OBJ.markkind'},'OBJ.fail',[('RLD','obj_seen')])
    P('OBJ.markkind').a(('LDX','obj_kind','obj_id',KIND)).branch({0:'OBJ.markundef'},'OBJ.marknext',[('RLD','obj_kind')])
    P('OBJ.markundef').a(('LDI','obj_kind',3),('STX','obj_id',KIND,'obj_kind')).goto('OBJ.marknext')
    P('OBJ.marknext').a(('ALUI','add','obj_i','obj_i',1)).goto('OBJ.markloop')
    P('OBJ.orderstart').a(('LDI','obj_group',1),('LDI','eo_nsym',0),('LDI','eo_nlocal',0),('LDI','eo_strsize',1),('LDI','obj_zero',0),('STX','obj_zero',STRINGS,'obj_zero')).goto('OBJ.group')
    P('OBJ.group').a(('LDI','obj_i',0)).goto('OBJ.orderloop')
    P('OBJ.orderloop').a(('CMP','obj_i','obj_names')).branch({0:'OBJ.order'},'OBJ.nextgroup')
    P('OBJ.order').a(('LDX','obj_id','obj_i',IDS),('LDX','obj_kind','obj_id',KIND),('CMP','obj_kind','obj_group')).branch({1:'OBJ.sym'},'OBJ.ordernext')
    P('OBJ.sym').a(('ALUI','add','obj_symix','eo_nsym',4),('STX','obj_id',POS,'obj_symix'),('ALUI','mul','obj_row','eo_nsym',4),('STX','obj_row',SYMBOLS,'eo_strsize'),('LDX','obj_blob','obj_i',NAMES),('INPUSH','obj_blob'),('LDI','obj_skip',0),('CMPI','obj_kind',1)).branch({1:'OBJ.strloop'},'OBJ.prefix')
    P('OBJ.prefix').b({103:'OBJ.prefixg'},'OBJ.strloop')
    P('OBJ.prefixg').a(('ADV',)).goto('OBJ.prefixcheck')
    P('OBJ.prefixcheck').b({95:'OBJ.prefixunder'},'OBJ.prefixrewind')
    P('OBJ.prefixrewind').a(('JUMP','obj_zero')).goto('OBJ.strloop')
    P('OBJ.prefixunder').a(('ADV',)).goto('OBJ.prefixmore')
    P('OBJ.prefixmore').b({256:'OBJ.prefixrewind'},'OBJ.strloop')
    P('OBJ.strloop').b({256:'OBJ.strend'},'OBJ.strbyte')
    P('OBJ.strbyte').a(('BYTE','obj_c'),('STX','eo_strsize',STRINGS,'obj_c'),('ALUI','add','eo_strsize','eo_strsize',1),('ADV',)).goto('OBJ.strloop')
    P('OBJ.strend').a(('STX','eo_strsize',STRINGS,'obj_zero'),('ALUI','add','eo_strsize','eo_strsize',1),('INPOP',),('LDX','obj_code','obj_id',LABD),('CMPI','obj_kind',3)).branch({1:'OBJ.symundef'},'OBJ.symdefined')
    P('OBJ.symundef').a(('LDI','obj_sec',0),('LDI','obj_val',0),('LDI','obj_info',16)).goto('OBJ.syminfo')
    P('OBJ.symdefined').a(('CMPI','obj_code',0)).branch({1:'OBJ.symdata'},'OBJ.symcode')
    P('OBJ.symcode').a(('LDI','obj_sec',1),('LDI','obj_info',0),('CMPI','obj_kind',2)).branch({1:'OBJ.symfunc'},'OBJ.symoffset')
    P('OBJ.symfunc').a(('LDI','obj_info',18)).goto('OBJ.symoffset')
    if direct_labels:
        P('OBJ.symoffset').a(('ALUI','sub','obj_val','obj_code',1)).goto('OBJ.syminfo')
    else:
        P('OBJ.symoffset').a(('ALUI','sub','obj_code','obj_code',1),('CMP','obj_code','npc')).branch({0:'OBJ.symoffread'},'OBJ.symoffend')
        P('OBJ.symoffread').a(('LDX','obj_val','obj_code',OFF)).goto('OBJ.syminfo')
        P('OBJ.symoffend').a(('COPYW','obj_val','endo')).goto('OBJ.syminfo')
    P('OBJ.symdata').a(('LDX','obj_val','obj_id',SYM),('ALUI','sub','obj_val','obj_val',256),('LDI','obj_info',0),('CMPI','obj_kind',2)).branch({1:'OBJ.symobject'},'OBJ.datasec')
    P('OBJ.symobject').a(('LDI','obj_info',17)).goto('OBJ.datasec')
    P('OBJ.datasec').a(('C64U','obj_val','eo_nzend')).branch({0:'OBJ.symdata2'},'OBJ.symdata3')
    P('OBJ.symdata2').a(('LDI','obj_sec',2)).goto('OBJ.syminfo')
    P('OBJ.symdata3').a(('LDI','obj_sec',3),('A64','sub','obj_val','obj_val','eo_nzend')).goto('OBJ.syminfo')
    p=P('OBJ.syminfo')
    for k,r in [(1,'obj_info'),(2,'obj_sec'),(3,'obj_val')]:p.a(('ALUI','add','obj_ix','obj_row',k),('STX','obj_ix',SYMBOLS,r))
    p.a(('ALUI','add','eo_nsym','eo_nsym',1),('CMPI','obj_kind',1)).branch({1:'OBJ.localcount'},'OBJ.ordernext')
    P('OBJ.localcount').a(('ALUI','add','eo_nlocal','eo_nlocal',1)).goto('OBJ.ordernext')
    P('OBJ.ordernext').a(('ALUI','add','obj_i','obj_i',1)).goto('OBJ.orderloop')
    P('OBJ.nextgroup').a(('ALUI','add','obj_group','obj_group',1),('CMPI','obj_group',4)).branch({1:'OBJ.relstart'},'OBJ.group')
    P('OBJ.relstart').a(('LDI','obj_i',0)).goto('OBJ.relloop')
    P('OBJ.relloop').a(('CMP','obj_i',OBJ_NREL)).branch({0:'OBJ.rel'},'OBJ.textstart')
    P('OBJ.rel').a(('ALUI','mul','obj_row','obj_i',4),('ALUI','add','obj_ix','obj_row',2),('LDX','obj_ref','obj_ix',OBJ_RELOCS),('CMPI','obj_ref',1000)).branch({(1,2):'OBJ.relname'},'OBJ.relnext')
    P('OBJ.relname').a(('ALUI','sub','obj_id','obj_ref',1000),('LDX','obj_ref','obj_id',POS),('STX','obj_ix',OBJ_RELOCS,'obj_ref')).goto('OBJ.relnext')
    P('OBJ.relnext').a(('ALUI','add','obj_i','obj_i',1)).goto('OBJ.relloop')
    P('OBJ.textstart').a(('INPUSH','text_blob'),('LDI','obj_i',0)).goto('OBJ.textloop')
    P('OBJ.textloop').b({256:'OBJ.textend'},'OBJ.textbyte')
    P('OBJ.textbyte').a(('BYTE','obj_c'),('STX','obj_i',TEXT,'obj_c'),('ALUI','add','obj_i','obj_i',1),('ADV',)).goto('OBJ.textloop')
    P('OBJ.textend').a(('INPOP',),('INPUSH','obj_rawblob'),('LDI','obj_rawlen',0)).goto('OBJ.hexhi')
    hexmap={c:n for n,c in enumerate(b'0123456789abcdef')}
    p=P('OBJ.hexhi');p.b({256:'OBJ.rawend',tuple(hexmap):'OBJ.hibyte'},'OBJ.fail')
    for state,reg,nextstate in [('hibyte','obj_hi','OBJ.hexlo'),('lobyte','obj_lo','OBJ.hexput')]:
        p=P('OBJ.'+state)
        for c,n in hexmap.items():
            nxt=p.fresh('r');p.branch({1:'OBJ.'+state+str(n)},nxt,[('BYTE','obj_c'),('CMPI','obj_c',c)]);p.label(nxt)
            P('OBJ.'+state+str(n)).a(('LDI',reg,n),('ADV',)).goto(nextstate)
        p.goto('OBJ.fail')
    P('OBJ.hexlo').b({tuple(hexmap):'OBJ.lobyte'},'OBJ.fail')
    P('OBJ.hexput').a(('ALUI','shl','obj_c','obj_hi',4),('ALU','or','obj_c','obj_c','obj_lo'),('STX','obj_rawlen',TAPE,'obj_c'),('ALUI','add','obj_rawlen','obj_rawlen',1)).goto('OBJ.hexhi')
    P('OBJ.rawend').a(('INPOP',),('LDI','obj_zero',0),('OLEN','obj_oldout'),('COPYW','n','obj_rawlen')).o('UNISATAPE1 lnx/'+arch+' ').call('PRN').o('\n').a(('OCUT','obj_prefix','obj_zero'),('INPUSH','obj_prefix'),('LDI','obj_prefixlen',0)).goto('OBJ.prefixloop')
    P('OBJ.prefixloop').b({256:'OBJ.prefixend'},'OBJ.prefixbyte')
    P('OBJ.prefixbyte').a(('BYTE','obj_c'),('STX','obj_prefixlen',STRINGS+(1<<39),'obj_c'),('ALUI','add','obj_prefixlen','obj_prefixlen',1),('ADV',)).goto('OBJ.prefixloop')
    P('OBJ.prefixend').a(('INPOP',),('COPYW','obj_i','obj_rawlen')).goto('OBJ.shiftcheck')
    P('OBJ.shiftcheck').a(('CMPI','obj_i',0)).branch({1:'OBJ.copyhead'},'OBJ.shift')
    P('OBJ.shift').a(('ALUI','sub','obj_i','obj_i',1),('LDX','obj_c','obj_i',TAPE),('ALU','add','obj_ix','obj_i','obj_prefixlen'),('STX','obj_ix',TAPE,'obj_c')).goto('OBJ.shiftcheck')
    P('OBJ.copyhead').a(('LDI','obj_i',0)).goto('OBJ.headloop')
    P('OBJ.headloop').a(('CMP','obj_i','obj_prefixlen')).branch({0:'OBJ.headbyte'},'OBJ.write')
    P('OBJ.headbyte').a(('LDX','obj_c','obj_i',STRINGS+(1<<39)),('STX','obj_i',TAPE,'obj_c'),('ALUI','add','obj_i','obj_i',1)).goto('OBJ.headloop')
    P('OBJ.write').a(('ALU','add','eo_tapelen','obj_rawlen','obj_prefixlen'),('CMPI','obj_unit',0)).branch({1:'OBJ.notape'},'OBJ.emit')
    P('OBJ.notape').a(('LDI','eo_tapelen',0)).goto('OBJ.emit')
    P('OBJ.emit').call('EO.write').a(('ACCEPT',)).goto('DEAD')
    P('OBJ.fail').a(*E.rej('not covered: object metadata or symbol plan')).goto('DEAD')
    writer(E,dict(text=TEXT,data=DATA,strings=STRINGS,tape=TAPE,symbols=SYMBOLS,relocs=OBJ_RELOCS),arch)
