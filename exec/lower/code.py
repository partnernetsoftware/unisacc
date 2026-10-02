"""Instruction lowering delta. Hand control rules, declared facts.
ARM shares setup and syscalls, with its own transition peepholes.
regmap/enc/abi/reloc are read from TSV; no reference lower() is run by generator.
"""
from pathlib import Path
# Operand rows use eight slots each; large tapes must not overlap token text.
# Match the sparse 64-bit namespaces already used by the data-layout tables.
# Each [i<<40, (i+1)<<40) region has 2^40 slots. The executor has int-sized
# input: buffered rows fit 2^30 slots; their eight-wide ARG offsets fit 2^33.
# Interned tokens (TXT/REG/FORM) fit 2^31 slots; low PRN scratch and data.py
# regions 1..8 are disjoint. ARG index arithmetic uses A64/A64I in every reader.
OP, KIND, AC, ARG, TXT, REG, FORM, ARGREG = (i << 40 for i in range(105,113))


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
    p=P('C.init')
    words=list(dict.fromkeys(list(SHAPE)+['r'+str(i) for i in range(8)]+['0','1','2','3','4','8','-8','_start:','write','exit','.hostcall','.hostaddr','.librarycall','.libraryaddr']))
    ids={w:'idc'+str(i) for i,w in enumerate(words)}
    for w,d in ids.items():
        p.a(('SBCLR',),[('SBOUT',c) for c in w.encode()],('SBINTERN',d),('SBSAVE','blob'),('STX',d,TXT,'blob'))
        if w in SHAPE:
            for j, kind in enumerate(SHAPE[w]):
                if kind == 'r':
                    p.a(('ALUI','mul','argshapeidx',d,8),
                        ('ALUI','add','argshapeidx','argshapeidx',j),
                        ('LDI','argshape_reg',1),
                        ('STX','argshapeidx',ARGREG,'argshape_reg'))
        if w in regmap:
            p.a(('SBCLR',),[('SBOUT',c) for c in regmap[w].encode()],('SBSAVE','blob'),('STX',d,REG,'blob'))
        if w in enc:
            p.a(('SBCLR',),[('SBOUT',c) for c in (' form='+enc[w]).encode()],('SBSAVE','blob'),('STX',d,FORM,'blob'))
    from finite_rules import install as install_rules
    install_rules(g, Path(__file__).parent, 'code-shell',
                  sequences={'data': p.acts}, section='init')
    # Complete token/buffer traversal group; setup and dynamic dispatch follow below.
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
    # callm carries a tape-stack address, not an extra argument register.
    # Materialise its target in an ISA-reserved scratch, then reuse callr.
    scratch = 'x16' if arch == 'arm64' else 'r11'
    assert scratch not in set(regmap.values()), 'callm scratch aliases tape register'
    g.st['CM.original'] = g.st.pop('C.dispatch.base')
    g.labels.add('CM.original')
    P('C.dispatch.base').a(('CMP','op',ids['callm'])).branch({1:'CM.emit'},'CM.original')
    (P('CM.emit').o('load64 '+scratch+', ').a(('COPYW','tok','a0')).call('PRINT')
     .o(', ').a(('COPYW','tok','a1')).call('PRINT')
     .o(' form='+enc['load64']+'\ncallr '+scratch+' form='+enc['callr']+'\n')
     .goto('C.advance'))
    hostbindings={'id:'+name:value for name,value in ids.items()}
    if os_=='win':
        P('CH.rejectwin').a(E.rej('not covered: Windows .hostcall/.hostaddr forwarding')).goto('DEAD')
    hostbindings.update(test0=P('CH').fresh('b'),test1=P('CH').fresh('b'),
                        call='CH.call' if os_ in ('osx','lnx') else 'CH.rejectwin',
                        addr='CH.addr' if os_ in ('osx','lnx') else 'CH.rejectwin')
    install_rules(g,Path(__file__).parent,'code-host',section='entry',bindings=hostbindings)
    if os_ in ('osx','lnx'):
        hostbindings.update(test2=P('CH').fresh('b'),test3=P('CH').fresh('b'))
        hostbindings.update({'ret'+str(i):P('CH').fresh('r') for i in range(4)})
        install_rules(g,Path(__file__).parent,'code-host',section='supported',bindings=hostbindings,
                      sequences=dict(calltext=E.O('hostcall '),addrtext=E.O('hostaddr '),comma=E.O(', ')))
        for i in range(4):
            install_rules(g,Path(__file__).parent,'code-host',section='bound',bindings=dict(
                entry='CH.addr.'+str(i),test=P('CH').fresh('b'),value=ids[str(i)],
                next='CH.addr.'+str(i+1) if i<3 else 'C.fail'))
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
    print_bindings.update(OP=OP, KIND=KIND, ARG=ARG, TXT=TXT, REG=REG, FORM=FORM,
                          ARGREG=ARGREG)
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
    for op in dict.fromkeys([*abi, 'exit_group']):p.a(('SBCLR',),[('SBOUT',c) for c in op.encode()],('SBINTERN','sysid_'+op))
    from modelinput import u64
    u64(E,'C.runargc',b'\0process/argc','run_argc','run_mode','C.fail')
    u64(E,'C.runargv',b'\0process/argv','run_argv','run_hasargv','C.fail')
    # Prelude controls consume the existing dynamic ABI-id init sequence.
    prelude_bindings = {'label'+str(i): P('C').fresh(kind)
                        for i, kind in enumerate('rbrbrr')}
    prelude_bindings['entry'] = 'C.prelude.base'
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
    # Materialize current ABI rows; templates own dispatch, calls and field order.
    from finite_rules import load as load_rules
    root = Path(__file__).parent
    def actions(section, sequences):
        return load_rules(root/'code-syscall-actions.tsv', sequences,
                          section=section)['actions'][0][1]
    def put(section, bindings, sequences=None):
        install_rules(g, root, 'code-syscall', bindings=bindings,
                      sequences=sequences or {}, section=section)
    selected = [(op,f) for op,f in abi.items()
                if f[0]!='none' or (os_=='win' and WINAPI.get(op) is not None and f[12]!='none')]
    entry = 'SYSCALL.base'
    for op,f in selected:
        nxt = 'SC.next.'+op
        put('dispatch', dict(entry=entry, branch=P(entry).fresh('b'),
                            match='SC.'+op, next=nxt, sysid='sysid_'+op))
        entry = nxt
    put('finish', dict(entry=entry, next='C.fail'), {'pending': []})
    for op,f in selected:
        entry = 'SC.'+op
        sysno = (actions('sysno', {'reg': E.O(f[9]), 'value': E.O(str(int(f[0],0)))})
                 if f[0]!='none' and op != 'syscall' else [])
        pending = actions('head', {'retchars': [('SBOUT',c) for c in f[7].encode()], 'sysno': sysno})
        if op == 'syscall':
            # ADDR resolves the scratch-area base; a literal SYSA offset would
            # address the wrong cell once the target data section is laid out.
            resume = P(entry).fresh('r')
            put('nrmem', dict(entry=entry, resume=resume, SYSA=SYSA),
                {'pending': pending, 'reg': E.O(f[9])})
            entry, pending = resume, actions('nrtail', {})
        if os_=='win':
            resume = P(entry).fresh('r')
            put('winhead', dict(entry=entry, resume=resume, WIN_SAVE=WIN_SAVE), {'pending': pending})
            entry, pending = resume, E.O('\n')
        for mode in range(5):
            match, nxt = 'SC.'+op+'.m'+str(mode), 'SC.'+op+'.n'+str(mode)
            put('mode', dict(entry=entry, branch=P(entry).fresh('b'), match=match,
                             next=nxt, mode=mode), {'pending': pending})
            source_mode = 5 if op == 'syscall' and mode == 1 else mode
            sources = [(kind, source_values[value] if value in source_values else int(value))
                       for m, shape, kind, value in source_rows
                       if int(m)==source_mode and shape in ('*', f[10])]
            if not sources:
                raise ValueError('new Linux '+arch+' argument shape requires migration: '+f[10])
            current, pending = match, []
            for i,(kind,value) in enumerate(sources):
                if f[1+i]=='none':
                    if os_=='win': break
                    raise ValueError('unsupported stack syscall argument')
                facts = {'reg': E.O(f[1+i]), 'kind': E.O(kind),
                         'value': E.O(str(value)), 'index': E.O(str(i))}
                if kind=='imm':
                    pending += actions('immediate', facts)
                else:
                    resume = P(current).fresh('r')
                    put('arg', dict(entry=current, resume=resume, value=value),
                        dict(facts, pending=pending))
                    current, pending = resume, actions('argtail', facts)
            put('finish', dict(entry=current, next='SC.'+op+'.gate'), {'pending': pending})
            entry, pending = nxt, []
        put('finish', dict(entry=entry, next='C.fail'), {'pending': []})
        val = lambda v: "'"+v if v=='none' or v.startswith(tuple("0123456789")) else v
        facts = dict(form=enc[op], gate=f[8], carry='true' if os_=='osx' else 'false',
                     winapi='none' if WINAPI.get(op) is None else WINAPI[op], op=op, sysno=val(f[0]),
                     retconv=val(f[11]), winimp=val(f[12]), ret=f[7])
        labels = {'label'+str(i): P('SC').fresh('r') for i in range(5 if os_=='win' else 4)}
        put('gate-win' if os_=='win' else 'gate',
            dict(labels, entry='SC.'+op+'.gate', WIN_HSTD=WIN_HSTD,
                 WIN_WRITTEN=WIN_WRITTEN, WIN_SAVE=WIN_SAVE, zero=0, eight=8),
            {name: E.O(value) for name,value in facts.items()})
    from libraryexit import install as install_libraryexit
    install_libraryexit(E, os_, regmap, SYSA)
    from librarymodule import install as install_librarymodule
    install_librarymodule(E, regmap)
    from libraryimports import install as install_libraryimports
    install_libraryimports(E, os_, ids)
    from librarydata import install as install_librarydata
    install_librarydata(E, os_, ids)
    install_rules(g, Path(__file__).parent, 'code-shell',
                  sequences={'reject': E.rej('not covered: '+os_+'/'+arch+' lowering')}, section='exit')
