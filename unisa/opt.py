"""-O1/-O2 on the tape: the Python twin of opt_stack() in unisacc_main.c. [H1] [H2]

Both front ends answer table-shaped questions through the same nets, and both
optimise the same way: `tests/optpy.sh` feeds the C front end's -O0 tape
through this module and requires the C front end's -O2 tape, byte for byte.
So every test below is the C code's test, down to which bytes of a line it
looks at -- a looser Python check would be a different optimiser.
"""
import re

_REG = re.compile(r"r(\d+)")
LABEL, RET, JUMP, JUMPZ, CALL, FRAME, SIMPLEK, OTHER = range(8)


def _isal(c):
    return c.isalpha() or c == "_"


def _regs(s):
    """every rN in s whose r is not preceded by a letter or _ -- ol_mask"""
    out = []
    for m in _REG.finditer(s):
        p = m.start()
        if p == 0 or not _isal(s[p - 1]):
            out.append(int(m.group(1)))
    return out


def _word(ln, w):
    if not ln.startswith(" "):
        return False
    rest = ln[2:]
    return rest.startswith(w) and (len(rest) == len(w) or rest[len(w)] == " ")


_INFO = {}


def _info(op):
    """the `opinfo` table's answer for a tape op word, asked once [I2]"""
    if op not in _INFO:
        from .gold import OPINFO_OPS
        key = (op if op in OPINFO_OPS else "other",)
        _INFO[op] = _oracle().ask("opinfo", key)
    return _INFO[op]


def _simple(ln):
    if len(ln) < 3 or not ln.startswith("  "):
        return False
    return _info(ln[2:].split(" ", 1)[0])["simple"] == "1"


def _firstreg(ln):
    p = ln.find(" ", 2)
    if p < 0:
        return -1
    i = p
    while i < len(ln):
        c = ln[i]
        if (c == "r" and not _isal(ln[i - 1]) and i + 1 < len(ln)
                and ln[i + 1].isdigit()):
            j = i + 1
            while j < len(ln) and ln[j].isdigit():
                j += 1
            return int(ln[i + 1:j])
        if c == "[":
            return -1
        i += 1
    return -1


def _writes(ln, z):
    if _word(ln, "store64") or _word(ln, ".st"):
        return False
    if _firstreg(ln) != z:
        return False
    p = ln.find(" ", 2)
    rs = _regs(ln[p:]) if p >= 0 else []
    return z not in rs[1:]


def _names(ln, r):
    return r in _regs(ln)


def _reg(s):
    """ol_reg: the whole of s is rN"""
    if len(s) < 2 or s[0] != "r" or not s[1:].isdigit():
        return -1
    return int(s[1:])


def _push(ln):
    t = "  store64 [r7+0], "
    return _reg(ln[len(t):]) if ln.startswith(t) else -1


def _pop(ln):
    t = "  load64 "
    if not ln.startswith(t):
        return -1
    q = ln.find(",", len(t))
    if q < 0:
        q = len(ln)
    if len(ln) - q != 8 or ln[q:] != ", [r7+0]":
        return -1
    return _reg(ln[9:q])


