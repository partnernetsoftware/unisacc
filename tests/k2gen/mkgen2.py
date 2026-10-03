# writes exec/parse2/gen2-manifest.tsv (transitional: segments selected by env fact seg_NAME)
from pathlib import Path
segs = [("types-dimensions", ["dimensions"]), ("types-dimensions-tail", ["dimensions-tail"]),
        ("types-prefix", ["type-prefix", "structure"]), ("types-typedef", ["type-typedef"]), ("types-tail", ["type-tail"]),
        ("startup-marker", ["startup-marker"]), ("staticauto", ["ordinary-staticauto"]),
        ("parameter-declarators", ["parameter-declarators"]),
        ("dispatch-block", ["dispatch*", "block*"]), ("if-loops", ["if*", "switch*", "loops*"]),
        ("sizeof0", ["sizeof0"]), ("sizeof1", ["sizeof1"]), ("sizeof2", ["sizeof2"]), ("sizeof3", ["sizeof3"]),
        ("address", ["address"])]
L = ["# op\tstem\tsection\twhen\tfacts\tfresh\tseq\tbind\topts",
     "# gen2 driver (K2, was exec/parse2/gen2.py build()): transitional segments; gen2.py runs one segment per",
     "# call with env seg_NAME=1 until the Python between segments has moved into rows (tests/k2gen/mkgen2.py)"]
LET = '{"let":{"extra":{},"seqb":{}}}'
row = "call\tcontrol\t-\tfact:seg_{}{}\t-\t-\t-\tcontrol_section=@str:{},statement=@str:{},extra=extra,seqb=seqb\t{}"
for seg, secs in segs:
    for s in secs:
        if s.endswith("*"):
            s = s[:-1]
            L.append(row.format(seg, "&warnings", s + "-warnings" if s in ("block", "if") else s, "STMT.body", LET))
            L.append(row.format(seg, "&!warnings", s, "STMT", LET))
        else:
            L.append(row.format(seg, "", s, "STMT", LET))
Path(__file__).resolve().parents[2].joinpath("exec/parse2/gen2-manifest.tsv").write_text("\n".join(L) + "\n")
