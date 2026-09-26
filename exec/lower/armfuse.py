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
