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
# tytail: ckm/resd rows over facts tables (labels, axis, mask text; first-row initial acts from operator-actions)
_ini = {l.split("\t")[1]: _j.loads(l.split("\t")[4]) for l in (Path(__file__).resolve().parents[2] / "exec/parse2/operator-actions.tsv").read_text().split("\n")
        if l.startswith("initial\t")}
_d = lambda v: _j.dumps(v, separators=(",", ":"))
for k, more in (("ckm", ""), ("resd", ',["h","it.hit"]')):
    L.append("foreach\t-\t-\tfact:seg_tytail\tk2-gen2\t-\t-\t-\t" + '{"over":"%srows","pre":[["c","it.current"]%s]}' % (k, more))
    L.append(".let\t-\t-\t-\tk2-gen2\t-\t-\ttm=@out:=it.masktext\t" + _d({"mapseq": {"ti": [{"over": "it.first", "acts": _ini[k]}]}}))
    e = {"word_state": "it.current", "tail_current": "it.current", "tail_test": "fresh:P:{c}:b", "tail_hit": "it.hit",
         "tail_next": "it.next", "tail_axis": "it.axis"}
    if k == "resd":
        e.update(tail_mask_test="@str:{h}.mask", tail_code="it.code")
    cc(k + "-row", e, {"tail_initial": "$ti", "tail_mask": "$tm"}, dot=".")
    L.append("let\t-\t-\tfact:seg_tytail\tk2-gen2\t-\t-\t-\t" + _d({"mapseq": {"ti": [{"over": k + "final.first", "acts": _ini[k]}]}}))
    if k == "ckm":
        cc("ckm-final", {"tail_current": "ckmfinal.current", "tail_test": "fresh:P:{ckmfinal[current]}:b", "tail_axis": "ckmfinal.axis"},
           {"tail_initial": "$ti"}, "fact:seg_tytail")
    else:
        cc("resd-final", {"tail_current": "resdfinal.current"}, {"tail_initial": "$ti"}, "fact:seg_tytail")
# optail: per operator (facts k2-gen2 oprows = operator domain data); states and sequences built here
_P2 = Path(__file__).resolve().parents[2] / "exec/parse2"
_st = [l.split("\t") for l in (_P2 / "operator-states.tsv").read_text().split("\n")[1:] if l]
_act = {l.split("\t")[1]: _j.loads(l.split("\t")[4]) for l in (_P2 / "operator-actions.tsv").read_text().split("\n")
        if l and not l.startswith("#") and l.startswith("operator\t")}
def _subst(a):
    if isinstance(a, list) and len(a) == 2 and a[0] == "constant":
        return {"CKT": "$CKT", "RST": "$RST", "operator_offset": "{offset}"}[a[1]]
    return [_subst(x) for x in a] if isinstance(a, list) else a
_seqs = {k: [{"acts": [_subst(x) for x in v]}] for k, v in _act.items()}
_seqs["operator_signed"] = [{"acts": [["@out", "  {sop} r0, {sregs}\n"]]}]
_seqs["operator_unsigned"] = [{"acts": [["@out", "  {uop} r0, {uregs}\n"]]}]
def opx(extra=None, seqb=None, sec="", w="-", dot="."):
    e = {n: "@str:OPX.{op}" + ("" if x == "-" else x) for n, x in _st}
    e.update(axis_illegal="AXILL", axis_f64="AXF64", integer_entry="$ient", pointer_compare_target="$pct",
             word_state="@str:OPX.{op}." + sec)
    e.update(extra or {})
    q = {k: "$" + k for k in _seqs}
    q.update(seqb or {})
    L.append(dot + CC.format(w=w, s="operator-" + sec) + _j.dumps({"let": {"extra": e, "seqb": q}}, separators=(",", ":")))
pre = [[k, "o." + k] for k in ("op", "offset", "sop", "sregs", "uop", "uregs", "ptr")]
L.append("foreach\t-\t-\tfact:seg_optail\tk2-gen2\t-\t-\t-\t" + _j.dumps({"over": "oprows", "as": "o", "pre": pre}, separators=(",", ":")))
L.append(".let\t-\t-\t-\tk2-gen2\t-\t-\tinvseq=@out:=INVTEXT,ient=@str:OPX.{op}.r,pct=@str:DEAD.pa\t" + _j.dumps({"mapseq": _seqs}, separators=(",", ":")))
L.append('.foreach\t-\t-\t-\t-\t-\t-\t-\t{"over":"o.ptrn","as":"x"}')
L.append("..let\t-\t-\t-\t-\t-\t-\tient=@str:OPX.{op}.n\t-")
L.append('.foreach\t-\t-\t-\t-\t-\t-\t-\t{"over":"o.cmpl","as":"x"}')
L.append("..let\t-\t-\t-\t-\t-\t-\tpct=@str:OPX.{op}.r\t-")
opx(sec="prefix")
L.append('.foreach\t-\t-\t-\t-\t-\t-\t-\t{"over":"o.fl","as":"x"}')
opx(sec="float-select", dot="..")
L.append('.foreach\t-\t-\t-\t-\t-\t-\t-\t' + _j.dumps({"over": "o.floats", "as": "f", "pre": [["fop", "f.fop"], ["fregs", "f.fregs"], ["sfx", "f.sfx"]]}, separators=(",", ":")))
L.append("..let\t-\t-\t-\t-\t-\t-\t-\t" + _j.dumps({"mapseq": {"float_opcode": [{"acts": [["@out", "  {fop} r0, {fregs}\n"]]}],
                                                    "float_invert": [{"over": "o.invl", "acts": ["$invseq"]}]}}, separators=(",", ":")))
