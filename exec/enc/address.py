"""Deferred x86 RIP-relative encoding; layout runs after branch relaxation.

No reference layout or encoder is called at runtime. Format constants are read
at generation; page rounding, symbol resolution and displacements are delta
operations. This slice outputs text only; header data is not an image yet.
"""
from unisa.image import elf, macho, pe
from unisa.tape import DATA_BASE
SYM, PRESENT, DEST, VALUE, NAMED, OPCODE, VALUE2 = (i * 10**6 for i in range(85, 92))


def install(E, byte, KND, SZ, OFF, LABD):
    import json
    from pathlib import Path
    from finite_rules import install as install_rules
    from memorylayout import install as memory_layout
    root = Path(__file__).parent
    sequences = {name: [tuple(a) for a in json.loads(actions)] for name, actions in
                 (line.split('\t') for line in (root/'address-sequences.tsv').read_text().splitlines() if not line.startswith('#'))}
    sequences.update({'byte'+value: byte(E.P('byte.binding'), int(value)).acts for value in
                      (root/'address-bytes.tsv').read_text().splitlines() if not value.startswith('#')})
    sequences['reject'] = E.rej('not covered: address or target declaration')
    facts = dict(SYM=SYM, PRESENT=PRESENT, DEST=DEST, VALUE=VALUE, NAMED=NAMED,
                 OPCODE=OPCODE, VALUE2=VALUE2, KND=KND, SZ=SZ, OFF=OFF, LABD=LABD, DATA_BASE=DATA_BASE)
    idata = 40+16*(len(pe.IMPORTS)+1)+sum((len(n.encode())+4)&-2 for n in pe.IMPORTS)+len(pe.DLL)+1
    idata = ((idata+7)&-8)+pe.LOADCFG
    layouts = {tag: dict(text=base+m.HDRS('x86_64'), round=m.HDRS('x86_64')+m.PAGE-1, mask=-m.PAGE, base=base)
               for tag, m, base in [('lnx',elf,elf.VADDR), ('osx',macho,macho.VMADDR)]}
    layouts['win'] = dict(text=pe.IMAGEBASE+pe.TEXT_RVA, round=pe.SECT_ALIGN-1, mask=-pe.SECT_ALIGN,
        imports=pe.IMAGEBASE+pe.TEXT_RVA+40+8*(len(pe.IMPORTS)+1),
        base=pe.IMAGEBASE+pe.TEXT_RVA+((idata+pe.SECT_ALIGN-1)&-pe.SECT_ALIGN))
    for phase in ('pre', 'post'):
        if phase == 'post': memory_layout(E, 'DEAD.addr')
        for line in (root/'address-instances.tsv').read_text().splitlines():
            if line.startswith('#'): continue
            selected, section, values, names, prepare = line.split('\t')
            if selected != phase: continue
            bindings = {**facts, **json.loads(values)}
            if section == 'posix': bindings.update(layouts[bindings['platform']])
            if section == 'windows': bindings.update(layouts['win'])
            bindings.update({name: E.P(owner).fresh(kind) for name, owner, kind in json.loads(names)})
            install_rules(E.g, root, 'address', section=section, bindings=bindings,
                          sequences={**sequences, 'prepare': sequences[prepare]})
