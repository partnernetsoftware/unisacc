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
L.append("call\tcontrol\t-\tfact:seg_startup-guard\tk2-gen2\t-\t-\tcontrol_section=@str:startup-guard,statement=@str:STMT,extra=extra,seqb=seqb\t" + '{"let":{"extra":{"POSSPAN":"POSSPAN"},"seqb":{}}}')
L.append("foreach\t-\t-\tfact:seg_types-word\tk2-gen2\t-\t-\t-\t" + '{"over":"typewords","pre":[["word","it.word"]]}')
L.append(".call\tcontrol\t-\t-\tk2-gen2\t-\t-\tcontrol_section=@str:type-word,statement=@str:STMT,extra=extra,seqb=seqb\t" + '{"let":{"extra":{"word_state":"@str:TS.{word}","word_return":"fresh:P:TS.{word}:r","type_value":"it.value","type_rank_value":"it.rank","word_follow":"it.follow"},"seqb":{}}}')
CC = "call\tcontrol\t-\t{w}\tk2-gen2\t-\t-\tcontrol_section=@str:{s},statement=@str:STMT,extra=extra,seqb=seqb\t"
import json as _j
def cc(s, extra, seqb, w="-", dot=""):
    L.append(dot + CC.format(w=w, s=s) + _j.dumps({"let": {"extra": extra, "seqb": seqb}}, separators=(",", ":")))
# types: TSPEC dispatch label allocated here and exported (gen2 reads env td for the dispatch template)
L.append("let\t-\t-\tfact:seg_types-entry\t-\t-\t-\ttd=fresh:P:TSPEC:b\t-")
cc("type-entry", {"type_dispatch": "$td"}, {}, "fact:seg_types-entry")
# tytail: ckm/resd rows over facts tables (tests/k2gen/mktytail.py)
pre = '{"over":"%s","pre":[["c","it.current"]]}'
L.append("foreach\t-\t-\tfact:seg_tytail\tk2-gen2\t-\t-\t-\t" + pre % "ckmrows")
cc("ckm-row", {"word_state": "it.current", "tail_current": "it.current", "tail_test": "fresh:P:{c}:b", "tail_hit": "it.hit",
               "tail_next": "it.next", "tail_axis": "it.axis"}, {"tail_initial": "@acts:it.initial", "tail_mask": "@acts:it.mask"}, dot=".")
cc("ckm-final", {"tail_current": "ckmfinal.current", "tail_test": "fresh:P:{ckmfinal[current]}:b", "tail_axis": "ckmfinal.axis"},
   {"tail_initial": "@acts:ckmfinal.initial"}, "fact:seg_tytail")
L.append("foreach\t-\t-\tfact:seg_tytail\tk2-gen2\t-\t-\t-\t" + pre.replace('"it.current"]', '"it.current"],["h","it.hit"]') % "resdrows")
cc("resd-row", {"word_state": "it.current", "tail_current": "it.current", "tail_test": "fresh:P:{c}:b", "tail_hit": "it.hit",
                "tail_mask_test": "@str:{h}.mask", "tail_next": "it.next", "tail_axis": "it.axis", "tail_code": "it.code"},
   {"tail_initial": "@acts:it.initial", "tail_mask": "@acts:it.mask"}, dot=".")
cc("resd-final", {"tail_current": "resdfinal.current"}, {"tail_initial": "@acts:resdfinal.initial"}, "fact:seg_tytail")
Path(__file__).resolve().parents[2].joinpath("exec/parse2/gen2-manifest.tsv").write_text("\n".join(L) + "\n")
