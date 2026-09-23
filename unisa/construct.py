"""Constructive weights: gold decision table -> exact integer net. [E-22] [K-5]

No training, no randomness, no float.  A conjunction over one-hot key fields is
one ReLU unit, so the deploy kernel is unchanged:

    W1 in {0,1}          b1 = -(|F|-1) in {0,-1,-2}, not stored
    W2 in {1,2,4,8,16}   hidden activation in {0,1}

so layer 1 is |F| integer adds and layer 2 is a conditional integer add --
no multiply, no shift, no float anywhere.  Exactness is an invariant of the
construction, verified by enumeration over the FULL domain (verify_int).

Ported from the minimal-cover derivation; techniques, in order of payoff:
  T2 decision list + greedy max cover   796,490 -> 71,828 theta
  T1 set-valued literals                one unit per gold override line
  REDUCE + winner-vs-loser ranking      keeps W2 bounded at 16
  T4 cross-head cube sharing            W1 shared, only W2 differs per head
  T5 additive factorisation             biggest win on the multi-head stages
"""

import os, sys, time

# run as a script from anywhere: the repo root is two levels up from this file
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from unisa.gold import STAGES, ALL
from unisa.linalg import argmax

NAIVE_THETA = 796490
SGD_THETA = 21925


# ------------------------------------------------------------- cube algebra --
# cube = tuple of frozensets of value indices, one per field.
# S_i == full vocab  =>  don't-care (no literal, not counted in the bias).

def cube_key(c):
    return tuple(tuple(sorted(s)) for s in c)


class Domain:
    """The key space, quotiented by value duplicates. [E-26]

    Per field, values whose label slice is identical are indistinguishable to an
    exact net, so they are merged.  The quotient is sound BOTH ways: restricting
    gives h_red <= h_full, and lifting each literal set to the union of the
    groups it names reproduces the logits verbatim, giving h_full <= h_red.  So
    K_s is not narrowed -- this is a bijective restatement, and verification
    still runs over the ORIGINAL domain (see verify_int).
    """

    def __init__(self, stage, quotient=True):
        from itertools import product as _product
        # Accept a stage name (unisa registry) or a Stage object (sibling
        # products such as ujs reuse the same construction without a second
        # copy of the cube algebra).
        if isinstance(stage, str):
            S = STAGES[stage]
            self.name = stage
        else:
            S = stage
            self.name = S.name
        self.S = S
        self.ovocabs = [list(v) for (_, v) in S.fields]
        self.m = len(self.ovocabs)
        self.okeys = S.keys()
        ovidx = [{v: i for i, v in enumerate(vo)} for vo in self.ovocabs]
        self.okidx = [tuple(ovidx[i][v] for i, v in enumerate(kv))
                      for kv in self.okeys]
        hn = [h[0] for h in S.heads]
        olab = {kv: S.label(*kv) for kv in self.okeys}

        groups = []
        for i in range(self.m):
            sig = {}
            for vi, v in enumerate(self.ovocabs[i]):
                slice_ = tuple(tuple(olab[kv][h] for h in hn)
                               for kv in self.okeys if kv[i] == v)
                sig.setdefault(slice_, []).append(vi)
            groups.append(sorted(sig.values(), key=lambda g: g[0])
                          if quotient else [[k] for k in
                                            range(len(self.ovocabs[i]))])
        self.groups = groups
        self.vocabs = [list(range(len(g))) for g in groups]
        self.m = len(self.vocabs)
        qkeys = list(_product(*[range(len(g)) for g in groups]))
        self.kidx = qkeys
        self.keys = [tuple(self.ovocabs[i][groups[i][g][0]]
                           for i, g in enumerate(gk)) for gk in qkeys]
        self.n = len(self.keys)
        self.full = [frozenset(range(len(v))) for v in self.vocabs]
        self.mask = [[0] * len(vo) for vo in self.vocabs]
        for j, ki in enumerate(self.kidx):
            for i, v in enumerate(ki):
                self.mask[i][v] |= 1 << j
        self.ALL = (1 << self.n) - 1

    def cube_mask(self, cube):
        m = self.ALL
        for i, s in enumerate(cube):
            if len(s) == len(self.vocabs[i]):
                continue
            o = 0
            for v in s:
                o |= self.mask[i][v]
            m &= o
        return m

    def nlits(self, cube):
        return sum(1 for i, s in enumerate(cube)
                   if len(s) != len(self.vocabs[i]))


