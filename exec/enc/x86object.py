"""ELF object address/undefined-call relocation capture, as delta actions."""
RELOCS = 250 << 40

def install(E, byte, OFF, LABD, KND, TGT, SYM, PRESENT):
    P,g=E.P,E.g
    def move(state):
        other='XO.original.'+state;assert other not in g.st
        g.st[other]=g.st.pop(state);g.labels.add(other);return other
    # Object addresses are section offsets, never executable-image virtual VAs.
    move('LAYOUT')
    P('LAYOUT').a(('LDI','text_va',0),('LDI','data_va',1<<32),('LDI','data_shift',(1<<32)-256)).ret()
    label=move('AD.label')
    P('AD.label').a(('LDX','obj_lab','ad_v',LABD)).branch({0:'XO.undefinedaddr'},label,[('RLD','obj_lab')])
    P('XO.undefinedaddr').a(('COPYW','obj_symid','ad_v'),('LDI','ad_v',0)).goto('AD.emit')
    rip=move('RIP')
    P('RIP').branch({0:'XO.ripvalue'},'XO.ripsymbol',[('RLD','obj_symid')])
    P('XO.ripsymbol').a(('ALUI','add','obj_relsect','obj_symid',1000),('LDI','obj_reladd',-4)).goto('XO.riprecord')
    P('XO.ripvalue').a(('LDI','obj_data_base',1<<32),('C64U','ad_v','obj_data_base')).branch({(1,2):'XO.ripdata'},'XO.ripbelow')
    P('XO.ripbelow').a(('LDI','obj_hostbase',(1<<32)-32),('C64U','ad_v','obj_hostbase')).branch({0:rip},('rej','not covered: object host FFI slot'))
    P('XO.ripdata').a(('A64I','sub','obj_reladd','ad_v',1<<32),('C64U','obj_reladd','obj_nzend')).branch({0:'XO.data'},'XO.bss')
    P('XO.data').a(('LDI','obj_relsect',2)).goto('XO.ripadd')
    P('XO.bss').a(('LDI','obj_relsect',3),('A64','sub','obj_reladd','obj_reladd','obj_nzend')).goto('XO.ripadd')
    P('XO.ripadd').a(('A64I','sub','obj_reladd','obj_reladd',4)).goto('XO.riprecord')
    P('XO.riprecord').a(('OLEN','obj_reloff'),('A64I','add','obj_reloff','obj_reloff',3),('LDI','obj_reltype',2)).call('XO.record').a(('OLEN','ad_v'),('A64I','add','ad_v','ad_v',7),('LDI','obj_symid',0)).goto(rip)
    call=move('WR.c')
    P('WR.c').a(('LDX','obj_callee','q',TGT),('LDX','obj_lab','obj_callee',LABD)).branch({0:'XO.call'},call,[('RLD','obj_lab')])
    P('XO.call').a(('OLEN','obj_reloff'),('A64I','add','obj_reloff','obj_reloff',1),('LDI','obj_reltype',4),('ALUI','add','obj_relsect','obj_callee',1000),('LDI','obj_reladd',-4)).call('XO.record').a(('OUT',232),('OUT',0),('OUT',0),('OUT',0),('OUT',0)).goto('WR.nx')
    P('XO.record').a(('CMPI','obj_nrel',262144)).branch({0:'XO.store'},('rej','not covered: object relocation capacity'))
    p=P('XO.store').a(('A64I','mul','obj_relindex','obj_nrel',4))
    for i,value in enumerate(['obj_reloff','obj_reltype','obj_relsect','obj_reladd']):
        p.a(('A64I','add','obj_relcell','obj_relindex',i),('STX','obj_relcell',RELOCS,value))
    p.a(('ALUI','add','obj_nrel','obj_nrel',1)).ret()
