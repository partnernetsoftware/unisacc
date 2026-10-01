"""ARM ELF relocation capture via existing generic model actions.
Only the emitting pass records relocations. Sizing has identical word lengths.
"""
from armbranch import LABELS
from objectplan import OBJ_RELOCS
DATA_BASE = 1 << 32
UNDEF_BASE = 1 << 35


def install(E, word):
    g, P = E.g, E.P
    assert 'SKIPL' not in g.st
    P('SKIPL').goto('LINE')
    def replace(name):
        assert name in g.st, name
        old = 'AO.original.' + name
        assert old not in g.st
        g.st[old] = g.st.pop(name)
        g.labels.add(old)
        return P(name)

    replace('LAYOUT').a(('LDI','text_va',0),('LDI','data_va',DATA_BASE),
                        ('LDI','data_shift',DATA_BASE-256)).ret()
    # All object targets here are Linux. No host runtime cells exist in an object.
    replace('EMIT.60').goto('FAIL')
    replace('EMIT.61').goto('FAIL')
    replace('LEA.code').a(('COPYW','ao_name','ad_v'),('LDX','ad_v','ad_v',LABELS),
                           ('CMPI','ad_v',0)).branch({1:'AO.undefined'},'LEA.codeok')
    P('AO.undefined').a(('A64I','shl','ad_v','ao_name',20),
                        ('A64I','add','ad_v','ad_v',UNDEF_BASE)).call('ADRP').goto('LINE')

    replace('ADRP').a(('CMPI','pass',1)).branch({1:'AO.address'},'ADR.zero')
    P('AO.address').a(('OLEN','ao_off'),('LDI','ao_type',275)).call('AO.classify').call('AO.record').a(
        ('ALUI','add','ao_off','ao_off',4),('LDI','ao_type',277)).call('AO.record').goto('ADR.zero')
    P('AO.classify').a(('LDI','ao_limit',UNDEF_BASE),('C64U','ad_v','ao_limit')).branch({0:'AO.dataorcode'},'AO.undsection')
    P('AO.undsection').a(('A64I','sub','ao_value','ad_v',UNDEF_BASE),
                         ('A64I','shr','ao_section','ao_value',20),
                         ('ALUI','add','ao_section','ao_section',1000),
                         ('A64I','and','ao_add','ao_value',1048575)).ret()
    P('AO.dataorcode').a(('LDI','ao_limit',DATA_BASE),('C64U','ad_v','ao_limit')).branch({0:'AO.textsection'},'AO.datasection')
    P('AO.textsection').a(('LDI','ao_limit',DATA_BASE-32),('C64U','ad_v','ao_limit')).branch({0:'AO.textok'},'FAIL')
    P('AO.textok').a(('LDI','ao_section',1),('COPYW','ao_add','ad_v')).ret()
    P('AO.datasection').a(('A64I','sub','ao_add','ad_v',DATA_BASE),('C64U','ao_add','obj_nzend')).branch({0:'AO.dataok'},'AO.bss')
    P('AO.dataok').a(('LDI','ao_section',2)).ret()
    P('AO.bss').a(('LDI','ao_section',3),('A64','sub','ao_add','ao_add','obj_nzend')).ret()
    P('AO.record').a(('LDI','ao_limit',262144),('C64U','obj_nrel','ao_limit')).branch({0:'AO.save'},'FAIL')
    p=P('AO.save').a(('ALUI','mul','ao_ix','obj_nrel',4))
    for i,r in enumerate(('ao_off','ao_type','ao_section','ao_add')):
        p.a(('STX','ao_ix',OBJ_RELOCS,r))
        if i != 3:p.a(('ALUI','add','ao_ix','ao_ix',1))
    p.a(('ALUI','add','obj_nrel','obj_nrel',1)).ret()
    # Internal calls retain their section-relative BL field; only unresolved calls relocate.
    replace('BR.undef').a(('CMPI','br_call',1)).branch({1:'AO.callundef'},'FAIL')
    P('AO.callundef').a(('OLEN','ao_off'),('LDI','ao_type',283),
                       ('ALUI','add','ao_section','target',1000),('LDI','ao_add',0)).call('AO.record').a(('LDI','disp',0)).ret()
