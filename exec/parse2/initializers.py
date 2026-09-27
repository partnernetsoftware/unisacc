"""Aggregate initializer delta procedures, shared by all storage classes.

The cursor is a scalar-slot index. Each brace level describes a subaggregate;
closing it advances to its end, while designators resolve in that level.
Member counts/offsets come from the parsed type layout, widths from ELSZ/STOREV.
This is the reference initaggr/slotat algorithm compiled into generic actions,
not a new executor primitive or a claim of complete C initializer semantics.
"""


def install(E, P, SBB, LOC, DIM, SSZ, SMN, SMEM, MOF, MSZ, MPT, MBS, MAR, SFLAT, MFLAT, MEMBER_STRIDE):
    ctx = ('ic_base', 'ic_slots', 'ic_st', 'ic_el', 'ic_elst', 'ic_row', 'ic_sub')
    root = ('ivt', 'ivb', 'iar', 'ivv', 'isl', 'ix', 'imode', 'inps', 'inpe', 'inlabel', 'ibytes')
    # Count scalar slots in a type. SBODY caches this for each member/aggregate.
    P('TYPECOUNT').a(('LDI','flat',1)).branch({1:'IC.type'},'RET',[('CMPI','td',0)])
    P('IC.type').branch({(1,2):'IC.struct'},'RET',[('CMPI','tb',SBB)])
    P('IC.struct').a(('ALUI','sub','t','tb',SBB),('LDX','flat','t',SFLAT)).branch({2:'RET'},'DEAD.linit',[('CMPI','flat',0)])
    # Find a struct member by relative scalar slot, or by its interned key.
    P('IMEMBER').a(('LDI','im_first',0),('LDI','im_n',0),('LDX','im_max','im_sid',SMN)).label('IM.loop')
    P('IM.loop').branch({0:'IM.at'},'DEAD.linit',[('CMP','im_n','im_max')])
    P('IM.at').a(('ALUI','mul','u','im_sid',64),('ALU','add','u','u','im_n'),('LDX','im_key','u',SMEM),('LDX','im_count','im_key',MFLAT)).branch({1:'IM.slot'},'IM.name',[('CMPI','im_named',0)])
    P('IM.slot').branch({0:'RET'},'IM.next',[('ALU','add','u','im_first','im_count'),('CMP','im_rel','u')])
    P('IM.name').branch({1:'RET'},'IM.next',[('CMP','im_key','im_want')])
    P('IM.next').a(('ALU','add','im_first','im_first','im_count'),('ALUI','add','im_n','im_n',1)).goto('IM.loop')
    # Root context: dimensions describe arrays; a struct describes its members.
    p=P('INITLIST').a(('LDX','ivt','ivv',E.PTR),('LDX','ivb','ivv',E.BASE),('LDX','iar','ivv',E.ARR),
        ('LDX','isl','ivv',LOC)).goto('INITVALUE')
    p=P('INITVALUE').a(('LDI','ix',0),('LDI','ioff',0),('COPYW','td','ivt'),('COPYW','tb','ivb'))
    p.branch({1:'IL.root'},'IL.arraytype',[('CMPI','iar',0)])
    P('IL.arraytype').a(('ALUI','sub','td','td',1)).goto('IL.root')
    p=P('IL.root').call('TYPECOUNT').a(('COPYW','ip_per','flat')).call('ELSZ').a(
        ('ALU','div','ic_slots','ibytes','es'),('ALU','mul','ic_slots','ic_slots','ip_per'),
        *[('LDI',v,0) for v in ctx if v!='ic_slots'])
    p.call('INITADDR').o('  .zero r1, 0, ').num('ibytes').o('\n').branch({1:'IL.rootst'},'IL.rootarr',[('CMPI','iar',0)])
    P('IL.rootst').branch({1:'IL.rootst1'},'IL.scalar',[('CMPI','ivt',0)])
    P('IL.rootst1').branch({(1,2):'IL.rootstruct'},'IL.scalar',[('CMPI','ivb',SBB)])
    P('IL.rootstruct').a(('ALUI','sub','ic_st','ivb',SBB)).goto('IL.level')
    P('IL.scalar').a(('LDI','ic_el',1)).goto('IL.level')
    P('IL.rootarr').a(('COPYW','ic_el','ip_per')).branch({1:'IL.arrst'},'IL.arrdims',[('CMPI','ivt',1)])
    P('IL.arrst').branch({(1,2):'IL.arrelem'},'IL.arrdims',[('CMPI','ivb',SBB)])
    P('IL.arrelem').a(('ALUI','sub','ic_elst','ivb',SBB)).goto('IL.arrdims')
    P('IL.arrdims').branch({2:'IL.rows'},'IL.level',[('CMPI','iar',1)])
    P('IL.rows').a(('ALUI','mul','u','ivv',8),('LDX','ic_row','u',DIM+1)).branch({1:'IL.row3'},'IL.rowend',[('CMPI','iar',3)])
    P('IL.row3').a(('LDX','ic_sub','u',DIM+2),('ALU','mul','ic_row','ic_row','ic_sub')).goto('IL.rowend')
    P('IL.rowend').a(('ALU','mul','ic_el','ic_el','ic_row')).goto('IL.level')
    P('IL.level').expect('{').call('NEXT').label('IL.loop').tok({'}':'IL.end',',':'IL.comma','{':'IL.nest','[':'IL.index','.':'IL.member'},'IL.value')
    P('IL.end').call('NEXT').ret()
    P('IL.comma').call('NEXT').goto('IL.loop')
    P('IL.nest').call('IL.bound').vpush(*ctx).call('IL.child').call('IL.level').a(('ALU','add','itail','ic_base','ic_slots')).vpop(*ctx).a(('COPYW','ix','itail')).goto('IL.loop')
    P('IL.bound').branch({0:'RET'},'DEAD.linit',[('ALU','add','t','ic_base','ic_slots'),('CMP','ix','t')])
    P('IL.index').branch({2:'IL.index1'},'DEAD.linit',[('CMPI','ic_el',0)])
    P('IL.index1').call('NEXT').call('CE').expect(']').call('NEXT').expect('=').a(('ALU','mul','ix','cv','ic_el'),('ALU','add','ix','ix','ic_base')).call('NEXT').goto('IL.loop')
    P('IL.member').branch({2:'IL.member1'},'DEAD.linit',[('CMPI','ic_st',0)])
    P('IL.member1').call('NEXT').tok({E.TK_ID:'IL.membername'},'DEAD.linit')
    P('IL.membername').a(('INTERN','t','ps','pe'),('A64I','mul','im_want','t',MEMBER_STRIDE),('A64','add','im_want','im_want','ic_st'),('COPYW','im_sid','ic_st'),('LDI','im_named',1)).call('IMEMBER').a(('ALU','add','ix','ic_base','im_first')).call('NEXT').expect('=').call('NEXT').goto('IL.loop')
    # A brace takes the current immediate subaggregate, even after flat scalars.
    p=P('IL.child').a(*[('COPYW','ip_'+v[3:],v) for v in ctx],*[('LDI',v,0) for v in ctx],('COPYW','ic_base','ix'),('LDI','ic_slots',1))
    p.branch({2:'IL.childst'},'IL.childarr',[('CMPI','ip_st',0)])
    P('IL.childst').a(('COPYW','im_sid','ip_st'),('ALU','sub','im_rel','ix','ip_base'),('LDI','im_named',0)).call('IMEMBER').a(
        ('ALU','add','ic_base','ip_base','im_first'),('COPYW','ic_slots','im_count'),('LDX','td','im_key',MPT),('LDX','tb','im_key',MBS)).branch({1:'IL.childtype'},'IL.childscalar',[('CMPI','td',0)])
    P('IL.childtype').branch({(1,2):'IL.childstruct'},'IL.childscalar',[('CMPI','tb',SBB)])
    P('IL.childstruct').a(('ALUI','sub','t','tb',SBB),('LDX','u','im_key',MAR)).branch({1:'IL.childone'},'IL.childmany',[('CMPI','u',0)])
    P('IL.childone').a(('COPYW','ic_st','t')).goto('IL.childdone')
    P('IL.childmany').a(('COPYW','ic_elst','t'),('LDX','ic_el','t',SFLAT)).goto('IL.childdone')
    P('IL.childscalar').branch({2:'IL.childelements'},'IL.childdone',[('CMPI','ic_slots',1)])
    P('IL.childelements').a(('LDI','ic_el',1)).goto('IL.childdone')
    P('IL.childarr').branch({2:'IL.childarr1'},'IL.childdone',[('CMPI','ip_el',0)])
    P('IL.childarr1').a(('ALU','sub','t','ix','ip_base'),('ALU','div','t','t','ip_el'),('ALU','mul','t','t','ip_el'),('ALU','add','ic_base','ip_base','t'),('COPYW','ic_slots','ip_el')).branch({2:'IL.childrow'},'IL.childelem',[('CMPI','ip_row',0)])
    P('IL.childrow').a(('ALU','div','ic_el','ip_el','ip_row'),('COPYW','ic_elst','ip_elst')).branch({2:'IL.childrow3'},'IL.childdone',[('CMPI','ip_sub',0)])
    P('IL.childrow3').a(('ALU','mul','ic_el','ic_el','ip_sub'),('COPYW','ic_row','ip_sub')).goto('IL.childdone')
    P('IL.childelem').a(('COPYW','ic_st','ip_elst')).goto('IL.childdone')
    P('IL.childdone').a(('COPYW','ix','ic_base')).ret()
    # Resolve a flat scalar slot to byte offset and type, descending member layouts.
    P('IL.value').call('IL.bound').a(('COPYW','il_i','ix'),('COPYW','td','ivt'),('COPYW','tb','ivb'),('LDI','ioff',0)).branch({1:'IL.slot'},'IL.slottype',[('CMPI','iar',0)])
    P('IL.slottype').a(('ALUI','sub','td','td',1)).goto('IL.slot')
    P('IL.slot').branch({1:'IL.slotbase'},'IL.slotscalar',[('CMPI','td',0)])
    P('IL.slotbase').branch({(1,2):'IL.slotst'},'IL.slotscalar',[('CMPI','tb',SBB)])
    P('IL.slotst').a(('ALUI','sub','im_sid','tb',SBB),('LDX','t','im_sid',SFLAT),('LDX','u','im_sid',SSZ),('ALU','div','w','il_i','t'),('ALU','mul','u','u','w'),('ALU','add','ioff','ioff','u'),('ALU','rem','il_i','il_i','t'),('COPYW','im_rel','il_i'),('LDI','im_named',0)).call('IMEMBER').a(
        ('ALU','sub','il_i','il_i','im_first'),('LDX','t','im_key',MOF),('ALU','add','ioff','ioff','t'),('LDX','td','im_key',MPT),('LDX','tb','im_key',MBS)).goto('IL.slot')
    saved=ctx+root+('ioff','vt','vb','bd','tb')
    P('IL.slotscalar').call('ELSZ').a(('ALU','mul','t','il_i','es'),('ALU','add','ioff','ioff','t'),('COPYW','vt','td'),('COPYW','vb','tb')).vpush(*saved).call('EXPR').a(('COPYW','rvt','vt'),('COPYW','rvb','vb')).vpop(*saved).call('ASSIGNCV').call('INITADDR').call('STOREV').a(('ALUI','add','ix','ix',1)).tok({',':'IL.comma','}':'IL.end'},'DEAD.linit')
    E.g.on('DEAD.linit',range(257),'DEAD',E.rej('not covered: this initialiser'),'r')
