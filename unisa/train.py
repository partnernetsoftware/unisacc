"""Training loop. [TR] [N-3] [N-4] [D-3]"""
import time
from .rng import Rng
from .net import Net
from .gold import STAGES, ALL, TABLES
from . import uns1

NET_READY = 0.85      # [F-1]
NET_HOT = 0.995       # [F-2]
SHIP_ACC = 1.000      # [F-3]
BATCH = 16


def lr_for(epoch):
    """[TR-2] epoch decay schedule -- NOT a parameter-count tier."""
    if epoch < 18:
        return 0.032
    if epoch < 50:
        return 0.014
    if epoch < 90:
        return 0.006
    return 0.0025


def make_net(st):
    cfg = st.cfg
    dims = cfg.get("dims") or [cfg["d"]] * len(st.fields)
    fields = [(fn, len(vo), dims[i]) for i, (fn, vo) in enumerate(st.fields)]
    return Net(st.name, fields, cfg["hidden"], list(st.heads), cfg["seed"],
               factor=cfg.get("factor", 0), bilinear=cfg.get("bilinear", 0))


def build_nets(names=ALL):
    return {n: make_net(STAGES[n]) for n in names}


def train(names=ALL, epochs=200, verbose=True, out=None):
    """[TR-3] Exactness is made ABSORBING.

    Measured (E-14): 30 of 99 seed runs reached 1.000 and then fell back off it,
    including three of the shipped seeds -- the hot-skip [N-3] resumes full-LR
    Adam steps with stale moments and can knock a marginal key loose.  So: the
    first epoch a net reaches SHIP_ACC we snapshot it and stop training it.
    A stage that is exact has nothing left to learn.
    """
    nets = build_nets(names)
    evalc = {n: STAGES[n].corpus() for n in names}
    trainc = {n: STAGES[n].train_corpus() for n in names}
    t0 = time.time()
    hist = []
    best = {}                       # name -> (acc, [tensor copies]) snapshot
    for epoch in range(epochs):
        lr = lr_for(epoch)
        for n in names:
            net = nets[n]
            if n in best:                                # frozen: already exact
                continue
            if net.acc >= NET_HOT and epoch % 6 != 0:     # [N-3] hot-skip
                continue
            corpus = trainc[n]
            order = Rng(net.seed ^ epoch).perm(len(corpus))  # [D-3]
            for i in range(0, len(order), BATCH):
                batch = [corpus[j] for j in order[i:i + BATCH]]
                net.train_step(batch, lr)
        for n in names:                                   # [N-4] FULL gold
            if n in best:
                continue
            net = nets[n]
            net.acc, _ = net.evaluate(evalc[n])
            if net.acc >= SHIP_ACC:                       # snapshot and freeze
                best[n] = (net.acc, [t.w[:] for t in net.T])
        accs = {n: nets[n].acc for n in names}
        hist.append(accs)
        if verbose and (epoch % 5 == 0 or epoch == epochs - 1
                        or all(a >= SHIP_ACC for a in accs.values())):
            line = "  ".join("%s %.3f" % (n, accs[n]) for n in names)
            print("ep%-3d lr%.4f  %s" % (epoch, lr, line), flush=True)
        if len(best) == len(names):                       # [TR-3]
            if verbose:
                print("-- all stages 1.000 at epoch %d (%.1fs)"
                      % (epoch, time.time() - t0))
            break
    for n, (acc, ws) in best.items():                     # restore snapshots
        for t, w in zip(nets[n].T, ws):
            t.w = w[:]
        nets[n].acc = acc
    if out:
        import os
        os.makedirs(out, exist_ok=True)
        for n in names:
            uns1.save_net(nets[n], os.path.join(out, n + ".f32.unisa"))
    return nets, hist


def report(nets):
    print("%-7s %7s %8s  %s" % ("stage", "rows", "theta", "acc"))
    under = []
    for n, net in nets.items():
        rows = STAGES[n].rows()
        flag = "" if net.acc >= SHIP_ACC else "  <-- UNDERFIT"  # [TR-4]
        if flag:
            under.append(n)
        print("%-7s %7d %8d  %.4f%s" % (n, rows, net.nparams(), net.acc, flag))
    tot = sum(net.nparams() for net in nets.values())
    print("%-7s %7s %8d" % ("TOTAL", "", tot))
    return under
