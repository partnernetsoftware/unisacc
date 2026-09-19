"""TableNet / StageNet / UnisaNet -- one class, three configurations. [N] [S]

    embed keys -> [factor] [bilinear] -> gemv+ReLU (1..2) -> heads -> argmax

TableNet : hidden=[h],        heads={'y': nout}
isel     : hidden=[24,16],    heads form/symbol/gate
abi      : hidden=[32,20],    heads sysno/arg0-2/ret/tls, bilinear=8
combo    : hidden=[48,32],    9 heads, factor=12 bilinear=8, shared W_reg
"""
import math
from .rng import Rng
from .linalg import gemv, gemv_bwd, relu, relu_bwd, argmax, softmax_ce, Adam


class Tensor:
    __slots__ = ("name", "rows", "cols", "w", "g", "m", "v")

    def __init__(self, name, rows, cols, init, rng):
        n = rows * cols
        self.name, self.rows, self.cols = name, rows, cols
        if init == 0.0:
            self.w = [0.0] * n
        else:
            self.w = [rng.gauss(init) for _ in range(n)]
        self.g = [0.0] * n
        self.m = [0.0] * n
        self.v = [0.0] * n


class Head:
    __slots__ = ("name", "classes", "share", "W", "b")

    def __init__(self, name, classes, share=None):
        self.name, self.classes, self.share = name, classes, share


