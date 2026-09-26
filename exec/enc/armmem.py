"""ARM memory packing rules as transitions. Opcode declarations read from seed;
scaled/unscaled selection and scratch-address fallback are hand rules here.
"""
from unisa.emit_arm import LDS, STS, LDU, STU, IP0


def install(E, word):
    P=E.P
    for cls,store,wide in ((9,False,True),(10,True,True),(11,False,False),(12,True,False)):
        p=P('EMIT.%d'%cls)
        p.a(('LDI','zeroing',0),('COPYW','mt','a2' if store else 'a0'),('COPYW','mb','a0' if store else 'a1'),
            ('COPYW','off','a1' if store else 'a2'),('LDI','store',int(store)))
        p.a(('LDI','width',8) if wide else ('COPYW','width','a3')).goto('MEM.width')
    P('MEM.width').a(('LDI','limit',8)).branch({2:'FAIL'},'MEM.width32',[('C64U','width','limit')])
    P('MEM.width32').branch({w:'MEM.w%d'%w for w in (1,2,4,8)},'FAIL',[('RLD','width')])
    for wd in (1,2,4,8):
        sh={1:0,2:1,4:2,8:3}[wd]
        P('MEM.w%d'%wd).branch({1:'MEM.s%d'%wd},'MEM.l%d'%wd,[('CMPI','store',1)])
        for st,tag in ((True,'s'),(False,'l')):
            P('MEM.%s%d'%(tag,wd)).a(('LDI','scaled',(STS if st else LDS)[wd]),
                ('LDI','unscaled',(STU if st else LDU)[wd]),('LDI','scale',sh)).goto('MEM.select')
    # Compare full 64-bit offsets before narrowing field values.
    P('MEM.select').a(('LDI','z',0)).branch({0:'MEM.unscaled'},'MEM.align',[('C64','off','z')])
    P('MEM.align').a(('ALUI','sub','mask','width',1),('A64','and','t','off','mask')).branch({1:'MEM.positive'},'MEM.unscaled',[('CMPI','t',0)])
    P('MEM.positive').a(('A64','shr','q','off','scale'),('LDI','limit',4096)).branch({0:'MEM.scaled'},'MEM.unscaled',[('C64','q','limit')])
    p=P('MEM.scaled').a(('ALUI','shl','w','q',10),('ALU','or','w','w','scaled'))
    p.goto('MEM.pack')
    P('MEM.unscaled').a(('LDI','limit',-256)).branch({0:'MEM.fallback'},'MEM.upper',[('C64','off','limit')])
    P('MEM.upper').a(('LDI','limit',255)).branch({2:'MEM.fallback'},'MEM.small',[('C64','off','limit')])
    P('MEM.small').a(('ALUI','and','w','off',511),('ALUI','shl','w','w',12),('ALU','or','w','w','unscaled')).goto('MEM.pack')
    # MOVIMM clobbers IP0. An aliasing base or store source would be destroyed;
    # these aren't normal tape registers, but reject explicit bad fixtures.
    P('MEM.fallback').branch({1:'FAIL'},'MEM.store',[('CMPI','mb',IP0)])
    P('MEM.store').branch({1:'MEM.source'},'MEM.abs',[('CMPI','store',1)])
    P('MEM.source').branch({1:'FAIL'},'MEM.abs',[('CMPI','mt',IP0)])
    P('MEM.abs').a(('COPYW','mv','off'),('LDI','addrbase',0x8B000000),('LDI','z',0)).branch({0:'MEM.negative'},'MEM.address',[('C64','off','z')])
    P('MEM.negative').a(('A64','sub','mv','z','off'),('LDI','addrbase',0xCB000000)).goto('MEM.address')
    p=P('MEM.address').a(('LDI','md',IP0)).call('MOVIMM')
    p.a(('ALUI','shl','w','mb',5),('ALUI','or','w','w',(IP0<<16)|IP0),('ALU','or','w','w','addrbase'))
    word(p).a(('LDI','mb',IP0),('COPYW','w','scaled')).goto('MEM.pack')
    p=P('MEM.pack').a(('ALUI','shl','t','mb',5),('ALU','or','w','w','t'),('ALU','or','w','w','mt'))
    word(p).branch({1:'ZERO.next'},'LINE',[('CMPI','zeroing',1)])
    # Store from XZR, splitting into the widest pieces that fit. Reuse exactly
    # the memory encoder above, including scratch-alias rejection on fallback.
    P('EMIT.32').a(('LDI','z',0)).branch({0:'FAIL'},'ZERO.init',[('C64','a2','z')])
    P('ZERO.init').a(('LDI','zeroing',1),('COPYW','zleft','a2'),('COPYW','zoff','a1')).goto('ZERO.loop')
    P('ZERO.loop').a(('LDI','z',0)).branch({1:'LINE'},'ZERO.width',[('C64','zleft','z')])
    P('ZERO.width').a(('LDI','width',8)).goto('ZERO.fit')
    P('ZERO.fit').branch({2:'ZERO.halve'},'ZERO.store',[('C64','width','zleft')])
    P('ZERO.halve').a(('ALUI','sar','width','width',1)).goto('ZERO.fit')
    P('ZERO.store').a(('LDI','store',1),('LDI','mt',31),('COPYW','mb','a0'),('COPYW','off','zoff')).goto('MEM.width')
    P('ZERO.next').a(('A64','sub','zleft','zleft','width'),('LDI','z',0)).branch({1:'LINE'},'ZERO.advance',[('C64','zleft','z')])
    P('ZERO.advance').a(('A64','add','zn','zoff','width')).branch({0:'FAIL'},'ZERO.save',[('C64','zn','zoff')])
    P('ZERO.save').a(('COPYW','zoff','zn')).goto('ZERO.loop')
