"""Run the E1 lexer delta with the option-A primitives only.

    python3 exec/lex/sim.py delta.json INPUT

INPUT is the lexer's input (the buffer after splice/decomment/preprocess/
expand; exec/lex/mkpre.sh's UA_LEXIN hook writes it).  On accept the output
bytes o go to stdout -- the -dump-tokens format -- and the exit status is 0.
On reject(k) nothing of o is written; stderr gets `REJECT <k> <i>` and the
exit status is 1 (compare.py renders i with the reference's own position map).

The executor knows nothing about C.  Its whole instruction set:
  ADV            i += 1
  MARK s         W[s] := i                 JUMP s     i := W[s]
  COPYW d s      W[d] := W[s]              INC s      W[s] := W[s]+1 mod 2^32
  DIVMOD10 s     W[s] := W[s] div 10;  r := (W[s] mod 10) + 10*[quotient == 0]
  OUT b          append constant byte b
  SPAN s         append x[W[s] .. i)       SPAN2 s e  append x[W[s] .. W[e])
  PUSH g / POP   stack
  ACCEPT / REJECT k
and one step: read obs = (q, r, x[i] or EOF, top or BOT), look up delta,
set q, run the actions in order.
"""
import json
import sys

EOF = 256


def run(delta, x, cov=None):
    S = delta["states"]
    # decode rows once: state -> (mode, list/dict of (next, seq))
    rows = {}
    for name, (mode, row) in S.items():
        rows[name] = (mode, {(int(k) if mode != "t" else k): tuple(v) for k, v in row.items()})
    seqs = [[tuple(a) for a in s] for s in delta["seqs"]]
    n = len(x)
    q, r, i = "DISPATCH", 0, 0
    stack, W, o = [], {}, bytearray()
    steps = 0
    while True:
        steps += 1
        mode, row = rows[q]
        if mode == "b":
            key = x[i] if i < n else EOF
        elif mode == "t":
            key = stack[-1] if stack else "BOT"
        else:
            key = r
        if cov is not None:
            cov.add((q, key))
        q, sid = row[key]
        for a in seqs[sid]:
            op = a[0]
            if op == "ADV":
                i += 1
            elif op == "OUT":
                o.append(a[1])
            elif op == "MARK":
                W[a[1]] = i
            elif op == "JUMP":
                i = W[a[1]]
            elif op == "SPAN":
                o += x[W[a[1]]:i]
            elif op == "SPAN2":
                o += x[W[a[1]]:W[a[2]]]
            elif op == "INC":
                W[a[1]] = (W.get(a[1], 0) + 1) & 0xFFFFFFFF
            elif op == "COPYW":
                W[a[1]] = W.get(a[2], 0)
            elif op == "DIVMOD10":
                v = W.get(a[1], 0)
                W[a[1]] = v // 10
                r = v % 10 + (10 if v // 10 == 0 else 0)
            elif op == "PUSH":
                stack.append(a[1])
            elif op == "POP":
                stack.pop()
            elif op == "ACCEPT":
                return ("accept", bytes(o), steps)
            elif op == "REJECT":
                return ("reject", (a[1], i), steps)
            else:
                raise ValueError(op)


def main():
    delta = json.load(open(sys.argv[1]))
    x = open(sys.argv[2], "rb").read()
    res, val, steps = run(delta, x)
    if res == "accept":
        sys.stdout.buffer.write(val)
        return 0
    sys.stderr.write("REJECT %s %d\n" % val)
    return 1


if __name__ == "__main__":
    sys.exit(main())