opx({"float_entry": "@str:OPX.{op}.{sfx}", "float_convert": "@str:TO.{sfx}", "float_result": "f.result",
     "word_state": "@str:OPX.{op}.float-body{sfx}"}, {"float_opcode": "$float_opcode", "float_invert": "$float_invert"},
    sec="float-body", dot="..")
L.append('.foreach\t-\t-\t-\t-\t-\t-\t-\t{"over":"o.nf","as":"x"}')
opx(sec="float-reject", dot="..")
opx(sec="pointer-{ptr}")
opx(sec="integer")
# ladder (E then C): levels from facts k2-gen2 ladder; up/next labels built here; rejects from ladder-reject.tsv
def ladder_rows(pf, bottom, gate):
    L.append("foreach\t-\t-\t%s\tk2-gen2\t-\t-\t-\t" % gate + _d({"over": "ladder", "as": "L", "pre": [["lv", "L.lv"], ["nm", "@str:%s{lv}" % pf]]}))
    for branch in ("mid", "last"):
        L.append(".foreach\t-\t-\t-\t-\t-\t-\t-\t" + _d({"over": "L." + branch, "as": "x", "pre": [["n", "L.nxt"]]}))
        if branch == "mid":
            up, nx, sec = "@str:%s{n}" % pf, "@str:E{n}", "ladder-up"
        else:
            up, nx, sec = ("@str:" + bottom if bottom else None), "@str:UNARY", ("ladder-up" if bottom else "ladder-empty")
        b = {"ladder_owner": "nm", "ladder_loop": "@str:{nm}.l", "ladder_up": up, "ladder_next": nx}
        cc(sec, dict(b, word_state="nm"), {}, dot="..")
        L.append("..let\t-\t-\t-\t-\t-\t-\tdisp=fresh:P:{nm}:b\t-")
        b["ladder_dispatch"] = "$disp"
        cc("ladder-read", dict(b, word_state="nm"), {}, dot="..")
        L.append("..template\tdispatch\tladderop\t-\t-\tP:{disp}\t-\t-\t" + _d({"let": {"ctx": [{"dispatch": "$disp", "owner": "nm"}], "op": "L.ops"}}))
        L.append("..foreach\t-\t-\t-\t-\t-\t-\t-\t" + _d({"over": "L.ops", "as": "q", "pre": [["o", "q.op"], ["mode", "q.mode"]]}))
        L.append("..." + CC.format(w="-", s="ladder-{mode}") + _d({"let": {"extra": dict(b, ladder_operator="@str:{nm}.{o}", ladder_tail="@str:OPX.{o}", word_state="@str:{nm}.{o}"), "seqb": {}}}))
_rej = [l.split("\t") for l in (Path(__file__).resolve().parents[2] / "exec/parse2/ladder-reject.tsv").read_text().split("\n")[1:] if l]
def reject_rows(pf, gate):
    for owner, state, message in _rej:
        if owner in ("all", pf):
            L.append("rows\tladder-reject\tmain\t%s\t-\t-\t-\treject_state=@str:%s\t" % (gate, state) + _d({"mapseq": {"reject": [{"acts": [["REJECT", message]]}]}}))
ladder_rows("E", "UNARY", "fact:seg_ladder-E")
reject_rows("E", "fact:seg_ladder-E-reject")
ladder_rows("C", None, "fact:seg_ladder-C")
reject_rows("C", "fact:seg_ladder-C-reject")
# startup-run: startup_data built by mapseq (type table rows, init actions, interned names), header from facts
_R = Path(__file__).resolve().parents[2] / "exec/parse2"
_sa = {l.split("\t")[0]: _j.loads(l.split("\t")[4]) for l in (_R / "startup-actions.tsv").read_text().split("\n") if l and not l.startswith("#")}
_names = [l.split("\t") for l in (_R / "startup-names.tsv").read_text().split("\n") if l and not l.startswith("#")]
def _intern(reg, name):
    return [["SBCLR"], ["@bytes", name], ["SBINTERN", reg]]
