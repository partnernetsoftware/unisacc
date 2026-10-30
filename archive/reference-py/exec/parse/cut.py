"""Walk the bundled-header prelude declaration by declaration.

  python3 exec/parse/cut.py [LO] [HEADER]      (default LO=7, HEADER=stdio.h)

The prelude is `#include <HEADER>` preprocessed by the reference (/tmp/ua_ref -E),
tokenised and split into top-level declarations (after a depth-0 `;`, or the
`}` that closes a function body).  Cut k is declarations 0..k plus an empty
main; each cut goes through compare.py until the first one that is not equal.
A cut the delta rejects because the reference would auto-include a header
(a prototype such as `int exit();` whose definition comes later) is skipped.
"""
import json, os, re, subprocess, sys, tempfile

REF = os.environ.get("E3REF", "/tmp/ua_ref")
TOK = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[A-Za-z_]\w*|\.?\d[\w.]*'
                 r'|<<=|>>=|\.\.\.|->|\+\+|--|<<|>>|<=|>=|==|!=|&&|\|\||[-+*/%&|^]=|\S')

def prelude(header, tmp):
    f = os.path.join(tmp, "hdr.c")
    open(f, "w").write("#include <%s>\n" % header)
    src = subprocess.run([REF, "-E", f, "-o", "-"], capture_output=True, text=True, timeout=20).stdout
    toks = TOK.findall(src)
    out, cur, depth = [], [], 0
    for i, t in enumerate(toks):
        cur.append(t)
        if t in "({[" and len(t) == 1: depth += 1
        elif t in ")}]" and len(t) == 1: depth -= 1
        if depth: continue
        body = t == "}" and "{" in cur and cur[cur.index("{") - 1] == ")"
        if t == ";" or body:
            out.append(" ".join(cur)); cur = []
    return out

def main():
    lo = int(sys.argv[1]) if len(sys.argv) > 1 else 7
    tmp = tempfile.mkdtemp(prefix="cut")
    out = prelude(sys.argv[2] if len(sys.argv) > 2 else "stdio.h", tmp)
    json.dump(out, open(os.path.join(tmp, "pre.json"), "w"), indent=0)
    for k in range(lo, len(out)):
        f = os.path.join(tmp, "cut%d.c" % k)
        open(f, "w").write("\n".join(out[:k + 1]) + "\nint main() { return 0; }\n")
        r = subprocess.run(["python3", "exec/parse/compare.py", os.environ.get("E3DELTA", "/tmp/e3delta.json"), f],
                           capture_output=True, text=True, timeout=50)
        last = r.stdout.strip().splitlines()
        res = [l for l in last if l.startswith("files")][0]
        print(k, out[k][:40], "|", res, "|", [l for l in last if "reasons" in l or "DIFF" in l][:2])
        if "auto-includes a header" in r.stdout and "not-covered 1" in res:
            continue   # a prototype before its header definition: the reference prepends the header
        if "equal 1" not in res:
            print("blocked at", k, "of", len(out), "--", f); break
    else:
        print("all", len(out), "pass")

if __name__ == "__main__":
    main()
