"""Run the E2 preprocessor delta with generic (option A) primitives only.

    python3 exec/pp/sim.py delta.json FILE        # -E text on stdout, exit 0
                                                 # or a diagnostic on stderr, exit 1

The executor knows nothing about C.  Machine: research/delta-framework.md s2,
extended as research/e2-pp-delta.md s3 says:

  obs = (q, r, b, t): b = byte of the TOP reader frame at its cursor, or EOF
  (256) at that frame's end; t = control-stack top (a return label, or BOT).
  Each state reads one component and is total over it.

Store:
  W        dictionary int -> int (missing keys read 0); values are 32-bit
           two's-complement (every ALU result is wrapped).
  frames   a stack of reader frames (bytes, attrs, cursor, end).  Frame 0 is
           the pass input x; INPUSHX pushes a view on x, INPUSH a view on a blob.
  blobs    immutable byte strings: the file dictionary (path -> bytes, read-only
           data), and those BLOBSAVE / SBSAVE make.
  intern   byte string -> id (1, 2, ...): hash-consing, nothing else.
  sb       one string builder (for constant strings and paths).
  o, oattr the output and one attribute word per output byte; OT is the
           attribute stamped on bytes that do not carry one.
  e        the diagnostic channel (stderr).

Every action (and nothing else runs):
  ADV | MARK s | JUMP s | LDI s v | COPYW d s | ALU op d a b | ALUI op d a v
  CMP a b | CMPI a v                       r := 0,1,2 for <,=,> (signed 32)
  RLD s                                    r := W[s] if 0<=W[s]<=256 else 256
  LDX d b off | STX b off s                W[d] := W[W[b]+off] / W[W[b]+off] := W[s]
  OUT c | OUTW s | COPY | COPYT | SPAN s | SPANT s | SPAN2 s e
  OLAST (r := last byte of o, 256 if none) | ODROP | OLEN d | OCLR | OSEL k
  SETOT s | XATTR d
  PUSH g | POP
  INTERN d s e | BLOBSAVE d s e | INPUSH b | INPUSHX s | INPUSHXE s e | INPOP
  SBCLR | SBOUT c | SBSPAN s e | SBBLOB b | SBINTERN d | SBSAVE d | SBFIND d
  BLEN d b                                 W[d] := length of blob W[b]
  BYTE d | XLEN d                          W[d] := byte at cursor (0 at end) / end of the top frame
  DIVMOD10 s                               W[s] := W[s] div 10; r := W[s] mod 10 + 10*[quotient == 0]
  SWAP                                     x := o (with attrs); o := empty; frames := [x at 0]
  ACCEPT | REJECT k
ALU ops: add sub mul div rem and or xor shl sar (int32; div/rem truncate
toward zero, x/0 = 0, x%0 = 0, INT_MIN/-1 = INT_MIN, INT_MIN%-1 = 0; shift
count taken mod 32).
"""
import json
import os
import sys

EOF = 256
BUNDLED = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))), "include")
M32 = 0xFFFFFFFF


def w32(v):
    v &= M32
    return v - 0x100000000 if v & 0x80000000 else v


def alu(op, a, b):
    if op == "add":
        return w32(a + b)
    if op == "sub":
        return w32(a - b)
    if op == "mul":
        return w32(a * b)
    if op == "div":
        if b == 0:
            return 0
        q = abs(a) // abs(b)
        return w32(q if (a < 0) == (b < 0) else -q)
    if op == "rem":
        if b == 0:
            return 0
        q = alu("div", a, b)
        return w32(a - q * b)
    if op == "and":
        return w32(a & b)
    if op == "or":
        return w32(a | b)
    if op == "xor":
        return w32(a ^ b)
    if op == "shl":
        return w32(a << (b & 31))
    if op == "sar":
        return w32(a >> (b & 31))
    raise ValueError(op)


class Files:
    """The read-only file dictionary.  Keys are the path strings the delta
    builds; values are bytes.  Backed by the disk (relative to cwd, as the
    reference reads it) -- data, not code."""

    def __init__(self):
        self.cache = {}

    def get(self, path):
        if path not in self.cache:
            try:
                p = path.decode("latin-1")
                if p.startswith("\0hdr/"):       # the bundled copies: include/ of this tree
                    p = os.path.join(BUNDLED, p[5:])
                self.cache[path] = open(p, "rb").read() if os.path.isfile(p) else None
            except (OSError, ValueError):
                self.cache[path] = None
        return self.cache[path]


