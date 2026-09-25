"""iterate/kernel/vocab.tsv against unisa/ckernel.py [J10 step 3, second slice].

Transition check: during the transition ckernel.emit_core is what writes the
vocab / BF / BH region, so this proves the declared mapping equals it:
  - the (SYM, tuple) list of ckernel.py's vocab loop (read from its source
    with ast, the names resolved in unisa.gold / unisa.front.lex), in order,
    with TYPEV at the typekw row's position;
  - each vocab row's TSV list == the named gold tuple (values, order, length);
  - the bfbh stage list == ckernel.py's literal stage tuple, in order;
  - per bfbh stage: STAGES[st].fields mapped through str() == the TSV #field
    lists, and the built nets' heads (weights/built.json) == the #head lists.
Exit 0 = all equal.  It reads Python and built.json; genmodel reads neither."""
import ast, os, sys
R = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, R)
from unisa import gold, intnet
from unisa.front import lex

def tsv(st):
    F, H = [], []
    for ln in open(os.path.join(R, "weights", "gold", st + ".tsv")):
        t = ln.rstrip("\n").split("\t")
        if t[0] == "#field":
            F.append((t[1], t[2:]))
        elif t[0] == "#head":
            H.append((t[1], t[3:]))
    return F, H

rows = [l.rstrip("\n").split("\t") for l in open(os.path.join(R, "iterate", "kernel", "vocab.tsv"))
        if l.strip() and not l.startswith("#")]
voc = [r for r in rows if r[0] in ("vocab", "typekw")]
bfbh = [r[1] for r in rows if r[0] == "bfbh"]
bad = []

# ckernel.py's vocab loop and bfbh tuple, from its source
tree = ast.parse(open(os.path.join(R, "unisa", "ckernel.py")).read())
ck_voc, ck_bfbh = None, None
for node in ast.walk(tree):
    if isinstance(node, ast.For) and isinstance(node.iter, ast.Tuple):
        el = node.iter.elts
        if el and all(isinstance(e, ast.Tuple) and len(e.elts) == 2 for e in el):
            ck_voc = [(e.elts[0].value, e.elts[1].id) for e in el]
        elif el and all(isinstance(e, ast.Constant) for e in el):
            ck_bfbh = [e.value for e in el]
if ck_voc is None or ck_bfbh is None:
    sys.exit("vocab_check: ckernel.py vocab loop or bfbh tuple not found")

if [r[1] for r in voc] != [n for n, _ in ck_voc]:
    bad.append("vocab order/symbols %s != ckernel %s" % ([r[1] for r in voc], [n for n, _ in ck_voc]))
for r, (nm, tup) in zip(voc, ck_voc):
    if r[0] == "typekw":
        if (nm, tup) != ("TYPEV", "TYPEKW"):
            bad.append("typekw row %s is ckernel's %s/%s" % (r, nm, tup))
        continue
    _, sym, st, kind, name = r
    F, H = tsv(st)
    lst = dict(F if kind == "field" else H).get(name)
    want = list(getattr(gold, tup))
    if lst != want:
        bad.append("%s: %s %s %s != gold.%s" % (sym, st, kind, name, tup))
if bfbh != ck_bfbh:
    bad.append("bfbh %s != ckernel %s" % (bfbh, ck_bfbh))
nets = intnet.load_all(os.path.join(R, "weights", "built.json"))
nbf = nbh = 0
for st in bfbh:
    F, H = tsv(st)
    pf = [(n, [str(v) for v in vals]) for n, vals in gold.STAGES[st].fields]
    ph = [(hn, [str(c) for c in cl]) for hn, cl in nets[st].heads]
    if F != pf:
        bad.append("%s fields != str(STAGES fields)" % st)
    if H != ph:
        bad.append("%s heads != nets heads" % st)
    nbf += len(F); nbh += len(H)
for b in bad:
    print("vocab_check: " + b)
print("vocab_check: %d vocab rows (%d generated + typekw %s) and %d bfbh stages "
      "(%d BF, %d BH) %s ckernel.py" % (len(voc), sum(r[0] == "vocab" for r in voc),
      [r[1] for r in voc if r[0] == "typekw"], len(bfbh), nbf, nbh, "==" if not bad else "!="))
sys.exit(1 if bad else 0)
