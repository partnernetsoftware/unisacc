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
def install(E,fail='DEAD'):
 """Stage control lives in modelgraphequality-result.tsv (section pre). Python binds only the
 bank constants, the fail continuation, and fresh labels pre-allocated in original order
 (modelgraphequality-fresh.tsv) without registering any extra named state. Kept in Python:
 the canonical (MS.*) install it depends on."""
 from pathlib import Path
 from finite_rules import install as rules
 if 'MG.equal' in E.g.st:return
 from modelsignature import install as canonical_install
 canonical_install(E,fail)
 root=Path(__file__).parent
 bindings=dict(FIELDS=FIELDS,EDGES=EDGES,LAYOUT=LAYOUT,SIGID=SIGID,PAIRLEFT=PAIRLEFT,PAIRRIGHT=PAIRRIGHT,
               VISITED=VISITED,FRAME=FRAME,EXTRA=EXTRA,ENTRYEXTRA=ENTRYEXTRA,fail=fail)
 holder=type('FreshScope',(),{'cur':'MG'})()
 for part,key,kind in (l.split('\t') for l in (root/'modelgraphequality-fresh.tsv').read_text().splitlines()[1:]):
  bindings[key]=E.P.fresh(holder,kind)
 rules(E.g,root,'modelgraphequality',bindings,None,None,'pre')
