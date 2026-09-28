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
    p.goto('EI.data')
    if image_format=='pe':
        from pedelta import install as install_pe
        install_pe(E,byte,arch)
    if image_format=='macho':
        from machodelta import install as install_macho
        install_macho(E,byte,arch)
    header = load_rules(Path(__file__).with_name('elfimage-result.tsv'), {},
                        bindings={'HDRS': elf.HDRS(arch), 'VADDR': elf.VADDR}, section='header-init')
    p=P('EH.elfwrite').a(header['actions'][0][1])
    for b in b'\x7fELF'+bytes([2,1,1,0])+bytes(8):byte(p,b)
    def field(width,v):
        p.a(('LDI' if isinstance(v,int) else 'COPYW','lb_v',v),('LDI','lb_n',width)).call('EI.bytes')
    for w,v in [(2,2),(2,elf.MACHINE[arch]),(4,1),(8,'entryva'),(8,elf.EHDR),(8,0),(4,0),(2,elf.EHDR),(2,elf.PHDR),(2,elf.NPH),(2,0),(2,0),(2,0)]:field(w,v)
    for flags,offset,va,fs,ms in [(5,0,elf.VADDR,'tend','tend'),(6,'doff','data_va','stored','memlen')]:
        for w,v in [(4,1),(4,flags),(8,offset),(8,va),(8,va),(8,fs),(8,ms),(8,elf.PAGE)]:field(w,v)
    install_rules(g, Path(__file__).parent, 'elfimage', section='header-end',
                  bindings={'state': p.cur}, sequences={'pending': p.acts})
    labels = (('EI_pad_b0', 'EI', 'b'), ('EI_loop_b0', 'EI', 'b'),
              ('EI_bytes_b0', 'EI', 'b'))
    image_bindings.update({key: P(owner).fresh(kind) for key, owner, kind in labels})
    install_rules(g, Path(__file__).parent, 'elfimage', bindings=image_bindings,
                  sequences=image_sequences, section='output-common')
    from librarysymbols import install as install_librarysymbols
    install_librarysymbols(E, OFF, LABD, direct_labels, image_bindings['EI_loop_b0'])
