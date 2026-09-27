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
    # Target setup is a fixed declaration; layout and register facts remain bindings.
    setup_section = 'win-'+arch if os_=='win' else 'posix'
    setup_labels = {
        'posix': (('C', 'b'), ('C', 'r'), ('C', 'r')),
        'win-x86_64': (('C', 'r'), ('C', 'b'), ('C', 'r'), ('C', 'r'), ('C', 'r'), ('C', 'b')),
        'win-arm64': (('C', 'r'), ('C', 'b'), ('C', 'r'), ('C', 'r'), ('C', 'r'), ('C', 'r'), ('C', 'b')),
    }[setup_section]
    entry_bindings = {'label'+str(i): P(owner).fresh(kind)
                      for i, (owner, kind) in enumerate(setup_labels)}
    entry_bindings.update(WIN_HSTD=WIN_HSTD, WIN_ARGVA=WIN_ARGVA,
                          stack_end=WIN_EXTRA+WIN_STACK)
    entry_sequences = {name: E.O(text) for name, text in (
        ('winstdh', 'winstdh '), ('winargs', 'winargs '), ('argsave', 'argsave '),
        ('newline', '\n'), ('comma', ', '), ('spinit', 'spinit '+regmap['r7']),
        ('process_flags', ', '+('true' if os_=='lnx' else 'false')+'\n'))}
    install_rules(g, Path(__file__).parent, 'code-entry', bindings=entry_bindings,
                  sequences=entry_sequences, section=setup_section)
    if arch=='arm64':
        from armfuse import install as install_armfuse, immediate
        install_armfuse(E,ids,OP,KIND,ARG)
        immediate(E,ids,OP,KIND,ARG,TXT)
    dispatch_bindings = {'label'+str(i): P('C' if i==0 else 'CD').fresh('b')
                         for i in range(10)}
    dispatch_bindings.update({'id:'+name: value for name, value in ids.items()})
    install_rules(g, Path(__file__).parent, 'code-entry', bindings=dispatch_bindings,
                  section='dispatch-'+arch)
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
    # Prelude controls consume the existing dynamic ABI-id init sequence.
    prelude_bindings = {'label'+str(i): P('C').fresh(kind)
                        for i, kind in enumerate('rbrbrr')}
    prep_sequences = {'text'+str(i): E.O(text) for i, text in enumerate((
        '@argc ', '\n@argv ', '\n', 'setmem ', ', ', 'mov '+regmap['r0']+', ',
        ', '+regmap['r6']+'\nsetmem ', ', '+regmap['r7']+'\n', 'setreg '+regmap['r6']+', mem ',
        ' role=fp\n', 'setreg '+regmap['r7']+', mem ', ' role=sp\n', 'itoa ',
    ))}
    install_rules(g, Path(__file__).parent, 'code-sysprep', bindings=prelude_bindings,
                  sequences={**prep_sequences, 'init': p.acts}, section='prelude')
    # Source spills, return copying, and print preparation precede dynamic ABI rules.
    prep_bindings = {'label'+str(i): P('DO').fresh('r') for i in range(40)}
    prep_bindings.update({'id:'+name: value for name, value in ids.items()})
    prep_bindings.update(SYSFP=SYSFP, SYSSP=SYSSP)
    prep_bindings.update({'SYSA'+str(i): SYSA+8*i for i in range(6)})
    install_rules(g, Path(__file__).parent, 'code-sysprep', bindings=prep_bindings,
                  sequences=prep_sequences, section='prepare')
    # Finite source layouts are declarations; ABI register facts stay in current rows.
    source_rows = [line.split('\t') for line in
                   Path(__file__).with_name('code-abi-sources.tsv').read_text().splitlines()
                   if line and not line.startswith('#')]
    source_values = {'SYSA'+str(i): SYSA+8*i for i in range(6)}
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
            sources = [(kind, source_values[value] if value in source_values else int(value))
                       for m, shape, _, kind, value in source_rows
                       if int(m)==mode and shape in ('*', f[10])]
            if not sources:
                raise ValueError('new Linux '+arch+' argument shape requires migration: '+f[10])
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
