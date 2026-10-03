"""ELF image-writing delta, after code generation. No image-specific executor op.

The binary format's fields are a template here; runtime layout, data decoding,
relocation, zero-tail trimming and output are ordinary delta actions. Linux x86_64 and ARM64 share this writer; machine and label representation
are explicit generation parameters.
"""
from unisa.image import elf
DATA = 1 << 40  # byte offsets are bounded below 2^31; separate wide region


def install(E, byte, OFF, LABD, arch="x86_64", direct_labels=False, image_format="elf"):
    assert arch in elf.MACHINE
    P,g=E.P,E.g
    from pathlib import Path
    from finite_rules import install as install_rules, load as load_rules
    from unisa.image.pe import IMPORTS
    from pedelta import RELOCS
    image_bindings = dict(DATA=DATA, LABD=LABD, OFF=OFF, RELOCS=RELOCS,
                          format_os={'macho':2,'pe':3,'elf':1}[image_format],
                          writer={'macho':'MACHO','pe':'PE','elf':'EH.elfwrite'}[image_format],
                          iat_size=8*len(IMPORTS))
    image_sequences = {'zero_byte': byte(P('byte.binding'),0).acts,
                       'memory_magic': E.O('UNIMEM1\n'),
                       'reject': E.rej('not covered: '+{'macho':'Mach-O','pe':'PE','elf':'ELF'}[image_format]+' input or relocation')}
    labels = (('ELF_b0', 'ELF', 'b'), ('ED_end_b0', 'ED', 'b'),
              ('EM_len_r0', 'EM', 'r'), ('EM_len_r0_b0', 'EM', 'b'),
              ('EM_bss_b0', 'EM', 'b'), ('EM_extra_r0', 'EM', 'r'),
              ('EM_extra_r0_b0', 'EM', 'b'), ('EM_bound_b0', 'EM', 'b'),
              ('EM_end_b0', 'EM', 'b'), ('EM_ready_b0', 'EM', 'b'),
              ('ER_digitbound_b0', 'ER', 'b'), ('ER_bound_b0', 'ER', 'b'),
              ('ER_b2_b0', 'ER', 'b'), ('ER_read_b0', 'ER', 'b'),
              ('MI_args_b0', 'MI', 'b'), ('MI_argc_b0', 'MI', 'b'),
              ('MI_argc_read_r0', 'MI', 'r'), ('MI_argc_read_r0_b0', 'MI', 'b'),
              ('MI_argc_bound_b0', 'MI', 'b'), ('MI_argc_store_b0', 'MI', 'b'),
              ('MI_argv_b0', 'MI', 'b'), ('MI_argv_read_r0', 'MI', 'r'),
              ('MI_argv_read_r0_b0', 'MI', 'b'), ('MI_argv_bound_b0', 'MI', 'b'),
              ('MI_argv_store_b0', 'MI', 'b'), ('ET_loop_b0', 'ET', 'b'),
              ('ET_last_b0', 'ET', 'b'), ('EH_b0', 'EH', 'b'),
              ('EH_entry_b0', 'EH', 'b'), ('EH_write_b0', 'EH', 'b'),
              ('MI_begin_b0', 'MI', 'b'), ('MI_header_r0', 'MI', 'r'),
              ('MI_header_r0_r0', 'MI', 'r'), ('MI_header_r0_r0_r0', 'MI', 'r'),
              ('MI_header_r0_r0_r0_r0', 'MI', 'r'), ('MI_tail_b0', 'MI', 'b'),
              ('MI_align_b0', 'MI', 'b'))
    omitted = ({'EM_extra_r0_b0'} if image_format=='pe' else set()) | ({'EH_entry_b0'} if direct_labels else set())
    image_bindings.update({key: P(owner).fresh(kind) for key, owner, kind in labels if key not in omitted})
    for section in ('input-common', 'input-pe' if image_format=='pe' else 'input-nonpe',
                    'input-direct' if direct_labels else 'input-indirect'):
        install_rules(g, Path(__file__).parent, 'elfimage', bindings=image_bindings,
                      sequences=image_sequences, section=section)
    p=P('MI.iat')
    for i in range(len(IMPORTS)):p.a(('COPYW','lb_v','ml_imp_'+str(i)),('LDI','lb_n',8)).call('EI.bytes')
    install_rules(g, Path(__file__).parent, 'elfimage', section='iat-end',
                  bindings={'state': p.cur}, sequences={'pending': p.acts})
    p=P('MI.dlslots')
    for i in range(4):p.a(('COPYW','lb_v','ml_dl_'+str(i)),('LDI','lb_n',8)).call('EI.bytes')
    install_rules(g, Path(__file__).parent, 'elfimage', section='iat-end',
                  bindings={'state': p.cur}, sequences={'pending': p.acts})
    if image_format=='pe':
        from pedelta import install as install_pe
        install_pe(E,byte,arch)
    if image_format=='macho':
        from machodelta import install as install_macho
        install_macho(E,byte,arch)
    header = load_rules(Path(__file__).with_name('elfimage-result.tsv'), {},
                        bindings={'HDRS': elf.HDRS(arch), 'VADDR': elf.VADDR}, section='header-init')
    install_rules(g, Path(__file__).parent, 'elfimage', section='elfwrite',
                  bindings={'branch': P('EH.elfwrite').fresh('b')})
    def field(p,width,v):
        p.a(('LDI' if isinstance(v,int) else 'COPYW','lb_v',v),('LDI','lb_n',width)).call('EI.bytes')
    def preamble(p,phnum,hdr):
        for b in b'\x7fELF'+bytes([2,1,1,0])+bytes(8):byte(p,b)
        for w,v in [(2,2),(2,elf.MACHINE[arch]),(4,1),(8,'entryva'),(8,elf.EHDR),(8,0),
                    (4,0),(2,elf.EHDR),(2,elf.PHDR),(2,phnum),(2,0),(2,0),(2,0)]:field(p,w,v)
    def phdr(p,typ,flags,off,va,filesz,memsz,align):
        for w,v in [(4,typ),(4,flags),(8,off),(8,va),(8,va),(8,filesz),(8,memsz),(8,align)]:field(p,w,v)
    p=P('EH.elfwrite.static').a(header['actions'][0][1])
    preamble(p,2,elf.HDRS(arch))
    phdr(p,1,5,0,elf.VADDR,'tend','tend',elf.PAGE)
    phdr(p,1,6,'doff','data_va','stored','memlen',elf.PAGE)
    install_rules(g, Path(__file__).parent, 'elfimage', section='header-end',
                  bindings={'state': p.cur}, sequences={'pending': p.acts})
    p=P('EH.elfwrite.dyn').a(('A64','add','entryva','entryoff','text_va'),
                             ('A64I','add','tend','endo',784),
                             ('A64I','sub','doff','data_va',elf.VADDR+32),
                             ('A64I','add','dyn_filesz','stored',32),
                             ('A64I','add','dyn_memsz','memlen',32),
                             ('A64I','sub','dyn_slots','data_va',32))
    preamble(p,4,784)
    interp=b'/lib/ld-linux-aarch64.so.1' if arch=='arm64' else b'/lib64/ld-linux-x86-64.so.2'
    phdr(p,3,4,288,elf.VADDR+288,len(interp)+1,len(interp)+1,1)
    phdr(p,1,5,0,elf.VADDR,'tend','tend',elf.PAGE)
    phdr(p,1,6,'doff','dyn_slots','dyn_filesz','dyn_memsz',elf.PAGE)
    phdr(p,2,4,608,elf.VADDR+608,176,176,8)
    for b in interp+b'\0'+bytes(32-len(interp)-1):byte(p,b)
    for b in b'\0libc.so.6\0dlopen\0dlsym\0dlclose\0dlerror\0':byte(p,b)
    for _ in range(24):byte(p,0)
    for nameoff in (11,18,24,32):
        field(p,4,nameoff)
        for b in (0x12,0,0,0):byte(p,b)
        field(p,8,0);field(p,8,0)
    field(p,4,1);field(p,4,5)
    for _ in range(24):byte(p,0)
    for i in range(4):
        p.a(('A64I','add','dyn_reloc','dyn_slots',8*i))
        field(p,8,'dyn_reloc');field(p,8,((i+1)<<32)|(1025 if arch=='arm64' else 6));field(p,8,0)
    for tag,val in ((1,1),(4,elf.VADDR+480),(5,elf.VADDR+320),(6,elf.VADDR+360),
                    (10,40),(11,24),(7,elf.VADDR+512),(8,96),(9,24),(30,8),(0,0)):
        field(p,8,tag);field(p,8,val)
    install_rules(g, Path(__file__).parent, 'elfimage', section='header-end',
                  bindings={'state': p.cur}, sequences={'pending': p.acts})
    p=P('EI.dynslots')
    for _ in range(32):byte(p,0)
    install_rules(g, Path(__file__).parent, 'elfimage', section='iat-end',
                  bindings={'state': p.cur}, sequences={'pending': p.acts})
    labels = (('EI_pad_b0', 'EI', 'b'), ('EI_loop_b0', 'EI', 'b'),
              ('EI_bytes_b0', 'EI', 'b'))
    image_bindings.update({key: P(owner).fresh(kind) for key, owner, kind in labels})
    image_bindings['EI_loop_done'] = 'LIB.finish'   # librarysymbols-manifest validates, then returns
    install_rules(g, Path(__file__).parent, 'elfimage', bindings=image_bindings,
                  sequences=image_sequences, section='output-common')
    import assemble
    if direct_labels:
        from exec.facts.load import facts as _f
        SYM, PRESENT = (next(r['value'] for r in _f('enc-armlayout-bindings') if r['name'] == k) for k in ('SYM', 'PRESENT'))
    else:
        from exec.facts.load import facts as _f
        SYM, PRESENT = (next(r['value'] for r in _f('enc-address-bindings') if r['name'] == k) for k in ('SYM', 'PRESENT'))
    assemble.run(Path(__file__).with_name('librarysymbols-manifest.tsv'), E, P, dict(direct_labels=direct_labels),
                 dict(OFF=OFF, LABD=LABD, SYM=SYM, PRESENT=PRESENT))