# ------------------------------------------- T1+T2: greedy decision list -----
def expand(D, seed, rem, badmask, order, setvalued=True):
    """Espresso EXPAND on the singleton cube {seed}: grow every literal as far
    as it stays consistent with the REMAINING keys (already-covered keys may be
    shadowed -- that is what makes this a decision list, not a cover)."""
    cur = [D.mask[i][seed[i]] for i in range(D.m)]
    sets = [{seed[i]} for i in range(D.m)]
    for i in order:
        others = D.ALL
        for j in range(D.m):
            if j != i:
                others &= cur[j]
        if not (others & badmask):
            sets[i] = set(range(len(D.vocabs[i])))
            cur[i] = D.ALL
            continue
        if not setvalued:
            continue
        for v in range(len(D.vocabs[i])):
            if v in sets[i]:
                continue
            nm = cur[i] | D.mask[i][v]
            if not (others & nm & badmask):
                sets[i] = sets[i] | {v}
                cur[i] = nm
    cube = tuple(frozenset(s) for s in sets)
    return cube, D.cube_mask(cube)


def _orders(m):
    if m == 1:
        return [(0,)]
    if m == 2:
        return [(0, 1), (1, 0)]
    return [(0, 1, 2), (2, 1, 0), (1, 0, 2), (0, 2, 1), (1, 2, 0), (2, 0, 1)]


def decision_list(D, classof, pool=None, setvalued=True, reduce_=True):
    """Rivest decision list, highest priority first.  `pool` (cubes another head
    already pays for) only breaks coverage TIES, so sharing never costs a term."""
    labmask = {}
    for j in range(D.n):
        labmask[classof[j]] = labmask.get(classof[j], 0) | (1 << j)
    ords = _orders(D.m)
    poolk = {cube_key(c) for c in pool} if pool else set()
    pool = sorted(pool, key=cube_key) if pool else []
    rem = D.ALL
    rules = []
    while rem:
        best = None
        seen = set()
        for j in range(D.n):
            if not (rem >> j) & 1:
                continue
            L = classof[j]
            bad = rem & ~labmask[L]
            for o in ords:
                cube, cm = expand(D, D.kidx[j], rem, bad, o, setvalued)
                ck = cube_key(cube)
                if (ck, L) in seen:
                    continue
                seen.add((ck, L))
                sc = (bin(cm & rem).count("1"),
                      1 if (pool and ck in poolk) else 0,
                      -D.nlits(cube))
                tb = (ck, str(L))
                if best is None or sc > best[0] or (sc == best[0] and tb < best[3]):
                    best = (sc, cube, cm, tb, L)
        newly = best[2] & rem
        # Espresso REDUCE: keep the same claimed keys, but shrink the cube to
        # their supercube.  Term count is unchanged and overlap collapses, which
        # is what keeps the rank DAG shallow (parse: 27 ranks -> 2).
        # Any cube between the supercube and the prime cube is equally valid, so
        # if another head already pays for one in that window, ALIGN to it (T4).
        cube = best[1]
        if reduce_:
            cube = supercube(D, newly)
            if pool:
                prime, sup = best[1], cube
                for pc in pool:
                    if all(sup[i] <= pc[i] <= prime[i] for i in range(D.m)):
                        cube = pc
                        break
        rules.append((cube, best[4]))
        rem &= ~newly
    return rules


def supercube(D, mask):
    sets = []
    for i in range(D.m):
        sets.append(frozenset(v for v in range(len(D.vocabs[i]))
                              if D.mask[i][v] & mask))
    return tuple(sets)