def _lit(group):
    return [{"acts": [a for g, reg, nm in _names if g == group for a in _intern(reg, nm)]}]
assert _sa["auto"] == [["LDI","u",1],["STX","t",["constant","AUT"],"u"]]
_sd = ([{"over": "tyrows", "acts": [["LDI", "t", "{t}"], ["LDI", "u", "{u}"], ["STX", "t", "{tab}", "u"]]}, {"acts": _sa["registers"]}]
       + _lit("head") + [{"acts": _sa["position"]}] + _lit("function")
       + [{"over": "syscalls", "acts": _intern("sy{i}", "{name}")}] + _lit("builtins")
       + [{"over": "autonames", "acts": _intern("t", "{name}") + [["LDI", "u", 1], ["STX", "t", "$AUT", "u"]]}])
L.append("let\t-\t-\tfact:seg_startup-run\tk2-gen2\t-\t-\t-\t" + _d({"mapseq": {"startup_data": _sd}}))
L.append("call\tcontrol\t-\tfact:seg_startup-run\tk2-gen2\t-\t-\tcontrol_section=@str:startup-run,statement=@str:STMT,control_export=@str:1,extra=extra,seqb=seqb\t"
         + _d({"let": {"extra": {}, "seqb": {"startup_data": "$startup_data", "startup_header": "@out:=HEADER"}}, "merge": True}))
# function/global/local control: per section, fresh lets (file order) then control calls; mode rows gated by warnings
def _tab(name):
    ls = [l for l in (_R / name).read_text().split("\n") if l and not l.startswith("#")]
    return [l.split("\t") for l in ls[1:]]
_NSC = {"function": ["PIDS", "PDB", "LOC", "FND", "FRD", "FRB", "VAR"] + ["FN_PDB%d" % i for i in range(16)],
        "global": ["LOC", "GIBLOB", "GIEND", "GINPS", "GINPE", "GSZ", "GUNIT", "SINIT", "SKIPS", "FND", "GMARK", "BASE", "ARR", "PTR"],
        "local": ["SKIPS", "PTR", "BASE"]}
for ns in ("function", "global", "local"):
    fr, secs = _tab(ns + "-fresh.tsv"), _tab(ns + "-sections.tsv")
    fk = "globals" if ns == "global" else ns
    for sec in dict.fromkeys(r[0] for r in fr + secs):
        for w, mode in (("&!warnings", "plain"), ("&warnings", "warnings")):
            gate = "fact:seg_ns-%s-%s%s" % (ns, sec, w)
            keys = []
            for part, m, prefix, kind, key in fr:
                if part == sec and m in ("common", mode):
                    L.append("let\t-\t-\t%s\t-\t-\t-\t%s=fresh:P:%s.%s_%s:%s\t-" % (gate, key, prefix, ns, key, kind))
                    keys.append(key)
            extra = {n: "nsconst.%s.%s" % (fk, n) for n in _NSC[ns]}
            extra.update({k: "$" + k for k in keys})
            for owner, m, rules in secs:
                if owner == sec and m in ("common", mode):
                    cc(rules, extra, {}, gate)
# ordinary control: per section fresh lets + control calls; caller values (update_*, resume) arrive in env
_ofr, _osec = _tab("ordinary-fresh.tsv"), _tab("ordinary-sections.tsv")
_otext = {n: "@text:" + v for n, v in _tab("ordinary-text.tsv")}
_UPD = ["word_state", "update_entry", "update_deref", "update_post"]
_OEX = {"update-address": _UPD, "update-result": _UPD + ["resume"], "down": ["resume"]}
for sec in dict.fromkeys(r[0] for r in _ofr + _osec):
    for w, mode in (("&!warnings", "plain"), ("&warnings", "warnings")):
        gate = "fact:seg_ord-%s%s" % (sec, w)
        extra = {n: "ordconst." + n for n in ("DBL", "FLT", "GMARK", "bottom")}
        extra.update({k: "$" + k for k in _OEX.get(sec, [])})
        for part, m, owner, kind, key in _ofr:
            if part == sec and m in ("common", mode):
                L.append("let\t-\t-\t%s\t-\t-\t-\t%s=fresh:U:%s:%s\t-" % (gate, key, owner, kind))
                extra[key] = "$" + key
        for owner, m, rules in _osec:
            if owner == sec and m in ("common", mode):
                cc(rules, extra, _otext, gate)
Path(__file__).resolve().parents[2].joinpath("exec/parse2/gen2-manifest.tsv").write_text("\n".join(L) + "\n")
