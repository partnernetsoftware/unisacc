"""iterate/kernel/order.tsv must equal unisa/gold.py ALL (and HEADS_MAX).
During the transition gold.ALL is the comparison source: this proves the two
agree, not that order.tsv is the only ordering authority.  Exit 0 = equal."""
import os, sys
R = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, R)
from unisa import gold, ckernel
st, hm = [], []
for ln in open(os.path.join(R, "iterate", "kernel", "order.tsv")):
    t = ln.rstrip("\n").split("\t")
    if not t[0] or t[0].startswith("#"):
        continue
    (st if t[0] == "stage" else hm).append(t[1:])
got = [x[0] for x in st]
ok = got == list(gold.ALL) and hm == [[str(ckernel.HEADS_MAX)]]
print("order.tsv %s gold.ALL (%d stages), heads_max %s %s HEADS_MAX %d"
      % ("==" if got == list(gold.ALL) else "!=", len(got), hm,
         "==" if hm == [[str(ckernel.HEADS_MAX)]] else "!=", ckernel.HEADS_MAX))
sys.exit(0 if ok else 1)
