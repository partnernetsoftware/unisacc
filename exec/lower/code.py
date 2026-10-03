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
    import assemble
    target = os_ + '/' + arch
    ids = {r['word']: 'idc' + str(r['i']) for r in assemble.load_facts('lower-code')['words'] if r['target'] == target}
    assemble.run(Path(__file__).parent/'codeinit-manifest.tsv', E, P, dict(win=os_=='win', arm64=arch=='arm64'),
                 dict(target=target, ids=ids, process_flag='true' if os_=='lnx' else 'false',
                      **{'reg_'+k: v for k, v in regmap.items()}))
    from finite_rules import install as install_rules
    assemble.run(Path(__file__).parent/'armfuse-manifest.tsv', E, P, dict(arm64=arch=='arm64'),
                 dict(ids=ids, idmapu={'id_'+n: v for n, v in ids.items()}))
    scratch = 'x16' if arch == 'arm64' else 'r11'
    assert scratch not in set(regmap.values()), 'callm scratch aliases tape register'
    assemble.run(Path(__file__).parent/'codedispatch-manifest.tsv', E, P, dict(win=os_=='win'),
                 dict(arch=arch, ids=ids, idmap={'id:'+n: v for n, v in ids.items()}, scratch=scratch,
                      form_load64=enc['load64'], form_callr=enc['callr'],
                      **{'reloc_'+k: v for k, v in reloc.items()}, **{'reg_'+k: v for k, v in regmap.items()}))
    # Wrapper around init, for the eight numeric spellings and syscall ids.
    p=P('C.prelude')
    for i in range(8):p.a(('SBCLR',),('SBOUT',48+i),('SBINTERN','inum'+str(i)))
    for op in dict.fromkeys([*abi, 'exit_group']):p.a(('SBCLR',),[('SBOUT',c) for c in op.encode()],('SBINTERN','sysid_'+op))
    assemble.run(Path(__file__).parent/'codeprep-manifest.tsv', E, P, {},
                 dict(prelude_init=p.acts, idmap={'id:'+n: v for n, v in ids.items()}, **{'reg_'+k: v for k, v in regmap.items()}))
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
    from unisa.emit_x86 import SCR
    from unisa.emit_arm import IP0
    import assemble
    scratch='x'+str(IP0) if regmap['r0'].startswith('x') else SCR
    assert scratch not in regmap.values(), 'library zero scratch aliases tape register'
    from unisa.emit_arm import IP1
    from unisa.emit_x86 import SCR2
    s0,s1=('x'+str(IP0),'x'+str(IP1)) if regmap['r0'].startswith('x') else (SCR,SCR2)
    assert not {s0,s1}&set(regmap.values()), 'process scratches alias tape values'
    hosted=dict(hosted=os_ in ('osx','lnx','win'))
    assemble.run(Path(__file__).parent/'codelib-manifest.tsv', E, P, hosted,
                 dict(scratch=scratch, s0=s0, s1=s1, **{'reg_'+k: v for k, v in regmap.items()}))
    from modelbindings import install as install_decoder
    from modelbindings import IDS, ADDRESS, DESC, KIND as BKIND, SUPPORTED, WRITABLE, EXTENT, STRIDE
    install_decoder(E)
    assemble.run(Path(__file__).parent/'codeend-manifest.tsv', E, P, hosted,
                 dict(ids=ids, target=target, IDS=IDS, ADDRESS=ADDRESS, DESC=DESC, BKIND=BKIND, SUPPORTED=SUPPORTED,
                      WRITABLE=WRITABLE, EXTENT=EXTENT, STRIDE=STRIDE))
