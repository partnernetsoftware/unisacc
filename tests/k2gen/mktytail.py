# appends tytail tables (ckmrows/ckmfinal/resdrows/resdfinal) to exec/facts/k2-gen2.tsv from gen2's constants
import json, sys
from pathlib import Path
root = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(root / "exec/parse2"))
import gen2 as G
acts = lambda a: [list(x) for x in a]
initials = {name: row[0][1] for name, row in G.load_rules(root / "exec/parse2/operator-actions.tsv",
            {}, domain=[0], section="initial").items()}
ck, current, initial = [], "CKM", initials["ckm"]
for width, mask in [(sz, (1 << (8 * sz)) - 1) for _, _, sz, un, _ in G.TYINT if un and sz < 8]:
    nxt = "CKM.k%d" % width
    ck.append(dict(current=current, hit="CKM.m%d" % width, next=nxt, axis=G.AX.index("u%d" % (8 * width)),
                   initial=acts(initial), mask=acts(G.O(G.TYPE_TAPE["mask_pair"] % mask))))
    current, initial = nxt, []
ckf = dict(current=current, axis=G.AX.index("u64"), initial=acts(initial))
rs, current, initial = [], "RESD", initials["resd"]
for name, code, size, unsigned, _ in G.TYINT:
    hit, nxt = "RESD." + name, "RESD.n" + name
    mask = G.O(G.TYPE_TAPE["mask"] % ((1 << (8 * size)) - 1)) if unsigned and size < 8 else []
    rs.append(dict(current=current, hit=hit, next=nxt, axis=G.AX.index(name), code=code,
                   initial=acts(initial), mask=acts(mask)))
    current, initial = nxt, []
rsf = dict(current=current, initial=acts(initial))
p = root / "exec/facts/k2-gen2.tsv"
s = "".join(l for l in p.read_text().splitlines(True) if not l.startswith(("=ckm", "=resd", "# tytail")))
s += "# tytail tables (tests/k2gen/mktytail.py from TYINT, AX, TYPE_TAPE, operator-actions initial)\n"
for k, v in (("ckmrows", ck), ("ckmfinal", ckf), ("resdrows", rs), ("resdfinal", rsf)):
    s += "=%s\tjson\t%s\n" % (k, json.dumps(v))
p.write_text(s)