# ------------------------------------------------- T3: winner-vs-loser ranks --
def ranks(D, rules):
    """W2 weight = 2**rank.  The constraint set is ONLY winner-vs-loser: rule i
    must outrank rule j iff somewhere i is the highest-priority firing rule, j
    also fires, and they disagree.  (Ranking every overlapping pair -- the
    obvious thing -- yields one 59-long chain on abi, i.e. weights to 2**58.)"""
    n = len(rules)
    masks = [D.cube_mask(c) for c, _ in rules]
    fire = [[] for _ in range(D.n)]
    for i in range(n):
        m = masks[i]
        while m:
            b = m & -m
            fire[b.bit_length() - 1].append(i)
            m ^= b
    cons, out = set(), {}

    def add(a, b):
        if a < b and (a, b) not in cons:
            cons.add((a, b))
            out.setdefault(a, []).append(b)
            return True
        return False

    for F in fire:
        if len(F) > 1:
            w = F[0]
            for i in F[1:]:
                if rules[i][1] != rules[w][1]:
                    add(w, i)
    lv = [0] * n
    for _ in range(500):
        lv = [0] * n
        for i in range(n - 1, -1, -1):
            mx = -1
            for b in out.get(i, ()):
                if lv[b] > mx:
                    mx = lv[b]
            lv[i] = mx + 1
        grew = False
        for F in fire:
            if len(F) < 2:
                continue
            w, cstar = F[0], rules[F[0]][1]
            z = {}
            for i in F:
                z[rules[i][1]] = z.get(rules[i][1], 0) + (1 << lv[i])
            for c, v in z.items():
                if c != cstar and v >= z[cstar]:
                    grp = [i for i in F if rules[i][1] == c]
                    for a in range(len(grp)):
                        grew |= add(w, grp[a])
                        for b in range(a + 1, len(grp)):
                            grew |= add(grp[a], grp[b])
        if not grew:
            break
    return lv


# ------------------------------------------------------------- representation
# A representation of one head = list of (cube, {class_index: weight}).

def rep_from_dl(D, rules, cidx):
    lv = ranks(D, rules)
    acc, order = {}, []
    for (cube, L), l in zip(rules, lv):
        k = cube_key(cube)
        if k not in acc:
            acc[k] = (cube, {})
            order.append(k)
        c = cidx[L]
        acc[k][1][c] = acc[k][1].get(c, 0) + (1 << l)
    return [acc[k] for k in order]


def _is_product(tuples, nf):
    proj = [sorted({t[i] for t in tuples}) for i in range(nf)]
    sz = 1
    for p in proj:
        sz *= len(p)
    return sz == len(set(tuples))


def _head_failures(D, classof, units, cidx, ncls):
    masks = [D.cube_mask(c) for c, _ in units]
    bad = []
    for j in range(D.n):
        z = [0] * ncls
        for u, m in enumerate(masks):
            if (m >> j) & 1:
                for c, w in units[u][1].items():
                    z[c] += w
        mx = max(z)
        if z.count(mx) != 1 or z.index(mx) != cidx[classof[j]]:
            bad.append(j)
    return bad


