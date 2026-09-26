#!/usr/bin/env python3
"""Derive the delta table of the toy LL(1) postfix translator from its grammar.

    python3 gen.py table  > toy.tbl      # write the (wildcard-compressed) table
    python3 gen.py states                # list Q and Gamma
    python3 gen.py check DUMP            # T1: DUMP (exec -dump toy.tbl) == delta_ref
                                         #     on the whole finite domain

Grammar (research/delta-framework.md section 4), as a translation scheme:
    S  -> E $                    ($ = EOF, reject 3 otherwise)
    E  -> T E'
    E' -> + T @+ E' | eps
    T  -> F T'
    T' -> * F @* T' | eps
    F  -> ( E ) | d!             (')' missing: reject 2; no alternative: reject 1)
@c emits the constant byte c; d! matches a digit and copies it to the output.

Construction (generic over any grammar in this form):
  Q     = one entry state per nonterminal + one item (A, alt, j) per position
  Gamma = the items that follow a non-tail call (return points)
  entry A : one alternative -> take it; else predict the alternative from b (FIRST sets; eps alternative is the
            default, as in recursive descent); if none and no eps -> reject.
  item at a terminal : match -> [COPY] ADV, next item; else reject(code)
  item at @c         : EMIT c, next item
  item at call B     : tail position -> goto B; else PUSH next item, goto B
  item at end        : return: q' = stack top, POP  (t = BOT: unreachable)
  S's end            : ACC
"""
import sys

EOF = 256
DIG = set(range(ord('0'), ord('9') + 1))

# ('t', byteset, copy, reject_code) | ('n', name) | ('e', byte) ; ('acc',)
def T(chars, code, copy=False):
    return ('t', frozenset(chars), copy, code)

G = {
    'S':  [[('n', 'E'), T([EOF], 3), ('acc',)]],
    'E':  [[('n', 'T'), ('n', "E'")]],
    "E'": [[T([ord('+')], 0), ('n', 'T'), ('e', ord('+')), ('n', "E'")], []],
    'T':  [[('n', 'F'), ('n', "T'")]],
    "T'": [[T([ord('*')], 0), ('n', 'F'), ('e', ord('*')), ('n', "T'")], []],
    'F':  [[T([ord('(')], 0), ('n', 'E'), T([ord(')')], 2)], [T(DIG, 0, copy=True)]],
}
NOALT = {'F': 1}          # reject code when no alternative predicts
START = 'S'
NR = 3                    # executor minimum; this delta never sets r

def first(alt, seen=()):
    """FIRST of an alternative (bytes); None in the set marks nullable."""
    out = set()
    for s in alt:
        if s[0] == 't':
            return out | set(s[1])
        if s[0] == 'n':
            f = set()
            for a in G[s[1]]:
                f |= first(a)
            out |= f - {None}
            if None not in f:
                return out
    return out | {None}

# ---- states ----
Q = [('entry', A) for A in G]
for A in G:
    for ai, alt in enumerate(G[A]):
        for j in range(len(alt) + 1):
            Q.append((A, ai, j))
QI = {s: k for k, s in enumerate(Q)}
GAMMA = []
for A in G:
    for ai, alt in enumerate(G[A]):
        for j, s in enumerate(alt):
            if s[0] == 'n' and j < len(alt) - 1:
                GAMMA.append((A, ai, j + 1))
GI = {s: k for k, s in enumerate(GAMMA)}
NG = len(GAMMA)
BOT = NG
DEFAULT = (QI[('entry', START)], [(1, 255)])      # unreachable obs: reject 255

def delta_ref(q, r, b, t):
    """The reference delta on one observation: (q', [(code, params...)])."""
    s = Q[q]
    if s[0] == 'entry':
        A = s[1]
        if len(G[A]) == 1:                    # nothing to predict
            return (QI[(A, 0, 0)], [])
        eps = None
        for ai, alt in enumerate(G[A]):
            f = first(alt)
            if b in f:
                return (QI[(A, ai, 0)], [])
            if None in f and eps is None:
                eps = ai
        if eps is not None:
            return (QI[(A, eps, 0)], [])
        return (q, [(1, NOALT[A])])
    A, ai, j = s
    alt = G[A][ai]
    if j == len(alt):
        if t == BOT:
            return DEFAULT
        return (QI[GAMMA[t]], [(4,)])
    sym = alt[j]
    nxt = QI[(A, ai, j + 1)]
    if sym[0] == 'acc':
        return (q, [(0,)])
    if sym[0] == 'e':
        return (nxt, [(5, sym[1])])
    if sym[0] == 't':
        if b in sym[1]:
            return (nxt, ([(6,)] if sym[2] else []) + [(2,)])
        return (q, [(1, sym[3])])
    B = QI[('entry', sym[1])]
    if j == len(alt) - 1:
        return (B, [])
    return (B, [(3, GI[(A, ai, j + 1)])])

def fmt(e):
    q, acts = e
    return ' '.join(str(v) for v in [q, len(acts)] + [p for a in acts for p in a])

def table():
    """Compressed rows: delta_ref depends on b only via finitely many classes
    and on t only in return items; emit wildcard rows, then byte overrides."""
    rows = []
    for q in range(len(Q)):
        if Q[q][0] != 'entry' and Q[q][2] == len(G[Q[q][0]][Q[q][1]]):
            for t in range(NG):
                rows.append((q, -1, -1, t, delta_ref(q, 0, 0, t)))
            continue
        base = delta_ref(q, 0, -1, BOT)       # b = -1 is in no byte set
        rows.append((q, -1, -1, -1, base))
        for b in range(257):
            e = delta_ref(q, 0, b, BOT)
            if e != base:
                rows.append((q, -1, b, -1, e))
    out = ['# toy LL(1) postfix translator; generated by exec/toy/gen.py',
           '# NQ NR NG q0', '%d %d %d %d' % (len(Q), NR, NG, QI[('entry', START)]),
           '# default entry', fmt(DEFAULT), '# rows: q r b t  q\' n acts (-1 = any)',
           str(len(rows))]
    for q, r, b, t, e in rows:
        out.append('%d %d %d %d  %s' % (q, r, b, t, fmt(e)))
    return '\n'.join(out) + '\n'

def check(path):
    got = open(path).read().split('\n')
    k = 0
    for q in range(len(Q)):
        for r in range(NR):
            for b in range(257):
                for t in range(NG + 1):
                    want = '%d %d %d %d %s' % (q, r, b, t, fmt(delta_ref(q, r, b, t)))
                    if k >= len(got) or got[k] != want:
                        print('T1 MISMATCH at', q, r, b, t, repr(got[k] if k < len(got) else None), want)
                        return 1
                    k += 1
    if got[k:] not in ([], ['']):
        print('T1 MISMATCH: extra lines')
        return 1
    print('T1 ok: %d observations (|Q|=%d |R|=%d |b|=257 |t|=%d)' % (k, len(Q), NR, NG + 1))
    return 0

if __name__ == '__main__':
    cmd = sys.argv[1] if len(sys.argv) > 1 else 'table'
    if cmd == 'table':
        sys.stdout.write(table())
    elif cmd == 'states':
        for k, s in enumerate(Q):
            print('q%d' % k, s)
        for k, s in enumerate(GAMMA):
            print('g%d' % k, s)
    elif cmd == 'check':
        sys.exit(check(sys.argv[2]))