class _Round:
    def __init__(self, lines, level):
        self.L = lines
        self.level = level
        self.n = len(lines)
        self.lab = {}
        for i, ln in enumerate(lines):
            if (len(ln) > 1 and ln.endswith(":") and not ln.startswith(" ")
                    and not ln.startswith(".")):
                self.lab.setdefault(ln[:-1], i)
        self.zok = {}
        if level >= 2:
            self.zok = {z: True for z in range(2, 6)}
            self._split()
            self._prep()
            for z in range(2, 6):
                if not self._solve(z):
                    self.zok[z] = False

    # ---- liveness, as bl_split / ol_prep / ol_scan / bl_solve ----------
    def _split(self):
        self.bl_of = [0] * self.n
        self.bl_s = []
        cut = True
        for l, ln in enumerate(self.L):
            if not ln.startswith(" "):
                cut = True
            if cut:
                self.bl_s.append(l)
                cut = False
            self.bl_of[l] = len(self.bl_s) - 1
            if ln.startswith(" ") and (_word(ln, "jump") or _word(ln, "jumpz")
                                       or _word(ln, "ret")):
                cut = True
        self.live = {}

    def _target(self, ln):
        p = ln.rfind(" ")
        t = self.lab.get(ln[p + 1:])
        return -1 if t is None else self.bl_of[t]

    def _prep(self):
        K, RM, WM, TG = [], [], [], []
        for ln in self.L:
            rm = wm = 0
            tg = -1
            if not ln.startswith(" "):
                k = LABEL
            elif _word(ln, "ret"):
                k = RET
            elif _word(ln, "jump"):
                k, tg = JUMP, self._target(ln)
            elif _word(ln, "jumpz"):
                k, tg = JUMPZ, self._target(ln)
                for r in _regs(ln):
                    if r < 16:
                        rm |= 1 << r
            elif _word(ln, "call"):
                k, tg = CALL, self._target(ln)
            elif _word(ln, ".frame"):
                k = FRAME
            elif _simple(ln):
                k = SIMPLEK
                m = 0
                for r in _regs(ln):
                    if r < 16:
                        m |= 1 << r
                f = -1
                if not _word(ln, "store64") and not _word(ln, ".st"):
                    f = _firstreg(ln)
                if f >= 0:
                    wm = 1 << f
                    rm = m & ~(1 << f)
                    if not _writes(ln, f):
                        rm |= 1 << f
                else:
                    rm = m
            else:
                k, rm = OTHER, 255
            K.append(k); RM.append(rm); WM.append(wm); TG.append(tg)
        self.K, self.RM, self.WM, self.TG = K, RM, WM, TG

    def _scan(self, z, l):
        live = self.live[z]
        bit = 1 << z
        K, RM, WM, TG = self.K, self.RM, self.WM, self.TG
        while l < self.n:
            k = K[l]
            if k == LABEL:
                return live[self.bl_of[l]]
            if k == RET:
                return 1 if z <= 1 else 0      # r0/r1 carry the result
            if k == JUMP:
                t = TG[l]
                return 1 if t < 0 else live[t]
            if k == JUMPZ:
                if RM[l] & bit:
                    return 1
                t = TG[l]
                if t < 0 or live[t]:
                    return 1
            elif k == CALL:
                t = TG[l]
                if t < 0 or live[t]:
                    return 1
            elif k != FRAME:
                if RM[l] & bit:
                    return 1
                if WM[l] & bit:
                    return 0
            l += 1
        return 1

    def _solve(self, z):
        live = self.live[z] = [0] * len(self.bl_s)
        changed, rounds = True, 0
        while changed and rounds < 64:
            changed = False
            rounds += 1
            for b in range(len(self.bl_s) - 1, -1, -1):
                s = self.bl_s[b]
                v = self._scan(z, s + 1 if not self.L[s].startswith(" ") else s)
                if v and not live[b]:
                    live[b] = 1
                    changed = True
        return not changed

    def dead(self, z, frm):
        if not self.zok.get(z):
            return False
        return self._scan(z, frm) == 0

    # ---- the rewrites --------------------------------------------------
    def local(self, i, out):
        """imm r2, N / sub64 rD, r6, r2 / .ld rD, [rD+0], W -- ol_local"""
        L = self.L
        if i + 2 >= self.n or not self.zok.get(2):
            return False
        a = L[i]
        t = "  imm r2, "
        if not a.startswith(t) or len(a) <= len(t) or not a[len(t):].isdigit():
            return False
        num = a[len(t):]
        b = L[i + 1]
        t = "  sub64 r"
        if not b.startswith(t) or len(b) <= len(t):
            return False
        d = ord(b[9]) - 48
        if d < 0 or d > 7 or d in (2, 6, 7):
            return False
        if len(b) != 9 + 9:
            return False
        q = 10
        if b[q] != "," or b[q + 2] != "r" or b[q + 3] != "6" or \
                b[q + 6] != "r" or b[q + 7] != "2":
            return False
        c = L[i + 2]
        ld, k = 0, 0
        if c.startswith("  .ld r") and len(c) > 7:
            k = 7
            if ord(c[k]) - 48 == d:
                ld = 1
        if ld == 0:
            if not (c.startswith("  load64 r") and len(c) > 10):
                return False
            k = 10
            if ord(c[k]) - 48 != d:
                return False
            ld = 2
        q = k + 1
        if len(c) < q + 8:
            return False
        if c[q] != "," or c[q + 2] != "[" or c[q + 3] != "r" or \
                ord(c[q + 4]) - 48 != d:
            return False
        if c[q + 5] != "+" or c[q + 6] != "0" or c[q + 7] != "]":
            return False
        if ld == 2 and q + 8 != len(c):
            return False
        if ld == 1 and (q + 8 >= len(c) or c[q + 8] != ","):
            return False
        if not self.dead(2, i + 3):
            return False
        if ld == 1:
            out.append("  .ld r%d, [r6-%s]%s" % (d, num, c[q + 8:]))
        else:
            out.append("  load64 r%d, [r6-%s]" % (d, num))
        return True

    def run(self):
        L, n, out, hits, i = self.L, self.n, [], 0, 0
        while i < n:
            if i + 3 < n and L[i] == "  .frame 8":
                x = _push(L[i + 1])
                if x >= 0:
                    j, ok = i + 2, False
                    while j < n and j <= i + 18:
                        y = _pop(L[j])
                        if y >= 0:
                            ok = j + 1 < n and L[j + 1] == "  .frame -8"
                            break
                        if not _simple(L[j]) or _names(L[j], 7):
                            break
                        j += 1
                    z = -1
                    if ok:
                        if any(_names(L[k], y) for k in range(i + 2, j)):
                            ok = False
                        if j > i + 2 and x == y:
                            ok = False
                        if not ok and self.level >= 2:
                            for zz in (3, 4, 5):
                                if z >= 0:
                                    break
                                if zz in (x, y):
                                    continue
                                if any(_names(L[k], zz) for k in range(i + 2, j)):
                                    continue
                                if self.dead(zz, j + 2):
                                    z = zz
                    if ok:
                        if x != y:
                            out.append("  mov r%d, r%d" % (y, x))
                        out.extend(L[i + 2:j])
                        hits += 1
                        i = j + 2
                        continue
                    if z >= 0:
                        out.append("  mov r%d, r%d" % (z, x))
                        out.extend(L[i + 2:j])
                        out.append("  mov r%d, r%d" % (y, z))
                        hits += 1
                        i = j + 2
                        continue
            if self.level >= 2 and self.local(i, out):
                hits += 1
                i += 3
                continue
            out.append(L[i])
            i += 1
        return out, hits