def rep_factored(D, classof, parts, merge, cidx, ncls, dl=None, k=0):
    """T5.  `parts` partitions the field indices.  One unit per group value of
    each part, with W2[unit, c] = 1 iff class c is reachable from that group
    value; the gold class is then the unique class reachable from ALL parts, so
    it alone scores len(parts).  |A|+|B| units replace one cube per class.

    A pure factorisation is wrecked by absorbing classes ("none" is reachable
    from every op and from every (os,arch)), so the first `k` rules of the head's
    decision list are kept as high-rank EXCEPTIONS; reachability is then computed
    only over the keys they do not claim, and whole groups can vanish (every MOP
    drops out of sysno).  Residual collisions are pinned by full-specificity
    patch units at the top rank."""
    if len(parts) < 2:
        return None
    units = []
    covered = 0
    if k:
        if dl is None or k > len(dl):
            return None
        for i in range(k):
            m = D.cube_mask(dl[i][0])
            units.append((dl[i][0], {cidx[dl[i][1]]: 1 << (k + 1 - i)}))
            covered |= m
    resid = D.ALL & ~covered
    if resid == 0:
        return None
    for p in parts:
        g = {}
        for j in range(D.n):
            if not (resid >> j) & 1:
                continue
            gv = tuple(D.kidx[j][i] for i in p)
            g.setdefault(gv, set()).add(classof[j])
        if not g:
            return None
        buckets = {}
        for gv, cs in g.items():
            buckets.setdefault(frozenset(cs), []).append(gv)
        for cs in sorted(buckets, key=lambda s: sorted(map(str, s))):
            gvs = sorted(buckets[cs])
            chunks = [gvs] if (merge and len(gvs) > 1
                               and _is_product(gvs, len(p))) else [[v] for v in gvs]
            for ch in chunks:
                sets = list(D.full)
                for fi, i in enumerate(p):
                    sets[i] = frozenset(t[fi] for t in ch)
                units.append((tuple(sets), {cidx[c]: 1 for c in cs}))
    cap = len(dl) if dl else D.n
    for _ in range(6):
        if len(units) >= cap:
            return None
        bad = _head_failures(D, classof, units, cidx, ncls)
        if not bad:
            return units
        have = {cube_key(c) for c, _ in units}
        for j in bad:
            cube = tuple(frozenset([D.kidx[j][i]]) for i in range(D.m))
            if cube_key(cube) in have:
                return None
            units.append((cube, {cidx[classof[j]]: 1 << (k + 2)}))
            have.add(cube_key(cube))
    return None


def partitions_of(m):
    if m < 2:
        return []
    if m == 2:
        return [[(0,), (1,)]]
    out = []
    for i in range(m):
        out.append([(i,), tuple(x for x in range(m) if x != i)])
    out.append([(i,) for i in range(m)])
    return out


