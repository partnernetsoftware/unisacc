"""USLSIG2 recursive layout serializer, executed as ordinary delta actions.
No host classification. Ordered member facts are the parser's actual layout.
"""
FRAME = 80 << 40

def install(E,P,b,integers,union_bank):
    regs=('d_depth','d_base','d_shape','d_array','d_arraybytes','d_class','d_width',
          'd_unsigned','d_align','d_tag','d_sid','d_members','d_i','d_key','d_old',
          'd_payload','d_blob','d_end','d_stride','d_return','d_dim','d_rank','d_fprank')
    assert len(regs)<64
    def frame(p,op):
        for i,r in enumerate(regs):
            p.a(('ALUI','mul','d_slot','lx_recursion',64),('ALUI','add','d_slot','d_slot',i))
            p.a((op,'d_slot',FRAME,r) if op=='STX' else (op,r,'d_slot',FRAME))
        return p
    def blob(p,r):
        return p.a(('INPUSH',r),('XLEN','d_end'),('SPAN2','lx_zero','d_end'),('INPOP',))
    def child(p):
        frame(p,'STX')
        p.call('LX.descriptor')
        return frame(p,'LDX')
    P('LX.descriptor').a(('ALUI','add','lx_recursion','lx_recursion',1),('ALUI','add','lx_nodes','lx_nodes',1)).branch(
        {2:'LX.fail'},'LTY.nodes',[('CMPI','lx_recursion',32)])
    P('LTY.nodes').branch({2:'LX.fail'},'LTY.start',[('CMPI','lx_nodes',16384)])
    P('LTY.start').a(('COPYW','d_depth','lx_depth'),('COPYW','d_base','lx_base'),
        ('COPYW','d_shape','lx_shape'),('COPYW','d_array','lx_array'),('COPYW','d_arraybytes','lx_arraybytes'),
        ('COPYW','d_return','lx_return'),('COPYW','d_dim','lx_arraydimension'),('COPYW','d_fprank','lx_rank'),('OCUT','d_old','lx_zero'),('LDI','d_class',6),
        ('LDI','d_width',0),('LDI','d_unsigned',0),('LDI','d_align',0),('LDI','d_tag',0)).branch(
        {1:'LTY.scalar'},'LTY.pointer',[('CMPI','d_depth',0)])
    P('LTY.pointer').branch({1:'LTY.fp'},'LTY.pointerupper',[('CMPI','d_base',b['FPB'])])
    P('LTY.pointerupper').branch({1:'LTY.fp'},'LTY.pointertyped',[('CMPI','d_base',b['FPV'])])
    P('LTY.pointertyped').branch({0:'LTY.ptr'},'LTY.pointertypedupper',[('CMPI','d_base',b['FPS_FIRST'])])
    P('LTY.pointertypedupper').branch({0:'LTY.fp'},'LTY.ptr',[('CMPI','d_base',b['SBB'])])
    P('LTY.ptr').a(('LDI','d_class',2),('LDI','d_width',8),('LDI','d_align',8)).goto('LTY.arraycheck')
    P('LTY.fp').branch({2:'LTY.ptr'},'LTY.fpclass',[('CMPI','d_depth',1)])
    P('LTY.fpclass').a(('LDI','d_class',4),('LDI','d_width',8),('LDI','d_align',8),('LDI','lx_supported',0)).branch({1:'LTY.fptyped'},'LTY.arraycheck',[('CMPI','d_depth',1)])
    P('LTY.fptyped').branch({0:'LTY.arraycheck'},'LTY.fptypedupper',[('CMPI','d_base',b['FPS_FIRST'])])
    P('LTY.fptypedupper').branch({0:'LTY.fpgraph'},'LTY.arraycheck',[('CMPI','d_base',b['SBB'])])
    P('LTY.fpgraph').a(('LDI','d_tag',4)).goto('LTY.arraycheck')
    P('LTY.scalar').branch({code:'LTY.int.'+str(code) for _,code,_,_,_ in integers},'LTY.other',[('RLD','d_base')])
    for _,code,size,uns,_ in integers:
        P('LTY.int.'+str(code)).a(('LDI','d_class',1),('LDI','d_width',size),('LDI','d_align',size),('LDI','d_unsigned',uns)).goto('LTY.arraycheck')
    P('LTY.other').branch({0:'LTY.void',b['BOOL']:'LTY.bool',b['DBL']:'LTY.double',b['FLT']:'LTY.float',b['FPB']:'LTY.fp',b['FPV']:'LTY.fp'},'LTY.composite',[('RLD','d_base')])
    P('LTY.void').a(('LDI','d_class',0)).branch({1:'LTY.arraycheck'},'LTY.unknown',[('CMPI','d_return',1)])
    P('LTY.bool').a(('LDI','d_class',1),('LDI','d_width',1),('LDI','d_align',1),('LDI','d_unsigned',1)).goto('LTY.arraycheck')
    P('LTY.double').a(('LDI','d_class',3),('LDI','d_width',8),('LDI','d_align',8)).goto('LTY.arraycheck')
    P('LTY.float').a(('LDI','d_class',3),('LDI','d_width',4),('LDI','d_align',4)).goto('LTY.arraycheck')
    P('LTY.composite').branch({0:'LTY.enumcheck'},'LTY.aggregate',[('CMPI','d_base',b['SBB'])])
    P('LTY.enumcheck').branch({0:'LTY.unknown'},'LTY.enumupper',[('CMPI','d_base',b['ENUM_FIRST'])])
    P('LTY.enumupper').branch({0:'LTY.enum'},'LTY.fp',[('CMPI','d_base',b['FPS_FIRST'])])
    isize=next(size for name,code,size,uns,narrow in integers if name=='i32')
    P('LTY.enum').a(('LDI','d_class',1),('LDI','d_width',isize),('LDI','d_align',isize)).goto('LTY.arraycheck')
    P('LTY.unknown').a(('LDI','d_class',6),('LDI','d_width',0),('LDI','d_align',0),('LDI','lx_supported',0)).goto('LTY.arraycheck')
    P('LTY.aggregate').a(('ALUI','sub','d_sid','d_base',b['SBB']),('LDI','d_class',5),
        ('LDX','d_width','d_sid',b['SSZ']),('LDX','d_align','d_sid',b['SAL']),
        ('LDX','d_members','d_sid',b['SMN']),('LDX','d_union','d_sid',union_bank),('LDI','d_tag',1)).branch(
        {1:'LTY.union'},'LTY.aggregatevalid',[('CMPI','d_union',1)])
    P('LTY.union').a(('LDI','d_tag',2),('LDI','lx_supported',0)).goto('LTY.aggregatevalid')
    P('LTY.aggregatevalid').branch({2:'LTY.aggregatemembers'},'LTY.unknown',[('CMPI','d_width',0)])
    P('LTY.aggregatemembers').branch({2:'LX.fail'},'LTY.arraycheck',[('CMPI','d_members',64)])
    P('LTY.arraycheck').branch({2:'LTY.array'},'LTY.flexcheck',[('CMPI','d_array',0)])
    P('LTY.flexcheck').branch({0:'LTY.flex'},'LTY.payload',[('CMPI','d_array',0)])
    P('LTY.flex').a(('LDI','d_class',6),('LDI','d_width',0),('LDI','d_align',0),('LDI','d_tag',0),('LDI','lx_supported',0)).goto('LTY.payload')
    P('LTY.array').a(('COPYW','d_stride','d_width'),('COPYW','d_width','d_arraybytes'),
        ('LDI','d_class',5),('LDI','d_tag',3)).goto('LTY.payload')
    P('LTY.payload').branch({1:'LTY.struct',2:'LTY.struct',3:'LTY.arraypayload',4:'LCG.payload'},'LTY.out',[('RLD','d_tag')])
    P('LTY.struct').a(('COPYW','lx_v','d_members')).call('LX.u64').a(('LDI','d_i',0)).goto('LTY.members')
    P('LTY.members').branch({0:'LTY.member'},'LTY.out',[('CMP','d_i','d_members')])
    p=P('LTY.member').a(('ALUI','mul','d_key','d_sid',64),('ALU','add','d_key','d_key','d_i'),
        ('LDX','d_key','d_key',b['SMEM']))
    for pool in ('MOF','BFO','BFW'):
        p.a(('LDX','lx_v','d_key',b[pool])).call('LX.u64')
    p.a(('LDX','d_bit','d_key',b['BFW'])).branch({2:'LTY.bitfield'},'LTY.ordinarymember',[('CMPI','d_bit',0)])
    P('LTY.bitfield').a(('LDI','lx_supported',0),('LDX','lx_v','d_key',b['MSZ'])).call('LX.u64').goto('LTY.membertype')
    P('LTY.ordinarymember').a(('LDX','lx_v','d_key',b['MSZ'])).call('LX.u64').goto('LTY.membertype')
    p=P('LTY.membertype')
    frame(p,'STX')
    p.a(('LDX','lx_depth','d_key',b['MPT']),('LDX','lx_base','d_key',b['MBS']),
        ('LDI','lx_shape',0),('LDX','lx_array','d_key',b['MAR']),('LDX','lx_arraybytes','d_key',b['MSZ']),('LDI','lx_arraydimension',0),('LDI','lx_return',0)).call('LX.descriptor')
    frame(p,'LDX').a(('ALUI','add','d_i','d_i',1)).goto('LTY.members')
    p=P('LTY.arraypayload').a(('COPYW','lx_v','d_array')).call('LX.u64').a(('COPYW','lx_v','d_stride')).call('LX.u64')
    frame(p,'STX')
    p.a(('COPYW','lx_depth','d_depth'),('COPYW','lx_base','d_base'),('COPYW','lx_shape','d_shape'),
        ('LDI','lx_array',0),('LDI','lx_arraybytes',0),('LDI','lx_arraydimension',0),('LDI','lx_return',0)).call('LX.descriptor')
    frame(p,'LDX').goto('LTY.out')
    from librarycallbackgraph import install as graph_install
    graph_install(E,P,b,frame)
    p=P('LTY.out').a(('OCUT','d_payload','lx_zero'))
    for r in ('d_depth','d_base','d_shape','d_class','d_width','d_unsigned','d_align'):
        p.a(('COPYW','lx_v',r)).call('LX.u64')
    p.a(('OUTW','d_tag'),('BLEN','lx_v','d_payload')).call('LX.u64')
    blob(p,'d_payload').a(('OCUT','d_blob','lx_zero'))
    blob(p,'d_old');blob(p,'d_blob').a(('ALUI','sub','lx_recursion','lx_recursion',1)).ret()

    from librarytypesv3 import install as v3_install
    v3_install(E,P,b,frame,blob)