def load(delta):
    names = list(delta["states"].keys())
    ix = {n: k for k, n in enumerate(names)}
    seqs = []
    for s in delta["seqs"]:
        seqs.append(tuple(tuple(a) for a in s))
    rows = []
    for n in names:
        mode, row = delta["states"][n]
        if mode == "t":
            r = {k: (ix[v[0]], seqs[v[1]]) for k, v in row.items()}
        else:
            r = [None] * 257
            for k, v in row.items():
                r[int(k)] = (ix[v[0]], seqs[v[1]])
        rows.append((mode, r))
    return names, ix, rows


def run(delta, x, srcpath, files=None, cov=None, maxsteps=None, loaded=None):
    names, ix, rows = loaded or load(delta)
    files = files or Files()
    q = ix[delta["start"]]
    r = 0
    W = {}
    stack = []
    blobs = [b"", bytes(srcpath, "latin-1")]      # blob 1: the source path
    blobid = {}
    intern = {}
    xattr = [0] * len(x)
    frames = [[x, xattr, 0, len(x)]]
    o = bytearray()
    oattr = []
    e = bytearray()
    osel = 0
    OT = 0
    sb = bytearray()
    steps = 0
    fr = frames[-1]
    while True:
        steps += 1
        if maxsteps and steps > maxsteps:
            return ("timeout", None, steps)
        mode, row = rows[q]
        if mode == "b":
            i = fr[2]
            key = fr[0][i] if i < fr[3] else EOF
        elif mode == "t":
            key = stack[-1] if stack else "BOT"
        else:
            key = r if 0 <= r <= 256 else 256
        if cov is not None:
            cov.add((q, key))
        q, acts = row[key]
        for a in acts:
            op = a[0]
            if op == "ADV":
                fr[2] += 1
            elif op == "MARK":
                W[a[1]] = fr[2]
            elif op == "JUMP":
                fr[2] = W.get(a[1], 0)
            elif op == "COPYT":
                i = fr[2]
                if i < fr[3]:
                    (o if osel == 0 else e).append(fr[0][i])
                    if osel == 0:
                        oattr.append(OT)
            elif op == "COPY":
                i = fr[2]
                if i < fr[3]:
                    if osel == 0:
                        o.append(fr[0][i])
                        oattr.append(fr[1][i] if fr[1] is not None else OT)
                    else:
                        e.append(fr[0][i])
            elif op == "OUT":
                if osel == 0:
                    o.append(a[1])
                    oattr.append(OT)
                else:
                    e.append(a[1])
            elif op == "LDI":
                W[a[1]] = a[2]
            elif op == "COPYW":
                W[a[1]] = W.get(a[2], 0)
            elif op == "ALU":
                W[a[2]] = alu(a[1], W.get(a[3], 0), W.get(a[4], 0))
            elif op == "ALUI":
                W[a[2]] = alu(a[1], W.get(a[3], 0), a[4])
            elif op == "CMP" or op == "CMPI":
                u = W.get(a[1], 0)
                v = W.get(a[2], 0) if op == "CMP" else a[2]
                r = 0 if u < v else (1 if u == v else 2)
            elif op == "RLD":
                v = W.get(a[1], 0)
                r = v if 0 <= v <= 256 else 256
            elif op == "LDX":
                W[a[1]] = W.get(W.get(a[2], 0) + a[3], 0)
            elif op == "STX":
                W[W.get(a[1], 0) + a[2]] = W.get(a[3], 0)
            elif op == "OUTW":
                c = W.get(a[1], 0) & 255
                if osel == 0:
                    o.append(c)
                    oattr.append(OT)
                else:
                    e.append(c)
            elif op in ("SPAN", "SPANT", "SPAN2"):
                s0 = W.get(a[1], 0)
                s1 = W.get(a[2], 0) if op == "SPAN2" else fr[2]
                s1 = min(s1, fr[3])
                if s0 < s1:
                    if osel == 1:
                        e += fr[0][s0:s1]
                    else:
                        o += fr[0][s0:s1]
                        if op == "SPANT" or fr[1] is None:
                            oattr.extend([OT] * (s1 - s0))
                        else:
                            oattr.extend(fr[1][s0:s1])
            elif op == "OLAST":
                r = o[-1] if o else 256
            elif op == "ODROP":
                if o:
                    o.pop()
                    oattr.pop()
            elif op == "OLEN":
                W[a[1]] = len(o)
            elif op == "OCLR":
                o = bytearray()
                oattr = []
            elif op == "OSEL":
                osel = a[1]
            elif op == "SETOT":
                OT = W.get(a[1], 0)
            elif op == "XATTR":
                i = fr[2]
                W[a[1]] = fr[1][i] if (fr[1] is not None and i < fr[3]) else 0
            elif op == "PUSH":
                stack.append(a[1])
            elif op == "POP":
                stack.pop()
            elif op == "INTERN" or op == "SBINTERN":
                if op == "INTERN":
                    s0 = W.get(a[2], 0)
                    s1 = min(W.get(a[3], 0), fr[3])
                    key = bytes(fr[0][s0:s1]) if s0 < s1 else b""
                else:
                    key = bytes(sb)
                v = intern.get(key)
                if v is None:
                    v = intern[key] = len(intern) + 1
                W[a[1]] = v
            elif op == "BLOBSAVE" or op == "SBSAVE":
                if op == "BLOBSAVE":
                    s0 = W.get(a[2], 0)
                    s1 = min(W.get(a[3], 0), fr[3])
                    data = bytes(fr[0][s0:s1]) if s0 < s1 else b""
                else:
                    data = bytes(sb)
                blobs.append(data)
                W[a[1]] = len(blobs) - 1
            elif op == "INPUSH":
                b = blobs[W.get(a[1], 0)]
                fr = [b, None, 0, len(b)]
                frames.append(fr)
            elif op == "INPUSHX":
                fr = [x, xattr, W.get(a[1], 0), len(x)]
                frames.append(fr)
            elif op == "INPUSHXE":
                fr = [x, xattr, W.get(a[1], 0), min(len(x), W.get(a[2], 0))]
                frames.append(fr)
            elif op == "INPOP":
                frames.pop()
                fr = frames[-1]
            elif op == "SBCLR":
                sb = bytearray()
            elif op == "SBOUT":
                sb.append(a[1])
            elif op == "SBSPAN":
                s0 = W.get(a[1], 0)
                s1 = min(W.get(a[2], 0), fr[3])
                if s0 < s1:
                    sb += fr[0][s0:s1]
            elif op == "SBBLOB":
                sb += blobs[W.get(a[1], 0)]
            elif op == "SBFIND":
                key = bytes(sb)
                if key not in blobid:
                    data = files.get(key)
                    if data is None:
                        blobid[key] = 0
                    else:
                        blobs.append(data)
                        blobid[key] = len(blobs) - 1
                W[a[1]] = blobid[key]
            elif op == "BYTE":
                i = fr[2]
                W[a[1]] = fr[0][i] if i < fr[3] else 0
            elif op == "XLEN":
                W[a[1]] = fr[3]
            elif op == "BLEN":
                W[a[1]] = len(blobs[W.get(a[2], 0)])
            elif op == "DIVMOD10":
                v = W.get(a[1], 0) & M32
                W[a[1]] = v // 10
                r = v % 10 + (10 if v // 10 == 0 else 0)
            elif op == "SWAP":
                x = bytes(o)
                xattr = oattr
                o = bytearray()
                oattr = []
                frames = [[x, xattr, 0, len(x)]]
                fr = frames[-1]
            elif op == "ACCEPT":
                return ("accept", bytes(o), steps)
            elif op == "REJECT":
                return ("reject", (a[1], bytes(e)), steps)
            else:
                raise ValueError(op)


def main():
    delta = json.load(open(sys.argv[1]))
    path = sys.argv[2]
    x = open(path, "rb").read()
    res, val, steps = run(delta, x, path)
    if res == "accept":
        sys.stdout.buffer.write(val)
        return 0
    if res == "reject":
        sys.stderr.buffer.write(val[1])
        return 1
    sys.stderr.write("timeout\n")
    return 3


if __name__ == "__main__":
    sys.exit(main())