# ---------------------------------------------------------------- assemble ---
def build_net(stage, setvalued=True, share=True, factor=True, reduce_=True):
    D = Domain(stage)
    S = D.S
    heads = [(h[0], list(h[1])) for h in S.heads]
    labels = [S.label(*kv) for kv in D.keys]
    cidx = {hn: {c: i for i, c in enumerate(cl)} for hn, cl in heads}

    cands, dl_rules = {}, {}
    for hn, cl in heads:
        classof = [labels[j][hn] for j in range(D.n)]
        dl_rules[hn] = decision_list(D, classof, None, setvalued, reduce_)
        cands[hn] = [rep_from_dl(D, dl_rules[hn], cidx[hn])]
        if factor:
            for parts in partitions_of(D.m):
                for mg in (False, True):
                    for k in range(0, min(9, len(dl_rules[hn]))):
                        r = rep_factored(D, classof, parts, mg, cidx[hn],
                                         len(cl), dl_rules[hn], k)
                        if r is not None and len(r) < len(cands[hn][0]):
                            cands[hn].append(r)

    def total_units(sel):
        u = set()
        for hn, _ in heads:
            u |= {cube_key(c) for c, _w in cands[hn][sel[hn]]}
        return len(u)

    def pick(chosen):
        for _ in range(5):
            moved = False
            for hn, _ in heads:
                bi, bv = chosen[hn], total_units(chosen)
                for i in range(len(cands[hn])):
                    if i == chosen[hn]:
                        continue
                    trial = dict(chosen); trial[hn] = i
                    v = total_units(trial)
                    if v < bv or (v == bv and
                                  len(cands[hn][i]) < len(cands[hn][bi])):
                        bi, bv = i, v
                if bi != chosen[hn]:
                    chosen[hn] = bi
                    moved = True
            if not moved:
                break
        return chosen

    starts = [{hn: 0 for hn, _ in heads}]
    for hn0, _ in heads:
        st = {hn: 0 for hn, _ in heads}
        st[hn0] = min(range(len(cands[hn0])), key=lambda i: len(cands[hn0][i]))
        starts.append(st)
    chosen = min((pick(dict(s0)) for s0 in starts),
                 key=lambda c: (total_units(c), sum(len(cands[h][c[h]]) for h, _ in heads)))

    if share and len(heads) > 1:
        # T4: rebuild every head's decision list with the cubes the CHOSEN
        # representations already pay for, then re-select.  Two rounds.
        for _ in range(3):
            pool = set()
            for hn, _ in heads:
                pool |= {c for c, _w in cands[hn][chosen[hn]]}
            improved = False
            for hn, cl in heads:
                classof = [labels[j][hn] for j in range(D.n)]
                mine = {cube_key(c) for c, _w in cands[hn][0]}
                rl = decision_list(D, classof,
                                   [c for c in pool if cube_key(c) not in mine],
                                   setvalued, reduce_)
                if len(rl) <= len(dl_rules[hn]):
                    dl_rules[hn] = rl
                    cands[hn][0] = rep_from_dl(D, rl, cidx[hn])
                    improved = True
                    if factor:
                        for parts in partitions_of(D.m):
                            for mg in (False, True):
                                for k in range(0, min(9, len(rl))):
                                    r = rep_factored(D, classof, parts, mg,
                                                     cidx[hn], len(cl), rl, k)
                                    if r is not None and len(r) < len(cands[hn][0]):
                                        cands[hn].append(r)
            if not improved:
                break
            chosen = min((pick(dict(s0)) for s0 in starts),
                         key=lambda c: (total_units(c),
                                        sum(len(cands[h][c[h]]) for h, _ in heads)))

    units, uidx = [], {}
    for hn, _ in heads:
        for cube, _w in cands[hn][chosen[hn]]:
            k = cube_key(cube)
            if k not in uidx:
                uidx[k] = len(units)
                units.append(cube)
    H = len(units)
    offs, o = [], 0
    for v in D.ovocabs:                    # original coords: W1 is lifted
        offs.append(o); o += len(v)
    h0 = o
    # PURE INTEGER.  b1 = -(|F|-1) instead of -(|F|-0.5): all literals match
    # -> pre-activation 1, one literal short -> 0, so ReLU cuts in exactly the
    # same place but the hidden activation is 0/1 and nothing is fractional.
    W1 = [[0] * H for _ in range(h0)]
    b1 = [0] * H
    for j, cube in enumerate(units):
        nf = 0
        for i, s in enumerate(cube):
            if len(s) == len(D.vocabs[i]):
                continue
            nf += 1
            for g in s:                       # lift: a group -> its members
                for v in D.groups[i][g]:
                    W1[offs[i] + v][j] = 1
        b1[j] = -(nf - 1)
    W2, wvals = {}, set()
    for hn, cl in heads:
        M = [[0] * len(cl) for _ in range(H)]
        for cube, wd in cands[hn][chosen[hn]]:
            row = M[uidx[cube_key(cube)]]
            for c, w in wd.items():
                row[c] = int(w)
                wvals.add(w)
        W2[hn] = M
    return dict(D=D, heads=heads, units=units, H=H, h0=h0, offs=offs,
                W1=W1, b1=b1, W2=W2, wvals=wvals, uidx=uidx,
                kinds={hn: ("factored" if chosen[hn] else "dlist")
                       for hn, _ in heads},
                )


# ------------------------------------------------------------------ verify ---
def verify_int(p):
    """PURE INTEGER deploy kernel over the FULL gold corpus.  No float touches
    this path: one-hot int input, int gemv, ReLU as max(0,.), int gemv,
    argmax with TIES REJECTED.  Also measures the accumulator ranges."""
    D, S, bad = p["D"], p["D"].S, []
    mx_pre = mx_log = 0
    for kv, ki in zip(D.okeys, D.okidx):          # FULL original domain
        x = [0] * p["h0"]
        for i, v in enumerate(ki):
            x[p["offs"][i] + v] = 1
        hid = []
        for j in range(p["H"]):
            a = p["b1"][j]
            for i in range(p["h0"]):
                if x[i]:
                    a += p["W1"][i][j]
            if abs(a) > mx_pre:
                mx_pre = abs(a)
            hid.append(a if a > 0 else 0)
        lab = S.label(*kv)
        for hn, cl in p["heads"]:
            M = p["W2"][hn]
            z = [0] * len(cl)
            for j, hv in enumerate(hid):
                if hv:
                    r = M[j]
                    for k in range(len(cl)):
                        z[k] += hv * r[k]
            m = max(z)
            if abs(m) > mx_log:
                mx_log = abs(m)
            if min(z) < -mx_log:
                mx_log = -min(z)
            if z.count(m) != 1 or cl[z.index(m)] != lab[hn]:
                bad.append((kv, hn))
    return bad, mx_pre, mx_log