# ---- -O2: the peep table [H2] -------------------------------------------
def _opword(ln):
    return ln[2:].split(" ", 1)[0][:15] if ln.startswith(" ") else ""


def _islab(ln):
    return (not ln.startswith(" ") and not ln.startswith(".") and len(ln) > 1
            and ln.endswith(":"))


def _last(ln):
    return ln[ln.rfind(" ") + 1:]


def _store(ln):
    t = "  store64 ["
    if not ln.startswith(t):
        return -1, None
    q = ln.find("]", 11)
    if q < 0:
        q = len(ln)
    if q + 3 >= len(ln) or ln[q + 1] != "," or ln[q + 2] != " ":
        return -1, None
    return _reg(ln[q + 3:]), ln[11:q]


def _load(ln):
    t = "  load64 "
    if not ln.startswith(t):
        return -1, None
    q = ln.find(",", 9)
    if q < 0:
        q = len(ln)
    y = _reg(ln[9:q])
    if y < 0 or q + 3 >= len(ln) or ln[q + 1] != " " or ln[q + 2] != "[" \
            or ln[-1] != "]":
        return -1, None
    return y, ln[q + 3:len(ln) - 1]


def _immat(ln):
    t = "  imm r"
    if not ln.startswith(t):
        return -1, 0
    q = ln.find(",", 6)
    if q < 0:
        q = len(ln)
    k = _reg(ln[6:q])
    if k < 0 or q + 2 >= len(ln) or ln[q + 1] != " ":
        return -1, 0
    num = ln[q + 2:]
    if len(num) > 18 or not all("0" <= c <= "9" for c in num):
        return -1, 0
    return k, int(num)


