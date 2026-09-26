"""The E2 executor (exec/pp/sim.py, unchanged) with the stage exit contract.

    python3 exec/pipeline/simrun.py delta.json IN

exec/pp/sim.py's main writes only the diagnostic bytes of a reject, which are
empty for E3's `not covered: ...`; here stderr is `REJECT <k>` then the
diagnostic, so the runner can name k.  0 accept, 1 reject, 3 out of steps.
"""
import json
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "pp"))
import sim  # noqa: E402

res, val, _ = sim.run(json.load(open(sys.argv[1])), open(sys.argv[2], "rb").read(), sys.argv[2])
if res == "accept":
    sys.stdout.buffer.write(val)
    sys.exit(0)
if res == "reject":
    sys.stderr.write("REJECT %s\n" % (val[0],))
    sys.stderr.buffer.write(val[1])
    sys.exit(1)
sys.stderr.write("timeout\n")
sys.exit(3)
