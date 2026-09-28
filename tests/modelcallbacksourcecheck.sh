#!/bin/sh
# Private source model and private token dumper; no shared compiler artifacts.
set -eu
R=$(cd "$(dirname "$0")/.." && pwd)
. "$R/tests/lib.sh"
T=$(scratch)
bound 55 "$R/tests/build_ref.sh" "$T/ref.c" "$T/ref"
bound 20 python3 - "$T/ref.c" "$T/dump.c" <<'PYDUMP'
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_bytes()
a=b'        if (tkind[i] == 2) { __write(1, "=", 1);'
b=b'        if (L == 4 && TOKV[p] == 116 && TOKV[p+1] == 121 && TOKV[p+2] == 112 && TOKV[p+3] == 101 && getenv("UA_TYPESPELL")) { __write(1, "=", 1); __write(1, src + tpos[i], tlen[i]); }\n'+a
assert s.count(a)==1;s=s.replace(a,b)
a=b'    splice();\n    decomment();\n    preprocess();\n    expandsrc();\n    if (lex() < 0) return 1;\n    i = 0;'
b=a.replace(b'    preprocess();',b'    autoinc();\n    preprocess();')
assert s.count(a)==1;s=s.replace(a,b);Path(sys.argv[2]).write_bytes(s)
PYDUMP
bound 20 cc -w -std=c99 -O0 "$T/dump.c" -o "$T/dump"
bound 20 cc -O2 "$R/exec/c/run.c" -o "$T/run"
bound 25 python3 "$R/exec/parse2/gen2.py" "$T/parse.json"
bound 55 python3 "$R/tests/modelcallbacksourcecheck.py" "$T/parse.json" "$T/run" "$T/dump"
