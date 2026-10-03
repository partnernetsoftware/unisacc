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
    assemble.run(Path(__file__).parent/'codeprep-manifest.tsv', E, P, {},
                 dict(target=target, idmap={'id:'+n: v for n, v in ids.items()}, **{'reg_'+k: v for k, v in regmap.items()}))
    assemble.run(Path(__file__).parent/'codeabi-manifest.tsv', E, P, dict(win=os_=='win'), dict(target=target))
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
