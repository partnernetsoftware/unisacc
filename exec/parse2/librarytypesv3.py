"""Opt-in ordered USLSIG3 parser projections in ordinary delta actions.
The LF topology is declaration order; source modifier and semantic FP facts
remain explicitly unknown. V2 actions and wire remain the default path.
"""
def install(E,P,b,frame,blob):
    import layoutfacts as LF
    import gen2
    g=E.g
    def select(name,v3):
        old='L3.original.'+name
        assert old not in g.st
        g.st[old]=g.st.pop(name);g.labels.add(old)
        alias='L3.select.'+name
        P(alias).branch({3:v3},old,[('RLD','lx_wireversion')]);g.st[name]=g.st[alias]
    def word(p,r):return p.a(('COPYW','lx_v',r)).call('LX.u64')
    select('LTY.aggregatemembers','L3.aggregate')
    P('L3.aggregate').a(('LDX','d_seen','d_sid',LF.SEEN)).branch({1:'L3.aggregatecount'},'LX.fail',[('CMPI','d_seen',1)])
    P('L3.aggregatecount').a(('LDX','d_members','d_sid',LF.COUNT)).branch({2:'LX.fail'},'LTY.arraycheck',[('CMPI','d_members',LF.ENTRY_LIMIT)])
    select('LTY.struct','L3.struct')
    p=P('L3.struct');word(p,'d_members').a(('LDI','d_i',0)).goto('L3.members')
    P('L3.members').branch({0:'L3.member'},'LTY.out',[('CMP','d_i','d_members')])
    p=P('L3.member').a(('A64I','mul','d_key','d_sid',LF.ENTRY_STRIDE),('A64','add','d_key','d_key','d_i'))
    word(p,'d_i').a(('LDX','d_entrykind','d_key',LF.KIND),('OUTW','d_entrykind'))
    for bank in (LF.ALIGN,LF.OFFSET,LF.BITOFFSET,LF.BITWIDTH,LF.STORAGE):
        p.a(('LDX','lx_v','d_key',bank)).call('LX.u64')
    frame(p,'STX')
    p.a(('LDX','lx_depth','d_key',LF.DEPTH),('LDX','lx_base','d_key',LF.BASE),
        ('LDX','lx_shape','d_key',LF.SHAPE),('LDX','lx_array','d_key',LF.ARRAY),
        ('LDX','lx_arraybytes','d_key',LF.STORAGE),('LDI','lx_arraydimension',0),('LDI','lx_return',0)).call('LX.descriptor')
    frame(p,'LDX').a(('ALUI','add','d_i','d_i',1)).goto('L3.members')
    select('LTY.array','L3.array')
    # MAR is the product of dimensions. SHAPE owns rank and individual extents;
    # preserve aggregate element topology even when LF.CHILD is nonzero.
    P('L3.array').branch({1:'L3.flatarray'},'L3.shapearray',[('CMPI','d_shape',0)])
    P('L3.flatarray').a(('COPYW','d_stride','d_width'),('COPYW','d_width','d_arraybytes'),('LDI','d_rank',0),('LDI','d_class',5),('LDI','d_tag',3)).goto('LTY.payload')
    P('L3.shapearray').a(('LDX','d_rank','d_shape',E.ARR)).branch({2:'LX.fail'},'L3.shaperank',[('CMPI','d_rank',8)])
    P('L3.shaperank').branch({0:'L3.shapedim'},'LX.fail',[('CMP','d_dim','d_rank')])
    P('L3.shapedim').a(('A64I','mul','d_dimkey','d_shape',8),('A64','add','d_dimkey','d_dimkey','d_dim'),('LDX','d_array','d_dimkey',gen2.DIM)).branch({2:'L3.shapestride'},'LX.fail',[('CMPI','d_array',0)])
    P('L3.shapestride').a(('COPYW','d_width','d_arraybytes'),('ALU','div','d_stride','d_width','d_array'),('LDI','d_class',5),('LDI','d_tag',3)).goto('LTY.payload')
    select('LTY.arraypayload','L3.arraypayload')
    p=P('L3.arraypayload');word(p,'d_array');word(p,'d_stride')
    frame(p,'STX')
    p.a(('COPYW','lx_depth','d_depth'),('COPYW','lx_base','d_base'),('COPYW','lx_shape','d_shape'),('COPYW','lx_arraybytes','d_stride'),('ALUI','add','lx_arraydimension','d_dim',1),('LDI','lx_array',0),('LDI','lx_return',0)).branch({0:'L3.arraymore'},'L3.arraychild',[('CMP','lx_arraydimension','d_rank')])
    P('L3.arraymore').a(('LDI','lx_array',1)).goto('L3.arraychild')
    p=P('L3.arraychild').call('LX.descriptor');frame(p,'LDX').goto('LTY.out')
    select('LTY.out','L3.out')
    p=P('L3.out').a(('OCUT','d_payload','lx_zero'))
    for r in ('d_depth','d_base','d_shape','d_class','d_width','d_unsigned','d_align'):word(p,r)
    p.a(('OUTW','lx_zero'),('LDI','d_format',0)).branch({1:'L3.fpformat'},'L3.facts',[('CMPI','d_class',3)])
    P('L3.fpformat').branch({4:'L3.fp32',8:'L3.fp64'},'LX.fail',[('RLD','d_width')])
    P('L3.fp32').a(('LDI','d_format',1)).goto('L3.facts')
    P('L3.fp64').a(('LDI','d_format',2)).goto('L3.facts')
    p=P('L3.facts').a(('OUTW','d_format'),('LDI','lx_v',0)).call('LX.u64').a(('OUTW','lx_zero'),('OUTW','lx_zero'),('OUTW','lx_zero'),('OUTW','d_tag'),('BLEN','lx_v','d_payload')).call('LX.u64')
    blob(p,'d_payload').a(('OCUT','d_blob','lx_zero'));blob(p,'d_old');blob(p,'d_blob').a(('ALUI','sub','lx_recursion','lx_recursion',1)).ret()
