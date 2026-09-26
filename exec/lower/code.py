"""Instruction lowering delta. Hand control rules, declared facts.
ARM shares setup and syscalls, with its own transition peepholes.
regmap/enc/abi/reloc are read from TSV; no reference lower() is run by generator.
"""
from pathlib import Path
OP, KIND, AC, ARG, TXT, REG, FORM = (i*10**6 for i in range(105,112))


def rows(name):
    lines=Path('weights/gold/'+name+'.tsv').read_text().splitlines()
    return [s.split('\t') for s in lines if s and not s.startswith('#') and '=>' not in s]


def install(E, arch="x86_64", os_="lnx"):
    P,g=E.P,E.g
    from unisa.tape import SHAPE
    from unisa.catalog import WINAPI
    from unisa.lower import WIN_HSTD, WIN_WRITTEN, WIN_SAVE, WIN_ARGVA, WIN_EXTRA, WIN_STACK, SYSA, SYSFP, SYSSP
    regmap={r[0]:r[2] for r in rows('regmap') if r[1]==arch}
    enc={r[0]:r[3] for r in rows('enc') if r[1:3]==[os_,arch]}
    abi={r[0]:r[3:] for r in rows('abi') if r[1:3]==[os_,arch]}
    reloc={r[0]:r[2] for r in rows('reloc') if r[1]==arch}
    # C.init runs with the code blob active; headers have already been output.
    p=P('C.init');p.a(('LDI','nc',0),('LDI','entry',-1),('LDI','firstop',-1),('LDI','pending',0))
    words=list(dict.fromkeys(list(SHAPE)+['r'+str(i) for i in range(8)]+['0','1','2','4','8','-8','_start:','write','exit']))
    ids={w:'idc'+str(i) for i,w in enumerate(words)}
    for w,d in ids.items():
        p.a(('SBCLR',),[('SBOUT',c) for c in w.encode()],('SBINTERN',d),('SBSAVE','blob'),('STX',d,TXT,'blob'))
        if w in regmap:
            p.a(('SBCLR',),[('SBOUT',c) for c in regmap[w].encode()],('SBSAVE','blob'),('STX',d,REG,'blob'))
        if w in enc:
            p.a(('SBCLR',),[('SBOUT',c) for c in (' form='+enc[w]).encode()],('SBSAVE','blob'),('STX',d,FORM,'blob'))
    p.goto('C.line')
    g.on('C.line',[32,9,10],'C.line',[('ADV',)])
    g.on('C.line',[256],'C.parsed',[])
    g.els('C.line','C.word',[('MARK','ts'),('LDI','last',0)])
    g.on('C.word',[32,9,10,256],'C.we',[('MARK','te')])
    g.els('C.word','C.word',[('BYTE','last'),('ADV',)])
    P('C.we').call('TOKEN').a(('STX','nc',OP,'tok'),('LDI','na',0)).branch({1:'C.label'},'C.op',[('CMPI','last',58)])
    P('C.label').a(('LDI','t',1),('STX','nc',KIND,'t')).branch({1:'C.entlabel'},'C.next',[('CMP','tok',ids['_start:'])])
    P('C.entlabel').a(('LDI','pending',1)).goto('C.next')
    P('C.op').a(('LDI','t',2),('STX','nc',KIND,'t')).branch({1:'C.first'},'C.ent',[('CMPI','firstop',-1)])
    P('C.first').a(('COPYW','firstop','nc')).goto('C.ent')
    P('C.ent').branch({1:'C.setentry'},'C.args',[('CMPI','pending',1)])
    P('C.setentry').a(('COPYW','entry','nc'),('LDI','pending',0)).goto('C.args')
    g.on('C.args',[32,9,44,91,93,43],'C.args',[('ADV',)])
    g.on('C.args',[10,256],'C.next',[])
    g.on('C.args',[45],'C.arg',[('MARK','ts'),('ADV',)])
    g.els('C.args','C.arg',[('MARK','ts')])
    g.on('C.arg',[32,9,44,91,93,43,45,10,256],'C.ae',[('MARK','te')])
    g.els('C.arg','C.arg',[('ADV',)])
    P('C.ae').call('TOKEN').a(('ALUI','mul','ai','nc',8),('ALU','add','ai','ai','na'),('STX','ai',ARG,'tok'),('ALUI','add','na','na',1)).branch({2:'C.fail'},'C.args',[('CMPI','na',7)])
    P('C.next').a(('STX','nc',AC,'na'),('ALUI','add','nc','nc',1)).goto('C.line')
    P('TOKEN').a(('INTERN','tok','ts','te'),('BLOBSAVE','blob','ts','te'),('STX','tok',TXT,'blob')).ret()
    P('C.parsed').branch({1:'C.default'},'C.output',[('CMPI','entry',-1)])
    P('C.default').a(('COPYW','entry','firstop')).goto('C.output')
    P('C.output').a(('LDI','ci',0)).goto('C.loop')
    P('C.loop').branch({0:'C.load'},'C.done',[('CMP','ci','nc')])
    P('C.load').a(('LDX','op','ci',OP),('LDX','kind','ci',KIND),('LDX','na','ci',AC),('ALUI','mul','ai','ci',8)).goto('C.load.more')
    p=P('C.load.more')
    # Separate state after the load so arguments are stable across printing calls.
    for i in range(7):p.a(('ALUI','add','aj','ai',i),('LDX','a'+str(i),'aj',ARG))
    p.branch({1:'C.labprint'},'C.enter',[('CMPI','kind',1)])
    P('C.labprint').a(('COPYW','tok','op')).call('PRINT').o('\n').goto('C.advance')
    P('C.enter').branch({1:'C.setup'},'C.dispatch',[('CMP','ci','entry')])
    p=P('C.setup')
    if os_=='win':
        p.o('winstdh ').a(('LDI','offset',WIN_HSTD)).call('ADDR').o('\nwinargs ').a(('LDI','offset',48)).call('ADDR').o(', ').a(('LDI','offset',56)).call('ADDR').o(', ').a(('LDI','offset',WIN_ARGVA)).call('ADDR').o('\n')
    p.o('spinit '+regmap['r7'])
    if os_=='win' and arch=='arm64':p.o(', ').a(('LDI','offset',WIN_EXTRA+WIN_STACK)).call('ADDR')
    p.o('\n')
    if os_!='win':p.o('argsave ').a(('LDI','offset',48)).call('ADDR').o(', ').a(('LDI','offset',56)).call('ADDR').o(', '+('true' if os_=='lnx' else 'false')+'\n')
    p.goto('C.dispatch')
    special={'.arg':'arg','.argc':'argc','.argv':'argv','.exit':'exit','.write':'write','.sys':'sys','.sys6':'sys6','.print':'print','.frame':'frame','load64':'load'}
    if arch=='arm64':
        special['imm']='armimm'; special['.frame']='armframe'; special.pop('load64')
        from armfuse import install as install_armfuse, immediate
        install_armfuse(E,ids,OP,KIND,ARG)
        immediate(E,ids,OP,KIND,ARG,TXT)
    p=P('C.dispatch')
    for op,tag in special.items():
        p.branch({1:'DO.'+tag},'CD.'+tag,[('CMP','op',ids[op])]);p=P('CD.'+tag)
    p.goto('GENERIC')
    # Print a mapped register, otherwise the exact token.
    P('PRINT').a(('LDX','pb','tok',REG)).branch({1:'PRINT.raw'},'PRINT.go',[('CMPI','pb',0)])
    P('PRINT.raw').a(('LDX','pb','tok',TXT)).goto('PRINT.go')
    P('PRINT.go').a(('INPUSH','pb')).goto('PCOPY')
    g.on('PCOPY',[256],'RET',[('INPOP',)]);g.els('PCOPY','PCOPY',[('COPY',),('ADV',)])
    P('ADDR').a(('ALUI','add','n','base',256),('ALU','add','n','n','offset')).call('PRN').ret()
    p=P('GENERIC');p.a(('COPYW','tok','op')).call('PRINT').a(('LDI','argj',0)).label('G.args').branch({0:'G.arg'},'G.meta',[('CMP','argj','na')])
    P('G.arg').branch({1:'G.space'},'G.comma',[('CMPI','argj',0)])
    P('G.space').o(' ').goto('G.value');P('G.comma').o(', ').goto('G.value')
    P('G.value').a(('ALU','add','aj','ai','argj'),('LDX','tok','aj',ARG)).call('PRINT').a(('ALUI','add','argj','argj',1)).goto('G.args')
    P('G.meta').a(('LDX','pb','op',FORM)).branch({1:'G.reloc'},'G.form',[('CMPI','pb',0)])
    P('G.form').a(('INPUSH','pb')).call('PCOPY').goto('G.reloc')
    p=P('G.reloc')
    for op,kind in [('jump','jmp'),('jumpz','jz'),('call','call')]:
        p.branch({1:'GR.'+op},'GN.'+op,[('CMP','op',ids[op])]);P('GR.'+op).o(' reloc='+reloc[kind]).goto('C.nl');p=P('GN.'+op)
    p.goto('C.nl')
    P('C.nl').o('\n').goto('C.advance')
    P('C.advance').a(('ALUI','add','ci','ci',1)).goto('C.loop')
    # Adjacent x86 push/pop fusion; any intervening label prevents it.
    P('NEXTOP').a(('ALUI','add','next','ci',1),('LDI','fuse',0)).branch({0:'NX.load'},'RET',[('CMP','next','nc')])
    P('NX.load').a(('LDX','nk','next',KIND)).branch({1:'NX.op'},'RET',[('CMPI','nk',2)])
    p=P('NX.op');p.a(('LDI','fuse',1),('LDX','nop','next',OP),('ALUI','mul','ni','next',8))
    for i in range(3):p.a(('ALUI','add','nj','ni',i),('LDX','b'+str(i),'nj',ARG))
    p.ret()
    P('DO.frame').call('NEXTOP').branch({1:'PF.a'},'GENERIC',[('CMPI','fuse',1)])
    checks=[('PF.a','a0',ids['8']),('PF.b','nop',ids['store64']),('PF.c','b0',ids['r7']),('PF.d','b1',ids['0'])]
    for i,(st,r,v) in enumerate(checks):P(st).branch({1:checks[i+1][0] if i+1<len(checks) else 'PF.emit'},'GENERIC',[('CMP',r,v)])
    P('PF.emit').o('push ').a(('COPYW','tok','b2')).call('PRINT').a(('ALUI','add','ci','ci',1)).goto('C.nl')
    P('DO.load').call('NEXTOP').branch({1:'PL.a'},'GENERIC',[('CMPI','fuse',1)])
    checks=[('PL.a','a1',ids['r7']),('PL.b','a2',ids['0']),('PL.c','nop',ids['.frame']),('PL.d','b0',ids['-8'])]
    for i,(st,r,v) in enumerate(checks):P(st).branch({1:checks[i+1][0] if i+1<len(checks) else 'PL.emit'},'GENERIC',[('CMP',r,v)])
    P('PL.emit').o('pop ').a(('COPYW','tok','a0')).call('PRINT').a(('ALUI','add','ci','ci',1)).goto('C.nl')
    P('DO.argc').o('setreg ').a(('COPYW','tok','a0')).call('PRINT').o(', mem ').a(('LDI','offset',48)).call('ADDR').o(' role=argc').goto('C.nl')
    P('DO.argv').o('argvget ').a(('COPYW','tok','a0')).call('PRINT').o(', ').a(('COPYW','tok','a1')).call('PRINT').o(', ').a(('LDI','offset',56)).call('ADDR').goto('C.nl')
    p=P('DO.arg')
    # .arg number comes from a numeric token, not an intern id's magnitude.
    for i in range(8):
        # Populate these ids during init below through a separate init prelude.
        p.branch({1:'DA.'+str(i)},'DA.n'+str(i),[('CMP','a0','inum'+str(i))])
        P('DA.'+str(i)).o('mov '+regmap['r'+str(i)]+', ').a(('COPYW','tok','a1')).call('PRINT').goto('C.nl');p=P('DA.n'+str(i))
    p.goto('C.fail')
    # Wrapper around init, for the eight numeric spellings and syscall ids.
    p=P('C.prelude')
    for i in range(8):p.a(('SBCLR',),('SBOUT',48+i),('SBINTERN','inum'+str(i)))
    for op in abi:p.a(('SBCLR',),[('SBOUT',c) for c in op.encode()],('SBINTERN','sysid_'+op))
    p.goto('C.init')
    # Syscall source preparation: spill before overwriting ABI registers.
    def spill(p,token,offset):
        p.o('setmem ').a(('LDI','offset',offset)).call('ADDR').o(', ').a(('COPYW','tok',token)).call('PRINT').o('\n')
    for tag,n,shift in [('sys',3,1),('sys6',6,1),('write',2,0),('exit',1,0)]:
        p=P('DO.'+tag)
        for i in range(n):spill(p,'a'+str(i+shift),SYSA+8*i if tag=='sys6' else 8*i)
        if tag=='sys6':
            p.o('setmem ').a(('LDI','offset',SYSFP)).call('ADDR').o(', '+regmap['r6']+'\nsetmem ').a(('LDI','offset',SYSSP)).call('ADDR').o(', '+regmap['r7']+'\n')
        p.a(('LDI','syskind',{'sys':0,'sys6':1,'write':2,'exit':3}[tag]),('COPYW','sop','a0' if tag in ('sys','sys6') else ids['write' if tag=='write' else 'exit'])).call('SYSCALL')
        if tag in ('sys','sys6'):
            p.o('mov '+regmap['r0']+', ').a(('INPUSH','retblob')).call('PCOPY').o('\n')
        if tag=='sys6':
            for r,off,role in [('r6',SYSFP,'fp'),('r7',SYSSP,'sp')]:p.o('setreg '+regmap[r]+', mem ').a(('LDI','offset',off)).call('ADDR').o(' role='+role+'\n')
        p.goto('C.advance')
    P('DO.print').goto('C.fail')  # fallback integer printer needs itoa encoder.
    p=P('SYSCALL')
    for op,f in abi.items():
        if f[0]=='none' and os_!='win':continue
        p.branch({1:'SC.'+op},'SC.next.'+op,[('CMP','sop','sysid_'+op)]);p=P('SC.next.'+op)
    p.goto('C.fail')
    for op,f in abi.items():
        if f[0]=='none' and os_!='win':continue
        p=P('SC.'+op)
        p.a(('SBCLR',),[('SBOUT',c) for c in f[7].encode()],('SBSAVE','retblob'))
        if f[0]!='none':p.o('setreg '+f[9]+', imm '+str(int(f[0],0))+' role=sysno\n')
        if os_=='win':p.o('winsave ').a(('LDI','offset',WIN_SAVE)).call('ADDR').o('\n')
        for mode in range(4):
            p.branch({1:'SC.'+op+'.m'+str(mode)},'SC.'+op+'.n'+str(mode),[('CMPI','syskind',mode)])
            q=P('SC.'+op+'.m'+str(mode))
            if mode==0:
                if f[10] not in ('plain','atfd_1','atfd_1_zero','atfd_2_zero5'):
                    raise ValueError('new Linux '+arch+' argument shape requires migration: '+f[10])
                sources={'plain':[('mem',0),('mem',8),('mem',16)],
                         'atfd_1':[('imm',-100),('mem',0),('mem',8),('mem',16)],
                         'atfd_1_zero':[('imm',-100),('mem',0),('imm',0)],
                         'atfd_2_zero5':[('imm',-100),('mem',0),('imm',-100),('mem',8),('imm',0)]}[f[10]]
            elif mode==1:sources=[('mem',SYSA+i*8) for i in range(6)]
            elif mode==2:sources=[('imm',1),('mem',0),('mem',8)]
            else:sources=[('mem',0),('imm',0),('imm',0)]
            for i,(kind,value) in enumerate(sources):
                if f[1+i]=='none':
                    if os_=='win':break
                    raise ValueError('unsupported stack syscall argument')
                q.o('setreg '+f[1+i]+', '+kind+' ')
                if kind=='imm':q.o(str(value))
                else:q.a(('LDI','offset',value)).call('ADDR')
                q.o(' role=arg'+str(i)+'\n')
            q.goto('SC.'+op+'.gate');p=P('SC.'+op+'.n'+str(mode))
        p.goto('C.fail')
        def val(v):return "'"+v if v=='none' or v.startswith(('0','1','2','3','4','5','6','7','8','9')) else v
        p=P('SC.'+op+'.gate').o('gate form='+enc[op]+' gate='+f[8]+' carry='+('true' if os_=='osx' else 'false')+' winapi='+('none' if WINAPI.get(op) is None else WINAPI[op])+' catop='+op+' sysno='+val(f[0])+' retconv='+val(f[11])+' winimp='+val(f[12])+' ret='+f[7])
        for name,off in [('hstd',WIN_HSTD),('written',WIN_WRITTEN),('scr0',0),('scr1',8)]:p.o(' '+name+'=').a(('LDI','offset',off)).call('ADDR')
        p.o('\n')
        if os_=='win':p.o('winrest ').a(('LDI','offset',WIN_SAVE)).call('ADDR').o(', '+f[7]+'\n')
        p.ret()
    P('C.done').a(('INPOP',),('ACCEPT',)).goto('DEAD')
    g.on('C.fail',range(257),'DEAD',E.rej('not covered: '+os_+'/'+arch+' lowering'),'r')