def _three(ln):
    if not ln.startswith(" "):
        return None
    p = ln.find(" ", 2)
    if p < 0:
        return None
    p += 1
    q = ln.find(",", p)
    if q < 0:
        q = len(ln)
    d = _reg(ln[p:q])
    if d < 0 or q + 2 >= len(ln):
        return None
    p = q + 2
    q = ln.find(",", p)
    if q < 0:
        q = len(ln)
    sr = _reg(ln[p:q])
    if sr < 0 or q + 2 >= len(ln):
        return None
    t = _reg(ln[q + 2:])
    if t < 0:
        return None
    return d, sr, t


def _movat(ln):
    t = "  mov r"
    if not ln.startswith(t):
        return None
    q = ln.find(",", 6)
    if q < 0:
        q = len(ln)
    d = _reg(ln[6:q])
    if d < 0 or q + 2 >= len(ln):
        return None
    sr = _reg(ln[q + 2:])
    return (d, sr) if sr >= 0 else None


def _rereg(ln, a, b, allr):
    """ln with register a written as b: every token, or only the first
    register token (the destination) [H4] -- as pp_rereg"""
    out, p, done = [], 0, False
    while p < len(ln):
        c = ln[p]
        if (not done and c == "r" and p > 0 and not _isal(ln[p - 1])
                and p + 1 < len(ln) and ln[p + 1].isdigit()):
            q = p + 1
            while q < len(ln) and ln[q].isdigit():
                q += 1
            n = int(ln[p + 1:q])
            out.append("r%d" % b if n == a else ln[p:q])
            p = q
            if not allr:
                done = True
            continue
        out.append(c)
        p += 1
    return "".join(out)