def verify_float(p):
    """Secondary: the same weights through a float64 kernel, to confirm the
    integer switch changed nothing observable."""
    D, S, bad = p["D"], p["D"].S, []
    for kv, ki in zip(D.keys, D.kidx):
        x = [0.0] * p["h0"]
        for i, v in enumerate(ki):
            x[p["offs"][i] + v] = 1.0
        hid = []
        for j in range(p["H"]):
            a = float(p["b1"][j])
            for i in range(p["h0"]):
                if x[i]:
                    a += float(p["W1"][i][j])
            hid.append(a if a > 0.0 else 0.0)
        lab = S.label(*kv)
        for hn, cl in p["heads"]:
            z = [0.0] * len(cl)
            M = p["W2"][hn]
            for j, hv in enumerate(hid):
                if hv:
                    r = M[j]
                    for k in range(len(cl)):
                        z[k] += hv * float(r[k])
            if cl[argmax(z)] != lab[hn]:
                bad.append((kv, hn))
    return bad


def verify_cubes(p):
    """Third, independent route: evaluate straight from the cube algebra."""
    D, S, bad = p["D"], p["D"].S, []
    cm = [D.cube_mask(c) for c in p["units"]]
    for j, kv in enumerate(D.keys):
        fired = [u for u in range(p["H"]) if (cm[u] >> j) & 1]
        lab = S.label(*kv)
        for hn, cl in p["heads"]:
            M = p["W2"][hn]
            z = [0] * len(cl)
            for u in fired:
                for k in range(len(cl)):
                    z[k] += M[u][k]
            m = max(z)
            if z.count(m) != 1 or cl[z.index(m)] != lab[hn]:
                bad.append((kv, hn))
    return bad


def nonzeros(p):
    """The constructed nets are extremely sparse: W1 is 0/1 with one block per
    literal, W2 has one entry per (unit, head).  Reported next to dense theta
    because the SGD nets are ~100% dense."""
    n = sum(1 for i in range(p["h0"]) for j in range(p["H"]) if p["W1"][i][j])
    n += p["H"]
    n += sum(1 for hn, cl in p["heads"] for row in p["W2"][hn] for w in row if w)
    return n


def theta(p):
    NH = sum(len(cl) for _, cl in p["heads"])
    return p["h0"] * p["H"] + p["H"] + p["H"] * NH


def literal_embed(p):
    """Optional T6 (reported, not used by default): per field keep the one-hot
    input OR swap it for a |V| x L 0/1 literal table, L = number of distinct
    literal sets that field actually uses -- whichever is cheaper.  Tables are
    counted the way prd counts the SGD embedding tables."""
    D = p["D"]
    NH = sum(len(cl) for _, cl in p["heads"])
    w = tbl = 0
    for i in range(D.m):
        ss = {tuple(sorted(c[i])) for c in p["units"]
              if len(c[i]) != len(D.vocabs[i])}
        L = max(1, len(ss))
        if len(D.vocabs[i]) * L + L * p["H"] < len(D.vocabs[i]) * p["H"]:
            w += L; tbl += len(D.vocabs[i]) * L
        else:
            w += len(D.vocabs[i])
    return w, tbl + w * p["H"] + p["H"] + p["H"] * NH


# -------------------------------------------------------------------- main ---
