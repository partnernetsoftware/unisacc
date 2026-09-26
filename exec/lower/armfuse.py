"""ARM lowering peepholes compiled into ordinary executor transitions.
The four-instruction truncation rule is hand control, not a learned fact.
Labels occupy rows and therefore block every lookahead across a target.
"""
def install(E, ids, OP, KIND, ARG):
    P=E.P
    # .frame 8; .st [r7+0], X,W; .ld Y,[r7+0],W; .frame -8
    P('DO.armframe').branch({1:'AF.bounds'},'GENERIC',[('CMP','a0',ids['8'])])
    P('AF.bounds').a(('ALUI','add','fi','ci',3)).branch({0:'AF.load1'},'GENERIC',[('CMP','fi','nc')])
    for k in (1,2,3):
        p=P('AF.load'+str(k)).a(('ALUI','add','fi','ci',k),('LDX','fk','fi',KIND))
        p.branch({1:'AF.row'+str(k)},'GENERIC',[('CMPI','fk',2)])
        p=P('AF.row'+str(k)).a(('LDX','fo'+str(k),'fi',OP),('ALUI','mul','fj','fi',8))
        for j in range(4):p.a(('ALUI','add','fx','fj',j),('LDX','f'+str(k)+str(j),'fx',ARG))
        p.goto('AF.load'+str(k+1) if k<3 else 'AF.test0')
    checks=[('fo1',ids['.st']),('fo2',ids['.ld']),('fo3',ids['.frame']),
            ('f10',ids['r7']),('f11',ids['0']),('f21',ids['r7']),
            ('f22',ids['0']),('f30',ids['-8']),('f13','f23')]
    for i,(a,b) in enumerate(checks):
        P('AF.test'+str(i)).branch({1:'AF.test'+str(i+1) if i+1<len(checks) else 'AF.width0'},'GENERIC',[('CMP',a,b)])
    for i,w in enumerate(('1','2','4')):
        P('AF.width'+str(i)).branch({1:'AF.emit'},'AF.width'+str(i+1) if i<2 else 'GENERIC',[('CMP','f13',ids[w])])
    P('AF.emit').o('sext ').a(('COPYW','tok','f20')).call('PRINT').o(', ').a(('COPYW','tok','f12')).call('PRINT').o(', ').a(('COPYW','tok','f13')).call('PRINT').a(('ALUI','add','ci','ci',3)).goto('C.nl')


