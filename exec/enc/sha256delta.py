"""SHA-256 transitions for Mach-O page signatures; no hash runtime primitive.
Input: sh_blob. Appends 32 digest bytes, restores input, returns via RET.
Round/initial constants are facts in exec/facts/enc-sha256delta-constants.tsv. The
compression and padding algorithm is explicitly compiled into ordinary actions.
"""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from exec.facts.load import facts
_BANK = {r['name']: r['bank'] << 40 for r in facts('enc-sha256delta-regions')}
W, K = _BANK['W'], _BANK['K']  # distinct from the image DATA byte region


def constants():
    rows=facts('enc-sha256delta-constants')
    k=[r['value'] for r in rows if r['table']=='k']
    h=[r['value'] for r in rows if r['table']=='h']
    assert [r['index'] for r in rows]==list(range(64))+list(range(8))
    assert len(k)==64 and len(h)==8
    assert all(0<=x<2**32 for x in k+h)
    return k,h


def install(E):
    from finite_rules import install as install_rules
    ks, hs = constants()
    initial = []
    for i, value in enumerate(ks):
        initial.extend((("LDI", "sh_x", i), ("LDI", "sh_y", value), ("STX", "sh_x", K, "sh_y")))
    initial.extend(("LDI", "sh_h" + str(i), value) for i, value in enumerate(hs))
    bindings = dict(W=W, K=K)
    for line in Path(__file__).with_name("sha256-names.tsv").read_text().splitlines():
        if line and not line.startswith("#"):
            key, prefix, kind = line.split("\t")
            bindings[key] = E.P(prefix + ".sha_" + key).fresh(kind)
    install_rules(E.g, Path(__file__).parent, "sha256", bindings=bindings,
                  sequences={"constants": initial}, section="main")
