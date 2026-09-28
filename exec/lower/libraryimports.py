"""Declared USBIND1/2/3 capability gates dedicated library calls in lowering.
The shared decoder validates format1 TypeGraphs/dispatcher handles as capabilities;
E3 chooses and types wrappers. Lower never classifies a source type or picks an import.
Ordinary .hostcall handling is untouched; only the dedicated token uses this route.
"""
from pathlib import Path
NAMES, IDS, ADDRESS, ARGC, DESC, SEEN = (i << 40 for i in range(190,196))

def install(E, os_, ids):
    from finite_rules import install as rules
    from code import REG
    P=E.P
    g=E.g
    from modelbindings import install as install_decoder
    install_decoder(E)
    g.st['LBI.original.dispatch']=g.st.pop('C.dispatch');g.labels.add('LBI.original.dispatch')
    P('C.dispatch').branch({1:'LBI.capability'},'LBI.original.dispatch',[('CMP','op',ids['.librarycall'])])
    P('LBI.capability').branch({1:'LBI.arity' if os_ in ('osx','lnx','win') else 'LBI.fail'},'LBI.fail',[('CMPI','library_module',1)])
    P('LBI.arity').branch({1:'LBI.reg0'},'LBI.fail',[('CMPI','na',2)])
    for i,nx in [(0,'LBI.reg1'),(1,'LBI.cached')]:
        P('LBI.reg'+str(i)).branch({1:'LBI.fail'},nx,[('LDX','lbi_reg','a'+str(i),REG),('CMPI','lbi_reg',0)])
    # Legacy descriptor-validation states are retained as constructor helpers;
    # the structural decoder no longer invokes them globally. E3 proves named
    # function ABI matches; raw .librarycall is trusted typed tape capability.
    p=P('LBI.checkdesc')
    for field,reg in [(0,'depth'),(3,'class'),(4,'width'),(5,'uns')]:
        p.a(('ALUI','add','lbi_t','lbi_index',field-5),('LDX','lbi_'+reg,'lbi_t',DESC))
    p.branch({2:'LBI.fail'},'LBI.depthbound',[('LDI','lbi_max',1024),('C64U','lbi_depth','lbi_max')])
    P('LBI.depthbound').branch({2:'LBI.fail'},'LBI.classbound',[('LDI','lbi_max',2),('C64U','lbi_class','lbi_max')])
    P('LBI.classbound').branch({2:'LBI.fail'},'LBI.widthbound',[('LDI','lbi_max',8),('C64U','lbi_width','lbi_max')])
    P('LBI.widthbound').branch({2:'LBI.fail'},'LBI.classdispatch',[('LDI','lbi_max',1),('C64U','lbi_uns','lbi_max')])
    P('LBI.classdispatch').branch({0:'LBI.void',1:'LBI.integer',2:'LBI.pointer'},'LBI.fail',[('RLD','lbi_class')])
    P('LBI.void').branch({1:'LBI.fail'},'LBI.voidresult',[('CMPI','lbi_kind',1)])
    P('LBI.voidresult').branch({1:'LBI.voiddepth'},'LBI.fail',[('CMPI','lbi_d',0)])
    P('LBI.voiddepth').branch({1:'LBI.voidwidth'},'LBI.fail',[('CMPI','lbi_depth',0)])
    P('LBI.voidwidth').branch({1:'LBI.unszero'},'LBI.fail',[('CMPI','lbi_width',0)])
    P('LBI.integer').branch({1:'LBI.integerwidth'},'LBI.fail',[('CMPI','lbi_depth',0)])
    P('LBI.integerwidth').branch({w:'LBI.integeruns' for w in (1,2,4,8)},'LBI.fail',[('RLD','lbi_width')])
    P('LBI.integeruns').branch({0:'LBI.descnext',1:'LBI.descnext'},'LBI.fail',[('RLD','lbi_uns')])
    P('LBI.pointer').branch({2:'LBI.pointerwidth'},'LBI.fail',[('CMPI','lbi_depth',0)])
    P('LBI.pointerwidth').branch({1:'LBI.unszero'},'LBI.fail',[('CMPI','lbi_width',8)])
    P('LBI.unszero').branch({1:'LBI.descnext'},'LBI.fail',[('CMPI','lbi_uns',0)])
    P('LBI.emit').o('hostcall ').a(('COPYW','tok','a0')).call('PRINT').o(', ').a(('COPYW','tok','a1')).call('PRINT').goto('C.nl')
    P('LBI.cached').branch({1:'LBI.emit'},'LBI.read',[('CMPI','lbi_validated',1)])
