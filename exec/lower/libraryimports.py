"""Declared USBIND1/2/3 capability gates dedicated library calls in lowering.
The shared decoder validates fixed format1 and variadic-template format2
TypeGraphs/dispatcher handles as capabilities;
E3 chooses and types wrappers. Lower never classifies a source type or picks an import.
Ordinary .hostcall handling is untouched; only the dedicated token uses this route.
Stage control (including the legacy LBI.checkdesc constructor helpers) lives in
libraryimports-result.tsv (finite_rules); fresh labels in libraryimports-fresh.tsv,
pre-allocated per section in the original global order. Python binds the
.librarycall token, REG/DESC bases and the os capability target. The C.dispatch wrap is a
move edit in libraryimports-template.tsv. Residue: the shared decoder install and the three modelinput.u64
resource readers (they create fact-dependent states).
"""
from pathlib import Path
NAMES, IDS, ADDRESS, ARGC, DESC, SEEN = (i << 40 for i in range(190,196))

def install(E, os_, ids):
    from finite_rules import install as rules
    from code import REG
    from modelinput import u64
    g=E.g;root=Path(__file__).parent
    b=dict(call=ids['.librarycall'],REG=REG,DESC=DESC,arity='LBI.arity' if os_ in ('osx','lnx','win') else 'LBI.fail')
    fresh=[l.split('\t') for l in (root/'libraryimports-fresh.tsv').read_text().splitlines()[1:]];ps={}
    def section(name):
        for sec,k,kind,prefix in fresh:
            if sec==name:b[k]=ps.setdefault(prefix,E.P(prefix)).fresh(kind)
        rules(g,root,'libraryimports',b,section=name)
    from modelbindings import install as install_decoder
    install_decoder(E)
    from finite_rules import install_template
    install_template(g,root,'libraryimports',{},None,section='dispatch')
    section('s1')
    # Callable-only modules have no import binding table. Their explicit host
    # resources authorise the generic dispatcher, never named data resolution.
    u64(E,'LBI.callableflag',b'\0library/callables','lbi_callableflag','lbi_callablepresent','LBI.fail')
    u64(E,'LBI.callablemake',b'\0library/callablemake','lbi_callablemake','lbi_makepresent','LBI.fail')
    u64(E,'LBI.callablecall',b'\0library/callablecall','lbi_callablecall','lbi_callpresent','LBI.fail')
    section('s2')