class _Peep(_Round):
    def __init__(self, lines, oracle):
        self.L = lines
        self.level = 2
        self.n = len(lines)
        self.oracle = oracle
        self.lab = {}
        for i, ln in enumerate(lines):
            if _islab(ln):
                self.lab.setdefault(ln[:-1], i)
        self.zok = {z: True for z in range(0, 6)}
        self._split()
        self._prep()
        for z in range(0, 6):
            if not self._solve(z):
                self.zok[z] = False

    def _acls(self, ln):
        w = _opword(ln)
        return _info(w)["acls"] if w else "other"

    def _bcls(self, l):
        if l >= self.n:
            return "none"
        w = _opword(self.L[l])
        return _info(w)["bcls"] if w else "other"

    def _real(self, l):
        while l < self.n and _islab(self.L[l]):
            l += 1
        return l

    def run(self):
        L, n, out, hits, i = self.L, self.n, [], 0, 0
        while i < n:
            A = L[i]
            if not A.startswith(" "):
                out.append(A)
                i += 1
                continue
            a, b, rel = self._acls(A), self._bcls(i + 1), "none"
            t = x = y = -1
            v = 0
            d3 = mv = None
            if _word(A, "jump") or _word(A, "jumpz"):
                name = _last(A)
                u = False
                k = i + 1
                while k < n and _islab(L[k]):
                    if L[k][:-1] == name:
                        u = True
                    k += 1
                if u:
                    rel, b = "to_next", self._bcls(self._real(i + 1))
                else:
                    t = self.lab.get(name, -1)
                    if t >= 0:
                        t = self._real(t + 1)
                        if t < n and _word(L[t], "jump") and _last(L[t]) != name:
                            rel, b = "to_jump", self._bcls(t)
            elif i + 1 < n and _store(A)[0] >= 0:
                x, m = _store(A)
                y, m2 = _load(L[i + 1])
                if y < 0:
                    y, m2 = _store(L[i + 1])
                if y >= 0 and m == m2 and not (m[:1] == "r" and m[1:2] == "7"):
                    rel = "same_slot_same_reg" if x == y else "same_slot"
            elif i + 1 < n and _immat(A)[0] >= 0:
                x, v = _immat(A)
                B = L[i + 1]
                d3 = _three(B) if x <= 5 and not _word(B, "mov") else None
                if d3 is not None:
                    dd, sr, tt = d3
                    if tt == x and sr != x and self.dead(x, i + 2):
                        if v == 0:
                            rel = "const0"
                        elif v == 1:
                            rel = "const1"
                        elif v & (v - 1) == 0:
                            rel = "pow2"
                elif x <= 5:
                    mv = _movat(B)
                    if mv is not None and mv[1] == x and mv[0] != x \
                            and self.dead(x, i + 2):
                        rel = "copy_dead"
            if rel == "none" and i + 1 < n:                     # [H4]
                ma = _movat(A)
                if ma is not None:
                    ya, xa = ma
                    if (ya <= 5 and ya != xa and self.K[i + 1] == SIMPLEK
                            and (self.RM[i + 1] >> ya) & 1
                            and not (self.WM[i + 1] >> ya) & 1
                            and self.dead(ya, i + 2)):
                        rel, y, x = "copy_into", ya, xa
                if rel == "none" and self.K[i] == SIMPLEK and \
                        not _word(A, "store64") and not _word(A, ".st"):
                    db = _firstreg(A)
                    mb = _movat(L[i + 1])
                    if (0 <= db <= 5 and (self.WM[i] >> db) & 1 and mb is not None
                            and mb[1] == db and mb[0] != db
                            and self.dead(db, i + 2)):
                        rel, x, y = "dest_to_mov", db, mb[0]
            if rel == "none" and self.K[i] == SIMPLEK and not _word(A, "store64") \
                    and not _word(A, ".st"):
                dd = _firstreg(A)
                if 0 <= dd <= 5 and (self.WM[i] >> dd) & 1 and self.dead(dd, i + 1):
                    rel = "a_dead"
            if rel == "none":
                out.append(A)
                i += 1
                continue
            act = self.oracle.ask("peep", (a, b, rel))
            if act == "keep":
                out.append(A)
                i += 1
                continue
            hits += 1
            if act == "load_to_mov":
                out.append(A)
                out.append("  mov r%d, r%d" % (y, x))
                i += 2
            elif act == "drop_b":
                out.append(A)
                i += 2
            elif act == "drop_a":
                i += 1
            elif act == "retarget":
                out.append(A[:A.rfind(" ") + 1] + _last(L[t]))
                i += 1
            elif act == "to_mov":
                dd, sr, _ = d3
                out.append("  mov r%d, r%d" % (dd, sr))
                i += 2
            elif act == "to_shl":
                dd, sr, _ = d3
                out.append("  imm r%d, %d" % (x, v.bit_length() - 1))
                out.append("  shl64 r%d, r%d, r%d" % (dd, sr, x))
                i += 2
            elif act == "retarget_dest":
                out.append(_rereg(A, x, y, False))
                i += 2
            elif act == "fold_copy":
                out.append(_rereg(L[i + 1], y, x, True))
                i += 2
            elif act == "fold_imm":
                out.append("  imm r%d, %d" % (mv[0], v))
                i += 2
            else:
                out.append(A)
                i += 1
                hits -= 1
        return out, hits


_ORACLE = []


def _oracle():
    if not _ORACLE:
        from .__main__ import _oracle as mk
        _ORACLE.append(mk("built"))
    return _ORACLE[0]


def optimise(text, level, oracle=None):
    """the tape text at -O<level>, as the C front end writes it"""
    if level <= 0:
        return text
    nl = text.endswith("\n")
    lines = text.split("\n")
    if nl:
        lines = lines[:-1]
    for _ in range(4):
        lines, hits = _Round(lines, level).run()
        if hits == 0:
            break
    if level >= 2:
        o = oracle or _oracle()
        for _ in range(4):
            lines, hits = _Peep(lines, o).run()
            if hits == 0:
                break
    return "\n".join(lines) + ("\n" if nl else "")
