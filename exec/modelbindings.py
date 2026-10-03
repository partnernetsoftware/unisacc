"""Finite USBIND1/2/3 resource wire decoder, reusable by stage builders.
No source parsing or host-side ABI selection. Structural validation precedes decoding; named data facts are checked on use.
Function typing is proved by E3; raw .librarycall is an explicitly trusted tape capability.
"""
from pathlib import Path
import sys as _sys, pathlib as _pl
_root=str(_pl.Path(__file__).resolve().parents[1])
if _root not in _sys.path: _sys.path.append(_root)
from exec.facts.load import facts
# Graph-table banks and wire constants: exec/facts/top-modelbindings-{banks,const}.tsv.
globals().update({r['name']:r['bank']<<40 for r in facts('top-modelbindings-banks')})
_C={r['name']:r['value'] for r in facts('top-modelbindings-const')}
STRIDE=_C['STRIDE']

def install(E):
    """Stage control lives in modelbindings-result.tsv (sections read, head, present, header,
    names, record). Python binds only dynamic facts: the graph-table constants, the resource
    path bytes, and fresh labels pre-allocated in original order (modelbindings-fresh.tsv).
    The USBIND magic chain, the identifier byte class and the two record kinds are
    fact-parameterised rows in modelbindings-template.tsv; install_candidates (MC.*) stays."""
    import json
    from finite_rules import install as rules,install_template
    P,g=E.P,E.g
    root=Path(__file__).parent
    fresh=[l.split('\t') for l in (root/'modelbindings-fresh.tsv').read_text().splitlines()[1:]]
    bindings=dict(NAMES=NAMES,IDS=IDS,ADDRESS=ADDRESS,ARGC=ARGC,DESC=DESC,SEEN=SEEN,KIND=KIND,
                  EXTENT=EXTENT,WRITABLE=WRITABLE,VARIADIC=VARIADIC,SUPPORTED=SUPPORTED,FORMAT=FORMAT,
                  TYPED=TYPED,CANON=CANON,CANONLEN=CANONLEN,DISPATCH=DISPATCH,PLAN=PLAN,
                  RESULT_KIND=RESULT_KIND,RESULT_WIDTH=RESULT_WIDTH)
    sequences={'librarypath':[('SBOUT',x) for x in _C['librarypath'].encode('latin-1')]}
    def section(name):
        p=P('LBI.fresh')
        for part,key,kind in fresh:
            if part==name:bindings[key]=p.fresh(kind)
        rules(g,root,'modelbindings',bindings,sequences,None,name)
    letters,digits=_C['letters'].encode(),_C['digits'].encode()
    tfacts=dict(magic=[dict(i=i,c=c,next=i+1) for i,c in enumerate(_C['magic'].encode())],
               id=[dict(letters=list(letters),digits=list(digits),all=list(letters+digits))],kind=_C['kind'])
    def template(name):
        install_template(g,root,'modelbindings',tfacts,P('LBI.fresh').fresh,bindings,None,None,name)
    rules(g,root,'modelbindings',section='read')
    section('head')
    from modelcandidates import install as install_candidates
    install_candidates(E)
    section('present')
    template('present')
    section('header')
    template('header')
    section('names')
    template('names')
    section('record')
