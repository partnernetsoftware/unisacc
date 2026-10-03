"""Finite USBIND1/2/3 resource wire decoder, reusable by stage builders.
No source parsing or host-side ABI selection. Structural validation precedes decoding; named data facts are checked on use.
Function typing is proved by E3; raw .librarycall is an explicitly trusted tape capability.
"""
from pathlib import Path
NAMES, IDS, ADDRESS, ARGC, DESC, SEEN = (i << 40 for i in range(190,196))
KIND, EXTENT, WRITABLE = (i << 40 for i in range(196,199))
VARIADIC, SUPPORTED = (i << 40 for i in range(313,315))
FORMAT, TYPED, CANON, CANONLEN, DISPATCH, PLAN, RESULT_KIND, RESULT_WIDTH = (i << 40 for i in range(320,328))
STRIDE=6150

def install(E):
    """Stage control lives in modelbindings-result.tsv (sections read, head, present, header,
    names, record). Python binds only dynamic facts: the graph-table constants, the resource
    path bytes, and fresh labels pre-allocated in original order (modelbindings-fresh.tsv).
    Kept in Python, each because it expands a fact into states through the call/branch
    helpers: the USBIND magic chain, the identifier byte class (one nameok state per byte,
    target named by the byte) and the two record kinds; plus install_candidates (MC.*)."""
    import json
    from finite_rules import install as rules
    P,g=E.P,E.g
    root=Path(__file__).parent
    fresh=[l.split('\t') for l in (root/'modelbindings-fresh.tsv').read_text().splitlines()[1:]]
    bindings=dict(NAMES=NAMES,IDS=IDS,ADDRESS=ADDRESS,ARGC=ARGC,DESC=DESC,SEEN=SEEN,KIND=KIND,
                  EXTENT=EXTENT,WRITABLE=WRITABLE,VARIADIC=VARIADIC,SUPPORTED=SUPPORTED,FORMAT=FORMAT,
                  TYPED=TYPED,CANON=CANON,CANONLEN=CANONLEN,DISPATCH=DISPATCH,PLAN=PLAN,
                  RESULT_KIND=RESULT_KIND,RESULT_WIDTH=RESULT_WIDTH)
    sequences={'librarypath':[('SBOUT',x) for x in b'\0library/bindings']}
    def section(name):
        p=P('LBI.fresh')
        for part,key,kind in fresh:
            if part==name:bindings[key]=p.fresh(kind)
        rules(g,root,'modelbindings',bindings,sequences,None,name)
    rules(g,root,'modelbindings',section='read')
    section('head')
    from modelcandidates import install as install_candidates
    install_candidates(E)
    section('present')
    for i,c in enumerate(b'USBIND'):
        P('LBI.magic'+str(i)).call('LBI.byte').branch({c:'LBI.magic'+str(i+1)},'LBI.fail',[('RLD','lbi_byte')])
    section('header')
    P('LBI.namebyte').call('LBI.byte').branch({x:'LBI.nameok.'+str(x) for x in b'_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ'},'LBI.namedigit',[('RLD','lbi_byte')])
    P('LBI.namedigit').branch({1:'LBI.fail'},'LBI.digit',[('CMPI','lbi_j',0)])
    P('LBI.digit').branch({x:'LBI.nameok.'+str(x) for x in range(48,58)},'LBI.fail',[('RLD','lbi_byte')])
    for c in b'_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789':
        P('LBI.nameok.'+str(c)).a(('SBOUT',c),('ALUI','add','lbi_j','lbi_j',1)).goto('LBI.name')
    section('names')
    for kind in (0,1):
        P('LBI.kind'+str(kind)).a(('LDI','lbi_kind',kind),('STX','lbi_i',KIND,'lbi_kind')).goto('LBI.origin')
    section('record')
