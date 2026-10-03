"""USBIND1 injected function declarations resolved by E3 delta.
Pure resource decoding, ABI matching and wrapper emission use existing actions.
No host source parser. Parent installs this after libraryexports, before finish.
Candidate origins are selected by modelcandidates. Structural wire checks run
before source parsing; support and ABI checks apply only to referenced externals.
"""
from pathlib import Path
import sys as _s,pathlib as _p
_s.path.insert(0,str(_p.Path(__file__).parents[1]/'facts'))
from load import facts as _facts
_t=lambda v:tuple(map(_t,v)) if isinstance(v,list) else v
globals().update((_r['name'],_t(_r['value'])) for _r in _facts('libraryimports'))

def install(E,P,b,start,integers):
    """Stage control lives in libraryimports-result.tsv / -byte.tsv (sections head, parse,
    prototype, argument, complete, structreturn, wrap, wrap2); fresh labels are declared in
    libraryimports-fresh.tsv. Python binds only dynamic facts: graph constants, the parent's
    tape codes (b, E.TK), the start label, and the integer-type list, whose per-type branch
    and states are created below because their keys and state names come from that list."""
    import json
    from finite_rules import install as rules
    import unresolved,librarycallables,libraryexports
    g=E.g;root=Path(__file__).parent
    import assemble
    assemble.run(assemble.FACTS.parent/'modelgraphequality-manifest.tsv',E,E.P,{},dict(fail='DEAD'))
    rules(g,root,'libraryimports',section='read')
    bindings=dict(start=start,STRIDE=STRIDE,PRINTDIGITS=186 << 40,TK_ID=E.TK_ID,TK_SEMI=E.TK[';'],
                  FPS_FIRST=b['FPS_FIRST'],SBB=b['SBB'])
    for mod,prefix in ((unresolved,'unresolved.'),(librarycallables,'librarycallables.'),(libraryexports,'libraryexports.')):
        for k,v in vars(mod).items():
            if type(v) is int and v>=1<<40:bindings[prefix+k]=v
    for k,v in globals().items():
        if type(v) is int and v>=1<<40:bindings[k]=v
    for k,v in b.items():
        if type(v) is int and v>=1<<40:bindings['b.'+k]=v
    for k in ('VAR','FND','FRD','FRB'):bindings['E.'+k]=getattr(E,k)
    classes=dict(FPB_FPV=[b['FPB'],b['FPV']],BOOL=[b['BOOL']])
    sequences={}
    for line in (root/'libraryimports-result.tsv').read_text().splitlines():
        if line.startswith('#') or line.count('\t')!=4:continue
        for action in json.loads(line.split('\t')[4]):
            if action[0]=='@' and action[1].startswith('O:'):sequences[action[1]]=[('OUT',c) for c in action[1][2:].encode()]
    fresh=[l.split('\t') for l in (root/'libraryimports-fresh.tsv').read_text().splitlines()[1:]]
    def section(name):
        # A fresh label keeps its owner's prefix (hooked parent states keep theirs).
        for part,key,kind in fresh:
            if part==name:
                owner=key.split('.')[0]
                bindings[key]=P('LI.fresh.'+key).fresh(kind).replace('LI.',owner+'.',1) if owner in ('S','CL','UD','FN') else P('LI.fresh.'+key).fresh(kind)
        rules(g,root,'libraryimports',bindings,sequences,classes,name)
    from finite_rules import install_template
    def hook(name,original):
        install_template(g,root,'libraryimports',{'hook':[{'name':name,'original':original}]},None,section='hook')
    section('head')
    assemble.run(assemble.FACTS.parent/'modelcandidates-manifest.tsv',E,E.P,{},dict(fail='DEAD'))
    section('parse')
    # A parsed typed native prototype is a real callable declaration; a forward
    # prototype needs the same stacked calling mode as its later definition.
    hook('FN.pr1','LI.original.prototype');section('prototype')
    # Fixed typed imports query the complete prototype pool for conversion.
    hook('CL.namedquery','LI.original.argumentquery');section('argument')
    # Every fixed typed native call supplies exactly the declared slots.
    hook('CL.done','LI.original.callcomplete');section('complete')
    # Aggregate native call expressions return their stable result-buffer address.
    hook('S.rs','LI.original.structreturn');section('structreturn')
    # UD has collected source labels; reconstruct the tape with wrapper definitions.
    hook('UD.calls0','LI.original.calls0');section('wrap')
    # Residue: keys and states come from the integer-type list (dynamic facts).
    install_template(g,root,'libraryimports',{'integer':[{'code':code,'width':width,'uns':uns} for _,code,width,uns,_ in integers]},
                     P('LI.scalar').fresh,section='scalar')
    for name in ('wrap2','scanbyte','scan','scanwordbyte','wrap3'):section(name)
    from librarydata import install as data_install
    data_install(E,P,b,integers)
    return 'LI.start'
