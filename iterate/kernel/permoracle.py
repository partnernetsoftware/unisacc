"""Python ORACLE for a permuted stage order [J10 step 3, check.sh perm].

    python3 iterate/kernel/permoracle.py ORDER.tsv OUTDIR

Runs ckernel.emit_core over weights/built.json with ckernel's module-level ALL
replaced by the stage list of ORDER.tsv (which must be a permutation of
gold.ALL), in this process only.  Nothing on disk changes except OUTDIR.
Oracle only: genmodel never reads what this writes."""
import os, sys
R = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, R)
os.chdir(R)
from unisa import gold, ckernel, intnet
order = [l.rstrip("\n").split("\t")[1] for l in open(sys.argv[1])
         if l.startswith("stage\t")]
assert sorted(order) == sorted(gold.ALL), "not a permutation of gold.ALL"
ckernel.ALL = tuple(order)
os.makedirs(sys.argv[2], exist_ok=True)
nets = intnet.load_all(os.path.join(R, "weights", "built.json"))
ckernel.emit_core(nets, os.path.join(sys.argv[2], "unisa_core.c"))
print("permoracle: %s" % " ".join(order))
