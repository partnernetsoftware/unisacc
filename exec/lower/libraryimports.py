"""Declared USBIND1/2/3 capability gates dedicated library calls in lowering.
The shared decoder validates fixed format1 and variadic-template format2
TypeGraphs/dispatcher handles as capabilities;
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
    # Callable-only modules have no import binding table. Their explicit host
    # resources authorise the generic dispatcher, never named data resolution.
    from modelinput import u64
    u64(E,'LBI.callableflag',b'\0library/callables','lbi_callableflag','lbi_callablepresent','LBI.fail')
    u64(E,'LBI.callablemake',b'\0library/callablemake','lbi_callablemake','lbi_makepresent','LBI.fail')
    u64(E,'LBI.callablecall',b'\0library/callablecall','lbi_callablecall','lbi_callpresent','LBI.fail')
    P('LBI.cached').branch({1:'LBI.emit'},'LBI.bindingpresence',[('CMPI','lbi_validated',1)])
    P('LBI.bindingpresence').a(('SBCLR',),*[('SBOUT',x) for x in b'\0library/bindings'],('SBFIND','lbi_probe')).branch({1:'LBI.callableonly'},'LBI.read',[('CMPI','lbi_probe',0)])
    P('LBI.callableonly').branch({1:'LBI.callablecap'},'LBI.fail',[('CMP','op',ids['.librarycall'])])
    P('LBI.callablecap').call('LBI.callableflag').branch({1:'LBI.callableaddresses'},'LBI.fail',[('CMPI','lbi_callableflag',1)])
    P('LBI.callableaddresses').call('LBI.callablemake').call('LBI.callablecall').branch({1:'LBI.fail'},'LBI.callablecallcheck',[('CMPI','lbi_callablemake',0)])
    P('LBI.callablecallcheck').branch({1:'LBI.fail'},'LBI.emit',[('CMPI','lbi_callablecall',0)])