class Net:
    def __init__(self, name, fields, hidden, heads, seed, factor=0, bilinear=0):
        # fields: [(fname, n_vocab, dim)]   heads: [(hname, classes, share|None)]
        self.name = name
        self.seed = seed
        self.fields = fields
        self.factor_n = factor
        self.bilinear_n = bilinear
        rng = Rng(seed)
        self.T = []                      # every tensor, fixed order  [D-6]

        def mk(nm, r, c, init):
            t = Tensor(nm, r, c, init, rng)
            self.T.append(t)
            return t

        self.E = [mk("E_" + f, n, d, 0.08) for (f, n, d) in fields]
        dsum = sum(d for (_, _, d) in fields)

        if factor:
            self.Wf = [mk("Wf_" + f, d, factor, math.sqrt(2.0 / dsum))
                       for (f, _, d) in fields]
            self.bf = mk("bf", 1, factor, 0.0)
        if bilinear:
            self.Pb = [mk("Pb_" + f, d, bilinear, math.sqrt(2.0 / dsum))
                       for (f, _, d) in fields]

        self.h0 = dsum + factor + bilinear
        self.L = []
        fan = self.h0
        for k, h in enumerate(hidden):
            W = mk("W%d" % (k + 1), fan, h, math.sqrt(2.0 / fan))
            b = mk("b%d" % (k + 1), 1, h, 0.0)
            self.L.append((W, b, fan, h))
            fan = h
        self.hlast = fan

        self.heads = []
        shared = {}
        for (hn, classes, share) in heads:
            H = Head(hn, classes, share)
            if share:
                if share not in shared:
                    shared[share] = mk("Wsh_" + share, fan, len(classes),
                                       math.sqrt(2.0 / fan))
                H.W = shared[share]
            else:
                H.W = mk("Wh_" + hn, fan, len(classes), math.sqrt(2.0 / fan))
            H.b = mk("bh_" + hn, 1, len(classes), 0.0)
            self.heads.append(H)
        self.head_by_name = {h.name: h for h in self.heads}
        self.opt = Adam()
        self.acc = 0.0

    # ---- inference -------------------------------------------------------

    def embed(self, key):
        """key: tuple of field indices -> (x, cache)"""
        x = []
        for i, (_, _, d) in enumerate(self.fields):
            base = key[i] * d
            x.extend(self.E[i].w[base:base + d])
        fpre = None
        if self.factor_n:
            f = self.bf.w[:]
            o = 0
            for i, (_, _, d) in enumerate(self.fields):
                Wf = self.Wf[i].w
                for a in range(d):
                    xa = x[o + a]
                    r = a * self.factor_n
                    for j in range(self.factor_n):
                        f[j] += xa * Wf[r + j]
                o += d
            fpre = f
            x.extend([a if a > 0.0 else 0.0 for a in f])
        bl = bal = bar = None
        if self.bilinear_n:
            n = self.bilinear_n
            proj = []
            o = 0
            for i, (_, _, d) in enumerate(self.fields):
                P = self.Pb[i].w
                p = [0.0] * n
                for a in range(d):
                    xa = x[o + a]
                    r = a * n
                    for j in range(n):
                        p[j] += xa * P[r + j]
                proj.append(p)
                o += d
            bal = proj[0]
            bar = [0.0] * n
            for p in proj[1:]:
                for j in range(n):
                    bar[j] += p[j]
            bl = [bal[j] * bar[j] for j in range(n)]
            x.extend(bl)
        return x, (fpre, bal, bar, proj if self.bilinear_n else None)

    def forward(self, key, heads=None):
        """heads=None computes every head; a subset keeps segment nets cheap
        (each sample only ever supervises the heads of its own stage)."""
        x, ec = self.embed(key)
        acts = [x]
        pres = []
        h = x
        for (W, b, fan, hn) in self.L:
            p = gemv(h, W.w, b.w, fan, hn)
            pres.append(p)
            h = relu(p)
            acts.append(h)
        out = {}
        for H in self.heads:
            if heads is not None and H.name not in heads:
                continue
            out[H.name] = gemv(h, H.W.w, H.b.w, self.hlast, len(H.classes))
        return out, (ec, acts, pres)

    def predict(self, key, heads=None):
        out, _ = self.forward(key, heads)
        return {n: H.classes[argmax(out[n])] for n, H in self.head_by_name.items()}

    # ---- training --------------------------------------------------------

    def backward(self, key, cache, dlogits):
        ec, acts, pres = cache
        h = acts[-1]
        dh = [0.0] * self.hlast
        for H in self.heads:
            dl = dlogits.get(H.name)
            if dl is None:
                continue
            d = gemv_bwd(h, H.W.w, dl, self.hlast, len(H.classes), H.W.g, H.b.g)
            for i in range(self.hlast):
                dh[i] += d[i]
        for k in range(len(self.L) - 1, -1, -1):
            W, b, fan, hn = self.L[k]
            dpre = relu_bwd(dh, pres[k])
            dh = gemv_bwd(acts[k], W.w, dpre, fan, hn, W.g, b.g)
        self._embed_bwd(key, ec, dh)

    def _embed_bwd(self, key, ec, dx):
        fpre, bal, bar, proj = ec
        dsum = sum(d for (_, _, d) in self.fields)
        demb = dx[:dsum]
        o = dsum
        if self.factor_n:
            n = self.factor_n
            df = [dx[o + j] if fpre[j] > 0.0 else 0.0 for j in range(n)]
            o += n
            bfg = self.bf.g
            for j in range(n):
                bfg[j] += df[j]
            p = 0
            for i, (_, _, d) in enumerate(self.fields):
                Wf = self.Wf[i]
                base = key[i] * d
                Ew = self.E[i].w
                for a in range(d):
                    xa = Ew[base + a]
                    r = a * n
                    s = 0.0
                    for j in range(n):
                        Wf.g[r + j] += xa * df[j]
                        s += Wf.w[r + j] * df[j]
                    demb[p + a] += s
                p += d
        if self.bilinear_n:
            n = self.bilinear_n
            db = dx[o:o + n]
            dproj = []
            for i in range(len(self.fields)):
                if i == 0:
                    dproj.append([db[j] * bar[j] for j in range(n)])
                else:
                    dproj.append([db[j] * bal[j] for j in range(n)])
            p = 0
            for i, (_, _, d) in enumerate(self.fields):
                P = self.Pb[i]
                dp = dproj[i]
                base = key[i] * d
                Ew = self.E[i].w
                for a in range(d):
                    xa = Ew[base + a]
                    r = a * n
                    s = 0.0
                    for j in range(n):
                        P.g[r + j] += xa * dp[j]
                        s += P.w[r + j] * dp[j]
                    demb[p + a] += s
                p += d
        p = 0
        for i, (_, _, d) in enumerate(self.fields):
            g = self.E[i].g
            base = key[i] * d
            for a in range(d):
                g[base + a] += demb[p + a]
            p += d

    def train_step(self, batch, lr):
        """batch: [(key, {head: class_index}, weight)]"""
        loss = 0.0
        n = len(batch)
        for item in batch:
            key, labels = item[0], item[1]
            w = item[2] if len(item) > 2 else 1.0
            out, cache = self.forward(key, labels.keys())
            dl = {}
            for hn, tgt in labels.items():
                l, d = softmax_ce(out[hn], tgt)
                loss += l * w
                k = w / n
                dl[hn] = [x * k for x in d]
            self.backward(key, cache, dl)
        self.opt.step(self.T, lr)
        return loss / max(1, len(batch))

    def evaluate(self, corpus):
        """FULL gold, per-head and joint accuracy. [N-4]"""
        n = len(corpus)
        per = {H.name: 0 for H in self.heads}
        joint = 0
        for (key, labels) in corpus:
            out, _ = self.forward(key, labels.keys())
            ok = True
            for hn, tgt in labels.items():
                if argmax(out[hn]) == tgt:
                    per[hn] += 1
                else:
                    ok = False
            if ok:
                joint += 1
        return joint / n, {k: v / n for k, v in per.items()}

    def nparams(self):
        return sum(len(t.w) for t in self.T)