def immediate(E, ids, OP, KIND, ARG, TXT):
    P,g=E.P,E.g
    from unisa.tape import SHAPE
    # Parse only the range potentially useful to these fusions. Larger constants
    # retain the original imm/op pair; no input is rejected by this optimisation.
    P('DO.armimm').call('NEXTOP').branch({1:'AI.op0'},'GENERIC',[('CMPI','fuse',1)])
    for i,op in enumerate(('add64','sub64','mul64')):
        P('AI.op'+str(i)).branch({1:'AI.kind'+str(i)},'AI.op'+str(i+1) if i<2 else 'GENERIC',[('CMP','nop',ids[op])])
        P('AI.kind'+str(i)).a(('LDI','ik',i)).goto('AI.src')
    P('AI.src').branch({1:'AI.distinct'},'GENERIC',[('CMP','b2','a0')])
    P('AI.distinct').branch({1:'GENERIC'},'AI.parse',[('CMP','b1','a0')])
    P('AI.parse').a(('LDX','ib','a1',TXT),('INPUSH','ib'),('LDI','iv',0),('LDI','ineg',0),('LDI','digits',0),('LDI','imax',922337203685477580)).goto('AI.sign')
    g.on('AI.sign',[45],'AI.digit',[('LDI','ineg',1),('ADV',)])
    g.els('AI.sign','AI.digit',[])
    g.on('AI.digit',range(48,58),'AI.bound',[('BYTE','ib'),('ALUI','sub','ib','ib',48)])
    g.on('AI.digit',[256],'AI.end',[]);g.els('AI.digit','AI.skip',[])
    P('AI.bound').branch({2:'AI.skip',1:'AI.last'},'AI.acc',[('C64U','iv','imax')])
    P('AI.last').branch({2:'AI.skip'},'AI.acc',[('CMPI','ib',8)])
    P('AI.acc').a(('A64I','mul','iv','iv',10),('A64','add','iv','iv','ib'),('ALUI','add','digits','digits',1),('ADV',)).goto('AI.digit')
    P('AI.skip').a(('INPOP',)).goto('GENERIC')
    P('AI.end').a(('INPOP',)).branch({1:'GENERIC'},'AI.select',[('CMPI','digits',0)])
    P('AI.select').branch({1:'AI.mul'},'AI.add',[('CMPI','ik',2)])
    P('AI.mul').branch({1:'GENERIC'},'AI.power',[('CMPI','ineg',1)])
    P('AI.power').a(('LDI','zero',0)).branch({1:'GENERIC'},'AI.power1',[('C64U','iv','zero')])
    P('AI.power1').a(('A64I','sub','it','iv',1),('A64','and','it','it','iv')).branch({1:'AI.shift'},'GENERIC',[('C64U','it','zero')])
    P('AI.shift').a(('LDI','ival',0),('LDI','one',1),('LDI','iform',2)).goto('AI.shiftloop')
    P('AI.shiftloop').branch({1:'AI.live'},'AI.shiftnext',[('C64U','iv','one')])
    P('AI.shiftnext').a(('A64I','shr','iv','iv',1),('ALUI','add','ival','ival',1)).goto('AI.shiftloop')
    P('AI.add').a(('LDI','limit',4095)).branch({2:'GENERIC'},'AI.addsign',[('C64U','iv','limit')])
    P('AI.addsign').a(('COPYW','ival','iv'),('ALU','xor','iform','ineg','ik')).branch({1:'AI.addzero'},'AI.live',[('CMPI','ival',0)])
    P('AI.addzero').a(('LDI','iform',0)).goto('AI.live')
    P('AI.live').branch({1:'AI.emit'},'AI.scaninit',[('CMP','b0','a0')])
    P('AI.scaninit').a(('ALUI','add','si','ci',2),('LDI','sc',0)).goto('AI.scan')
    P('AI.scan').branch({0:'AI.count'},'GENERIC',[('CMP','si','nc')])
    P('AI.count').branch({0:'AI.row'},'GENERIC',[('CMPI','sc',32)])
    P('AI.row').a(('LDX','sk','si',KIND)).branch({1:'AI.shape'},'GENERIC',[('CMPI','sk',2)])
    p=P('AI.shape').a(('LDX','so','si',OP),('ALUI','mul','sj','si',8))
    shapes={('r','r','r'),('r','r'),('r','i'),('r','s'),('r','r','i'),('r','r','i','i')}
    for i,(op,shape) in enumerate((o,s) for o,s in SHAPE.items() if s in shapes and o!='.write'):
        p.branch({1:'AI.read'+str(i)},'AI.next'+str(i),[('CMP','so',ids[op])]);p=P('AI.next'+str(i))
        q=P('AI.read'+str(i))
        for j,t in enumerate(shape[1:],1):
            if t=='r':
                q.a(('ALUI','add','sx','sj',j),('LDX','sv','sx',ARG)).branch({1:'GENERIC'},'AI.read'+str(i)+'.'+str(j),[('CMP','sv','a0')]);q=P('AI.read'+str(i)+'.'+str(j))
        q.goto('AI.dest')
    p.goto('GENERIC')
    P('AI.dest').a(('LDX','sv','sj',ARG)).branch({1:'AI.emit'},'AI.cont',[('CMP','sv','a0')])
    P('AI.cont').a(('ALUI','add','si','si',1),('ALUI','add','sc','sc',1)).goto('AI.scan')
    P('AI.emit').branch({1:'AI.sub',2:'AI.lsl'},'AI.plus',[('RLD','iform')])
    for st,op in [('AI.sub','subi'),('AI.lsl','lsli'),('AI.plus','addi')]:P(st).o(op+' ').goto('AI.args')
    P('AI.args').a(('COPYW','tok','b0')).call('PRINT').o(', ').a(('COPYW','tok','b1')).call('PRINT').o(', ').a(('COPYW','n','ival')).call('PRN').a(('ALUI','add','ci','ci',1)).goto('C.nl')
