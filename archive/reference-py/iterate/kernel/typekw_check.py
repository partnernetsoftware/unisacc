"""iterate/kernel/typekw.tsv against unisa/front/lex.py TYPEKW [J10 step 3, typekw slice].

A TRANSITION COMPATIBILITY JUDGE, not an authority: genmodel writes TYPEV from
typekw.tsv, while the Python front end still uses lex.TYPEKW.  This reports
whether the two agree (values, order, count).  Neither is the single
project-wide source while Python does not consume the declaration.
  python3 typekw_check.py [typekw.tsv]     exit 0 equal, 1 different/invalid
lex.TYPEKW is read with ast from lex.py's source (no import side effects);
a missing, non-tuple/list, non-string or EMPTY TYPEKW is an error (exit 1)."""
import ast, os, sys
R = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
path = sys.argv[1] if len(sys.argv) > 1 else os.path.join(R, "iterate", "kernel", "typekw.tsv")

def fail(m):
    print("typekw_check: " + m)
    sys.exit(1)

# LEX_SRC (test entry): a fixture copy of lex.py for the judge's own negatives
tree = ast.parse(open(os.environ.get("LEX_SRC") or os.path.join(R, "unisa", "front", "lex.py")).read())
hits = [n for n in tree.body if isinstance(n, ast.Assign) and len(n.targets) == 1
        and isinstance(n.targets[0], ast.Name) and n.targets[0].id == "TYPEKW"]
if len(hits) != 1:
    fail("lex.py: %d top-level TYPEKW assignments, want 1" % len(hits))
v = hits[0].value
if not isinstance(v, (ast.Tuple, ast.List)) or not v.elts:
    fail("lex.py: TYPEKW is not a non-empty tuple/list literal")
if not all(isinstance(e, ast.Constant) and isinstance(e.value, str) for e in v.elts):
    fail("lex.py: TYPEKW holds a non-string element")
lex = [e.value for e in v.elts]

decl = []
lines = open(path, encoding="latin-1").read().split("\n")
if lines and lines[-1] == "":
    lines.pop()
for i, ln in enumerate(lines):
    if ln.startswith("#"):
        continue
    t = ln.split("\t")
    if len(t) != 2 or t[0] != "kw":
        fail("%s line %d: want kw<TAB>value" % (path, i + 1))
    decl.append(t[1])
if not decl:
    fail("%s: no kw rows" % path)

if decl == lex:
    print("typekw_check: %s == lex.TYPEKW (%d values, same order)" % (path, len(decl)))
    sys.exit(0)
print("typekw_check: DIFFERENCE: %s != lex.TYPEKW" % path)
for i in range(max(len(decl), len(lex))):
    a = decl[i] if i < len(decl) else "(none)"
    b = lex[i] if i < len(lex) else "(none)"
    if a != b:
        print("typekw_check:   [%d] declared %r, lex %r" % (i, a, b))
if sorted(decl) == sorted(lex):
    print("typekw_check:   same set, different order")
sys.exit(1)
