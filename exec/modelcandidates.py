"""USBIND2 candidate selection in delta, normalized to the existing USBIND1 decoder.
Winner priority is (origin, ordinal), independent of record order. Output names
retain first-seen order; no host parser or host winner selection participates.
V1 passthrough does not alter the caller's input/output/string builder.
"""
LIST, SEEN, DUP, RANK, BEGIN, END, ORIGIN, LENGTH = (i<<40 for i in range(300,308))


def install(E, fail='DEAD'):
    """Stage control lives in STEM-result.tsv (finite_rules declarations). Python binds only
    the bank constants, the fail target and the fresh labels (STEM-fresh.tsv, created in the
    original order so label numbering is unchanged); helper stages stay helper calls."""
    P=E.P
    if 'MC.normalize' in E.g.st: return
    import pathlib
    from finite_rules import install as install_rules
    from modelsignature import install as install_signature
    install_signature(E,fail)
    root=pathlib.Path(__file__).parent
    fresh=[l.split('\t') for l in (root/'modelcandidates-fresh.tsv').read_text().splitlines()[1:]]
    bindings=dict(LIST=LIST,SEEN=SEEN,DUP=DUP,RANK=RANK,BEGIN=BEGIN,END=END,ORIGIN=ORIGIN,LENGTH=LENGTH,fail=fail)
    def section(name):
        p=P('MC.fresh')
        for part,key,kind in fresh:
            if part==name: bindings[key]=p.fresh(kind)
        install_rules(E.g,root,'modelcandidates',bindings,{},None,name)
    section('main')
    from modelinput import u64
    u64(E,'MC.callablecap',b'\0library/callables','mc_callablecap','mc_callablepresent','MC.fail')
    section('afteru64')
