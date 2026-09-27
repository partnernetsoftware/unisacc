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
    # Complete token/buffer traversal group; setup and dynamic dispatch follow below.
    from finite_rules import install as install_rules
    scan_labels = (('C', 'r'), ('C', 'b'), ('C', 'b'), ('C', 'b'),
                   ('C', 'b'), ('C', 'r'), ('C', 'b'), ('C', 'b'),
                   ('C', 'b'), ('C', 'b'), ('C', 'r'), ('C', 'b'))
    scan_bindings = {'label'+str(i): P(owner).fresh(kind)
                     for i, (owner, kind) in enumerate(scan_labels)}
    scan_bindings.update(OP=OP, KIND=KIND, AC=AC, ARG=ARG, TXT=TXT,
                         start_id=ids['_start:'])
    install_rules(g, Path(__file__).parent, 'code-scan', bindings=scan_bindings,
                  sequences={'newline': E.O('\n')})
    p=P('C.setup')
    if os_=='win':
        p.o('winstdh ').a(('LDI','offset',WIN_HSTD)).call('ADDR').o('\n')
        p.branch({1:'C.winargs'},'C.spinit',[('CMPI','run_mode',0)])
        p=P('C.winargs').o('winargs ').a(('LDI','offset',48)).call('ADDR').o(', ').a(('LDI','offset',56)).call('ADDR').o(', ').a(('LDI','offset',WIN_ARGVA)).call('ADDR').o('\n').goto('C.spinit')
        p=P('C.spinit')
    p.o('spinit '+regmap['r7'])
    if os_=='win' and arch=='arm64':p.o(', ').a(('LDI','offset',WIN_EXTRA+WIN_STACK)).call('ADDR')
    p.o('\n')
    p.branch({1:'C.processargs'},'C.dispatch',[('CMPI','run_mode',0)])
    p=P('C.processargs')
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
    # Generic output and fixed fusion/argument control; facts stay in their original maps.
    print_labels = (('PRINT', 'b'), ('ADDR', 'r'), ('GENERIC', 'r'), ('G', 'b'),
                    ('G', 'b'), ('G', 'r'), ('G', 'b'), ('G', 'r'),
                    ('G', 'b'), ('GN', 'b'), ('GN', 'b'), ('NEXTOP', 'b'),
                    ('NX', 'b'), ('DO', 'r'), ('DO', 'b'), ('PF', 'b'),
                    ('PF', 'b'), ('PF', 'b'), ('PF', 'b'), ('PF', 'r'),
                    ('DO', 'r'), ('DO', 'b'), ('PL', 'b'), ('PL', 'b'),
                    ('PL', 'b'), ('PL', 'b'), ('PL', 'r'), ('DO', 'r'),
                    ('DO', 'r'), ('DO', 'r'), ('DO', 'r'), ('DO', 'r'),
                    ('DO', 'b'), ('DA', 'r'), ('DA', 'b'), ('DA', 'r'),
                    ('DA', 'b'), ('DA', 'r'), ('DA', 'b'), ('DA', 'r'),
                    ('DA', 'b'), ('DA', 'r'), ('DA', 'b'), ('DA', 'r'),
                    ('DA', 'b'), ('DA', 'r'), ('DA', 'b'), ('DA', 'r'))
    print_bindings = {'label'+str(i): P(owner).fresh(kind)
                      for i, (owner, kind) in enumerate(print_labels)}
    print_bindings.update(OP=OP, KIND=KIND, ARG=ARG, TXT=TXT, REG=REG, FORM=FORM)
    print_bindings.update({'id:'+name: value for name, value in ids.items()})
    print_sequences = {'text'+str(i): E.O(text) for i, text in enumerate((
        ' ', ', ', ' reloc='+reloc['jmp'], ' reloc='+reloc['jz'], ' reloc='+reloc['call'], '\n',
        'push ', 'pop ', 'setreg ', ', mem ', ' role=argc', 'argvget ', ', ', ', ',
    ) + tuple('mov '+regmap['r'+str(i)]+', ' for i in range(8)))}
    install_rules(g, Path(__file__).parent, 'code-print', bindings=print_bindings,
                  sequences=print_sequences)
    # Wrapper around init, for the eight numeric spellings and syscall ids.
    p=P('C.prelude')
    for i in range(8):p.a(('SBCLR',),('SBOUT',48+i),('SBINTERN','inum'+str(i)))
    for op in abi:p.a(('SBCLR',),[('SBOUT',c) for c in op.encode()],('SBINTERN','sysid_'+op))
    from modelinput import u64
    u64(E,'C.runargc',b'\0process/argc','run_argc','run_mode','C.fail')
    u64(E,'C.runargv',b'\0process/argv','run_argv','run_hasargv','C.fail')
    p.call('C.runargc').branch({1:'C.init'},'C.argv',[('CMPI','run_mode',0)])
    P('C.argv').call('C.runargv').branch({1:'C.fail'},'C.runheaders',[('CMPI','run_hasargv',0)])
    # Data cells are initialised by the image model, just as bk_run does;
    # there are no extra instructions in the compiled entry sequence.
    P('C.runheaders').o('@argc ').a(('LDI','offset',48)).call('ADDR').o('\n@argv ').a(('LDI','offset',56)).call('ADDR').o('\n').goto('C.init')
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
    p=P('DO.print');spill(p,'a0',0)
    p.o('itoa ')
    for i,off in enumerate((0,24,16)):
        if i:p.o(', ')
        p.a(('LDI','offset',off)).call('ADDR')
    p.o('\n').a(('LDI','syskind',4),('COPYW','sop',ids['write'])).call('SYSCALL').goto('C.advance')
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
        for mode in range(5):
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
            elif mode==3:sources=[('mem',0),('imm',0),('imm',0)]
            else:sources=[('imm',1),('addr',24),('mem',16)]
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
