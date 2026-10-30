"""The E2 executor (exec/pp/sim.py, unchanged) with the stage exit contract.

    python3 exec/pipeline/simrun.py delta.json IN [--source ORIGINAL.c] [-I DIR]

exec/pp/sim.py's main writes only the diagnostic bytes of a reject, which are
empty for E3's `not covered: ...`; here stderr is `REJECT <k>` then the
diagnostic, so the runner can name k.  0 accept, 1 reject, 3 out of steps.
"""
import json
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "pp"))
import sim  # noqa: E402

source = sys.argv[2]
include_dirs = []
attributes = None
i = 3
while i < len(sys.argv):
    arg = sys.argv[i]
    if arg == "--source":
        i += 1
        if i >= len(sys.argv):
            sys.exit("simrun: missing --source path")
        source = sys.argv[i]
    elif arg == "-I":
        i += 1
        if i >= len(sys.argv):
            sys.exit("simrun: missing -I directory")
        include_dirs.append(sys.argv[i])
    elif arg.startswith("-I") and len(arg) > 2:
        include_dirs.append(arg[2:])
    elif arg == "--attributes":
        i += 1
        if i >= len(sys.argv):
            sys.exit("simrun: missing --attributes path")
        attributes = sys.argv[i]
    else:
        sys.exit("simrun: unknown option " + arg)
    i += 1
files = sim.Files()
if include_dirs:
    files.cache[b"\0cli/include-dir"] = "\n".join(include_dirs).encode("utf-8")
if attributes is not None:
    files.cache[b"\0cli/attributes"] = open(attributes, "rb").read()
try:
    res, val, _ = sim.run(json.load(open(sys.argv[1])), open(sys.argv[2], "rb").read(), source, files)
except (RuntimeError, OSError) as ex:     # a bad table or an unreadable file: 2, as exec/c/run.c
    sys.stderr.write("run: %s\n" % ex)
    sys.exit(2)
if res == "accept":
    sys.stdout.buffer.write(val)
    sys.exit(0)
if res == "reject":
    sys.stderr.write("REJECT %s\n" % (val[0],))
    sys.stderr.buffer.write(val[1])
    sys.exit(1)
sys.stderr.write("timeout\n")
sys.exit(3)
