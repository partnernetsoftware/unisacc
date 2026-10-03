"""Bounded semantic USLSIG2/3 graph equality in ordinary model actions.
MG.equal: mg_left_blob/mg_left_len, mg_right_blob/mg_right_len -> mg_equal.
MG.compatible: same inputs/output; complete source/external origins may differ.
Canonical validation precedes indexing. Top-level mode is excluded by the old
contract; nested modes and every ABI layout fact remain significant. No host
parser, graph isomorphism or callback execution. Budget exhaustion rejects.
"""
FIELDS, EDGES, LAYOUT, SIGID, PAIRLEFT, PAIRRIGHT, VISITED, FRAME = (i<<40 for i in range(430,438))
assert not set(range(430,438)) & (set(range(350,358)) | set(range(400,408)) | set(range(410,414)))
# Legacy FIELDS stride16, EDGES stride1025 and LAYOUT stride4 stay unchanged.
# V3 extras: per-node [version,rank,format,natural,flags,known,origin];
# per-entry [ordinal,kind,effective_alignment].
EXTRA, ENTRYEXTRA = (i<<40 for i in range(632,634))
assert not set(range(632,634)) & set(range(430,438))

def install(E, fail='DEAD'):
    """Stage control lives in STEM-result.tsv (finite_rules declarations). Python binds only
    the bank constants, the fail target and the fresh labels (STEM-fresh.tsv, created in the
    original order so label numbering is unchanged); helper stages stay helper calls."""
    P=E.P
    if 'MG.equal' in E.g.st: return
    import pathlib
    from finite_rules import install as install_rules
    from modelsignature import install as install_signature
    install_signature(E,fail)
    root=pathlib.Path(__file__).parent
    fresh=[l.split('\t') for l in (root/'modelgraphequality-fresh.tsv').read_text().splitlines()[1:]]
    bindings=dict(FIELDS=FIELDS,EDGES=EDGES,LAYOUT=LAYOUT,SIGID=SIGID,PAIRLEFT=PAIRLEFT,PAIRRIGHT=PAIRRIGHT,VISITED=VISITED,FRAME=FRAME,EXTRA=EXTRA,ENTRYEXTRA=ENTRYEXTRA,fail=fail)
    def section(name):
        p=P('MG.fresh')
        for part,key,kind in fresh:
            if part==name: bindings[key]=p.fresh(kind)
        install_rules(E.g,root,'modelgraphequality',bindings,{},None,name)
    section('main')
