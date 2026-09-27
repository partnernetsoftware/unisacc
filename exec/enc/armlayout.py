"""ARM text/data address resolution. Header values are declarations, not answers.
Image format constants are read at generation; layout/ADRP arithmetic lives in
armlayout-*.tsv. This outputs text; retained data headers are not an image.
"""
from unisa.image import elf,macho,pe
from unisa.tape import DATA_BASE
from armbranch import LABELS
SYM,PRESENT,HSEEN=77000000,78000000,79000000


def init(p):
    p.a(('LDI','target_os',1),('SBCLR',),[('SBOUT',c) for c in b'_start'],('SBINTERN','id_entry'))
    for key in ('target','data','src_os','data_len','bss','relocs','argc','argv','sym'):
        p.a(('SBCLR',),[('SBOUT',c) for c in ('@'+key).encode()],('SBINTERN','h_'+key))
    for key,value in (('lnx','lnx/arm64'),('osx','osx/arm64'),('win','win/arm64')):
        p.a(('SBCLR',),[('SBOUT',c) for c in value.encode()],('SBINTERN','target_'+key))


def install(E,word):
    from pathlib import Path
    from finite_rules import install as install_rules
    from memorylayout import install as memory_layout
    from unisa.emit_arm import STD_FIRST
    bindings = dict(SYM=SYM, PRESENT=PRESENT, HSEEN=HSEEN, LABELS=LABELS, DATA_BASE=DATA_BASE)
    for name, image, base in (("lnx", elf, elf.VADDR), ("osx", macho, macho.VMADDR)):
        h, pg = image.HDRS("arm64"), image.PAGE
        bindings.update({name+"_text": base+h, name+"_align_add": h+pg-1,
                         name+"_mask": -pg, name+"_base": base})
    idata = 40+16*(len(pe.IMPORTS)+1)+sum((len(n.encode())+4)&-2 for n in pe.IMPORTS)+len(pe.DLL)+1
    idata = ((idata+7)&-8)+pe.LOADCFG
    pg, base = pe.SECT_ALIGN, pe.IMAGEBASE+pe.TEXT_RVA
    bindings.update(win_text=base, win_align_add=pg-1, win_mask=-pg,
                    win_imports=base+40+8*(len(pe.IMPORTS)+1), win_base=base+((idata+pg-1)&-pg),
                    GetStdHandle=pe.IMPORTS.index("GetStdHandle"))
    bindings.update(("std_word"+str(k), 0x92800000|(((~(STD_FIRST-k))&65535)<<5)) for k in range(3))
    sequences = {"word": word(E.P("word.binding")).acts}
    names = Path(__file__).with_name("armlayout-names.tsv").read_text().splitlines()
    for section in ("headers", "address"):
        if section == "address":
            memory_layout(E, "FAIL", "length")
        for line in names:
            if not line.startswith("#"):
                selected, name, prefix, kind = line.split("\t")
                if selected == section:
                    bindings[name] = E.P(prefix).fresh(kind)
        install_rules(E.g, Path(__file__).parent, "armlayout", bindings=bindings,
                      sequences=sequences, section=section)
