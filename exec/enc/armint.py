"""Integer instruction layouts and tape frame adjustment as transition rules."""
SPECS={'.div':('rrr',16),'.udiv':('rrr',17),'.mod':('rrr',18),'.umod':('rrr',19),
       'sext':('rri',20),'addi':('rri',21),'subi':('rri',22),'lsli':('rri',23),'.frame':('i',24)}


def install(E,word):
    P=E.P
    for cls in (16,17,18,19):
        p=P('EMIT.%d'%cls)
        if cls>=18:
            p.branch({1:'FAIL'},'DIV.src%d'%cls,[('CMPI','a1',17)])
            p=P('DIV.src%d'%cls);p.branch({1:'FAIL'},'DIV.emit%d'%cls,[('CMPI','a2',17)])
            p=P('DIV.emit%d'%cls)
        p.a(('ALUI','shl','w','a2',16),('ALUI','shl','t','a1',5),('ALU','or','w','w','t'),
            ('ALUI','or','w','w',0x9AC00C00 if cls in (16,18) else 0x9AC00800))
        p.a(('ALUI','or','w','w',17) if cls>=18 else ('ALU','or','w','w','a0'))
        word(p)
        if cls>=18:
            p.a(('ALUI','shl','w','a2',16),('ALUI','shl','t','a1',10),('ALU','or','w','w','t'),
                ('ALUI','or','w','w',0x9B008000|(17<<5)),('ALU','or','w','w','a0'))
            word(p)
        p.goto('LINE')
    P('EMIT.20').a(('LDI','limit',8)).branch({2:'FAIL'},'SEXT.width',[('C64U','a2','limit')])
    P('SEXT.width').branch({1:'SEXT',2:'SEXT',4:'SEXT',8:'SEXT'},'FAIL',[('RLD','a2')])
    p=P('SEXT').a(('ALUI','mul','w','a2',8),('ALUI','sub','w','w',1),('ALUI','shl','w','w',10),
        ('ALUI','or','w','w',0x93400000),('ALUI','shl','t','a1',5),('ALU','or','w','w','t'),('ALU','or','w','w','a0'))
    word(p).goto('LINE')
    for cls in (21,22,23):
        P('EMIT.%d'%cls).a(('LDI','limit',63 if cls==23 else 4095)).branch({2:'FAIL'},'IMMOP.%d'%cls,[('C64U','a2','limit')])
        p=P('IMMOP.%d'%cls)
        if cls==23:
            p.a(('LDI','w',64),('ALU','sub','w','w','a2'),('ALUI','and','w','w',63),('ALUI','shl','w','w',16),
                ('LDI','t',63),('ALU','sub','t','t','a2'),('ALUI','shl','t','t',10),('ALU','or','w','w','t'),('ALUI','or','w','w',0xD3400000))
        else: p.a(('ALUI','shl','w','a2',10),('ALUI','or','w','w',0x91000000 if cls==21 else 0xD1000000))
        p.a(('ALUI','shl','t','a1',5),('ALU','or','w','w','t'),('ALU','or','w','w','a0'));word(p).goto('LINE')
    P('EMIT.24').a(('COPYW','mv','a0'),('LDI','zero',0),('LDI','fb',0xD1000000),('LDI','fr',0xCB000000)).branch({0:'FRAME.neg'},'FRAME.size',[('C64','mv','zero')])
    P('FRAME.neg').a(('A64','sub','mv','zero','mv'),('LDI','fb',0x91000000),('LDI','fr',0x8B000000)).goto('FRAME.size')
    P('FRAME.size').a(('LDI','limit',4096)).branch({0:'FRAME.small'},'FRAME.large',[('C64U','mv','limit')])
    word(P('FRAME.small').a(('ALUI','shl','w','mv',10),('ALU','or','w','w','fb'),('ALUI','or','w','w',(7<<5)|7))).goto('LINE')
    p=P('FRAME.large').a(('LDI','md',16)).call('MOVIMM').a(('ALUI','or','w','fr',(16<<16)|(7<<5)|7))
    word(p).goto('LINE')
