"""E1 step 3: the lexer delta as constructed integer nets (unisa construction).

    python3 exec/lex/net.py delta.json OUTDIR

What is built.  The single stage key = (state, observation) -- exec/lex/stage.py,
86,095 keys, quotient 306 x 87 = 26,622 -- is past the construction's reach:
its greedy decision list costs ~23-26 s per rule on this machine (one EXPAND
per remaining key per field order), and it needs hundreds of rules, far past
the 60 s ceiling.  The minimal change that fits: one stage PER STATE, key =
the observed value v (0..256) of the component that state reads, heads nxt
and seq.  Same function (the state moves from a key field to the stage index),
same construction (unisa.construct.build_net), same format (UNS2).

Steps, all in this process:
  1. build every per-state net; intnet.build verifies each by enumeration
     over its whole domain (construct.verify_int, integer kernel);
  2. write the nets as one UNS2 blob, load it back;
  3. T1: the LOADED nets, run by IntNet.predict on every (state, v), equal the
     delta on the whole domain;
  4. DENSE: the loaded nets' answers on each state's own domain (b: 257, t:
     GAMMA, r: 0..19) written back as a delta JSON (OUTDIR/e1net.json), which
     tbl.py turns into the executor's table.  So the executor runs a table
     COMPUTED BY RUNNING THE NET, as the compiler's DENSE is -- a lookup at run
     time, not inference.
"""
import json
import os
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, ROOT)
sys.path.insert(0, HERE)

from unisa.gold import Stage                     # noqa: E402
from unisa import intnet, uns2                   # noqa: E402
import stage as e1stage                          # noqa: E402

V = tuple(str(v) for v in range(257))


def stages(d):
    _, tab = e1stage.stage(d)
    reg, qname = {}, {}
    for k, q in enumerate(d["states"]):
        cn = sorted(set(tab[(q, v)]["nxt"] for v in V))
        cs = sorted(set(tab[(q, v)]["seq"] for v in V), key=int)
        n = "q%d" % k
        reg[n] = Stage(n, [("v", V)], [("nxt", tuple(cn), None), ("seq", tuple(cs), None)],
                       lambda v, q=q: tab[(q, v)], cfg=None)
        qname[n] = q
    return reg, qname, tab


def main():
    d = json.load(open(sys.argv[1]))
    out = sys.argv[2]
    os.makedirs(out, exist_ok=True)
    t = time.time()
    reg, qname, tab = stages(d)
    nets = intnet.build_all(list(reg), verify=True, stages=reg)       # 1
    tb = time.time() - t
    H = sum(n.H for n in nets.values())
    wv = sorted(set(w for n in nets.values() for w in n.weight_values()))
    mx = max(max(len(cl) for _, cl, _ in S.heads) for S in reg.values())
    blob = uns2.dump(nets, reg)                                          # 2
    with open(os.path.join(out, "e1.uns2"), "wb") as f:
        f.write(blob)
    back = uns2.load(blob, reg)
    bad = n = 0                                                          # 3
    for sn, net in back.items():
        q = qname[sn]
        for v in V:
            n += 1
            if net.predict((v,)) != tab[(q, v)]:
                bad += 1
    GAMMA = e1stage.GAMMA                                                # 4
    states = {}
    for sn, net in sorted(back.items(), key=lambda kv: int(kv[0][1:])):
        q = qname[sn]
        mode, ref = d["states"][q]
        dom = [(k, str(GAMMA.index(k)) if mode == "t" else k) for k in ref]  # the delta's own order
        assert len(dom) == (257 if mode == "b" else len(GAMMA) if mode == "t" else 20)
        row = {}
        for key, v in dom:
            a = net.predict((v,))
            row[key] = [a["nxt"], int(a["seq"])]
        states[q] = [mode, row]
    dj = {"states": states, "seqs": d["seqs"], "tok_names": d["tok_names"]}
    with open(os.path.join(out, "e1net.json"), "w") as f:
        json.dump(dj, f, separators=(",", ":"))
    same = dj["states"] == d["states"]
    print("per-state stages %d  units %d  weight values %s  max classes/head %d  build+verify %.1f s"
          % (len(nets), H, wv, mx, tb))
    print("uns2 %d B  (e1.uns2)" % len(blob))
    print("T1 %s: %d keys (loaded UNS2 nets vs delta), wrong %d" % ("ok" if bad == 0 and n else "FAIL", n, bad))
    print("net-derived delta == gen delta: %s" % same)
    return 0 if bad == 0 and n and same else 1


if __name__ == "__main__":
    sys.exit(main())
