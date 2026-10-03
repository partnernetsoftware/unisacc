import json, re, sys
sys.argv = ['x']; sys.path[:0] = ['exec/parse2', 'exec', 'exec/facts']
import gen2 as G
E = G.E
J = lambda v: json.dumps(v, separators=(',', ':'))
R = 'exec/parse2/'
rows = lambda s: [l.split('\t') for l in open(R + 'callcontrol-' + s + '.tsv').read().splitlines()[1:]]
SYS = G.SYSCALLS
consts = dict(LOC=G.LOC, SBB=G.SBB, SSZ=G.SSZ, FPS_FN=G.FPS_FN, PDB=G.PDB, DBL=G.DBL, FLT=G.FLT, BOOL=G.BOOL, TIX=G.TIX)
consts.update(GMARK=E.GMARK, FND=E.FND, FRB=E.FRB, FRD=E.FRD, VAR=E.VAR, VS_TOP=E.VS - 1,
              WF_FORMAT=51 << 40, find_end='CL.b%d' % (len(SYS) + 1))
formats = {n: json.loads(v) for n, v in rows('format')}
seqs = {n: E.O(json.loads(v)) for n, v in rows('text')}
for n, v in rows('template'):
    t, i = json.loads(v); seqs[n] = E.O(re.split(r'(\{[^}]*\})', G.TEMPL[t])[i])
p = G.P('callcontrol.k2facts')
for n, v in rows('stack'):
    m, sl = json.loads(v); p.acts = []; seqs[n] = getattr(p, m)(*sl).acts
E.WORDS.append("type=extern"); E.TK["type=extern"] = max(E.TK.values()) + 1
E.WORDS.append("type=_Bool"); E.TK["type=_Bool"] = max(E.TK.values()) + 1
E.tokenizer(("type=const", "type=volatile", "type=restrict", "type=inline"))
tokens = dict(E.TK, identifier=E.TK_ID)
classes = {n: [tokens[t]] for n, t in rows('tokens')}
fpu = G.FPU
texts = {}
for k in range(5):
    texts['save%d' % k] = E.O('  load64 r2, [r7+%d]\n  store64 [r0+%d], r2\n' % (8 * k, 24 + 8 * k))
    texts['restore%d' % k] = E.O('  load64 r2, [r1+%d]\n  store64 [r7+%d], r2\n' % (24 + 8 * k, 8 * k))
for suffix in 'ds':
    texts['sqrt' + suffix] = E.O(formats['sqrt'] % fpu[suffix + 'sqrt'])
for k, (_, op, width) in enumerate(SYS, 1):
    regs = ', '.join('r%d' % i for i in range(width))
    if op == 'hostcall':
        t = '  imm r2, 0\n  .hostcall r0, r1\n'
    elif op.startswith('hostaddr'):
        t = '  .hostaddr r0, %s\n' % op[-1]
    else:
        t = formats['syscall'] % ('6' if width == 6 else '', op, regs)
    texts['sys%d' % k] = E.O(t)
open('exec/facts/k2-call.tsv', 'w').write(
    "# call control facts (was callcontrol.py): constants, sequences, token classes, per-instance texts\n"
    "=consts\tjson\t%s\n=seqs\tjson\t%s\n=classes\tjson\t%s\n=convclasses\tjson\t%s\n=texts\tjson\t%s\n"
    "=tok\tjson\t%s\n=n\tjson\t%s\n=nil\tjson\t[]\n" % (
        J(consts), J(seqs), J(classes), J({k: [consts[k]] for k in ('DBL', 'FLT', 'BOOL')}), J(texts),
        J({t: E.TK[t] for t in '(),'}), J(list(range(128)))))
fresh = rows('fresh')
sections = {l.split('\t')[0] for l in open(R + 'callcontrol-result.tsv').read().splitlines()[1:]}
H = "# op\tstem\tsection\twhen\tfacts\tfresh\tseq\tbind\topts"
F = "k2-call"
sticky = {}
def seqcells():
    return ",".join("%s=@acts:texts.%s" % kv for kv in sticky.items()) or "-"
def ln(*c):
    out.append("\t".join(c))
def sec(name, cells="-", export=None, first=None):
    wmodes = any(r[0] == name and r[1] == 'warnings' for r in fresh)
    variants = [("!warnings", {"section": name, "mode": "all"}), ("warnings", {"section": name})] if wmodes else [("-", {"section": name})]
    for when, where in variants:
        o = {"accumulate": "cb", "cellsfirst": True, "freshrows": [{"file": "callcontrol-fresh.tsv", "where": where,
             "key": "{key}", "owner": "{owner}", "kind": "{kind}", "holder": True, "lookup": True}]}
        if first:
            o["bindmap"] = first
        if export:
            o["export"] = export
        if name + '.all' in sections:
            o.update(seqlist=["seqs"], classes="classes")
            ln("rows", "callcontrol", name + ".all", when, F, "-", seqcells(), cells, J(o))
        else:
            ln("let", "-", "-", when, F, "-", "-", cells, J(o))
    for mode, when in (("warnings", "warnings"), ("plain", "!warnings")):
        if name + '.' + mode in sections:
            ln("rows", "callcontrol", name + "." + mode, when, F, "-", seqcells(), "-",
               J({"accumulate": "cb", "seqlist": ["seqs"], "classes": "classes"}))
def addr(entry, key):
    ln("call", "addr", "-", "-", F, "-", "-", "entry=%s,pending=nil" % entry, '{"result":"addrenv"}')
    ln("let", "-", "-", "-", "-", "-", "-", "%s=$$addrenv:done" % key, '{"accumulate":"cb"}')
def tmpl(section, fresh_spec, let, seq=False):
    o = {"let": let}
    if seq:
        o["seqlist"] = ["seqs"]
    ln("template", "callcontrol-setjmp", section, "-", F, fresh_spec, seqcells() if seq else "-", "-", J(o))
S = lambda t: "@str:" + t
# ---- begin
out = [H, "# call control, phase begin (was callcontrol.py): accumulated bindings kept in env cb for the finish phase"]
sec('part0', first="consts")
addr("@str:CL.fpg", "f_part1_1427_CL_ad_1")
sec('part2'); sec('va0')
for k, nx in ((1, 'CL.va2'), (2, 'CL.b1')):
    sec('van', "va_entry=@str:CL.va%d,va_target=@str:VA%d,va_next=@str:%s,va_register=@str:va%d" % (k, k, nx, k))
sec('part4', export=['f_part4_1442_VA0_r_1'])
addr("$f_part4_1442_VA0_r_1", "f_part5_1443_VA0_ad_1")
sec('part6', export=['f_part6_1451_VA1_r_1'])
addr("$f_part6_1451_VA1_r_1", "f_part7_1452_VA1_ad_1")
sec('part8')
ln("rows", "varargs", "aggregate", "-", "-", "-", "-", "-", '{"accumulate":"cb"}')
sec('part10')
for k in range(1, len(SYS) + 1):
    nxt = 'CL.b%d' % (k + 1) if k < len(SYS) else 'CL.sj0'
    sec('sysfind', "find_entry=@str:CL.b%d,find_hit=@str:CL.s%d,find_next=@str:%s,sys_register=@str:sy%d,sys_index=n.%d" % (k, k, nxt, k, k))
for k in range(6):
    sj = dict(entry=S('CL.sj%d' % k), hit=S('CL.sj' if k < 3 else 'CL.lj'),
              next=S('CL.sj%d' % (k + 1) if k < 5 else 'CL.b%d' % (len(SYS) + 1)), register=S('sj%d' % k))
    tmpl('sjscan', 'P:CL.sj%d' % k, {"sj": [sj]})
def holder(name):
    ln("holder", "h_" + name, "-", "-", "-", "P:" + name, "-", "-", "-")
def fr(name, kinds):
    return {key: "fresh:=h_%s:%s" % (name, kind) for key, kind in kinds}
holder('CL.sj')
sj = dict(entry=S('CL.sj'), **fr('CL.sj', [('open_ok', 'e'), ('open_test', 'b'), ('next_ret', 'r'), ('expr_ret', 'r'), ('close_ok', 'e'), ('close_test', 'b'), ('depth_ret', 'r')]))
sj.update(open_token="tok.(", close_token="tok.)")
tmpl('save-entry', '=h_CL.sj', {"sj": [sj]}, True)
for k in range(5):
    sticky['sj_text'] = 'save%d' % k
    tmpl('sjframe', 'P:SJ.save%d' % k, {"sj": [dict(state=S('SJ.save%d' % k), emit=S('SJ.saveemit%d' % k), next=S('SJ.save%d' % (k + 1)), threshold=8 * (k + 1))]}, True)
holder('SJ.save5')
tmpl('restore-entry', '=h_SJ.save5', {"save": [dict(entry=S('SJ.save5'), **fr('SJ.save5', [('first_ret', 'r'), ('second_ret', 'r'), ('third_ret', 'r')]))]}, True)
for k in range(5):
    sticky['sj_text'] = 'restore%d' % k
    tmpl('sjframe', 'P:SJ.restore%d' % k, {"sj": [dict(state=S('SJ.restore%d' % k), emit=S('SJ.restoreemit%d' % k), next=S('SJ.restore%d' % (k + 1)), threshold=8 * (k + 1))]}, True)
holder('SJ.restore5')
tmpl('restore-tail', '=h_SJ.restore5', {"restore": [dict(entry=S('SJ.restore5'), **fr('SJ.restore5', [('first_ret', 'r'), ('second_ret', 'r'), ('third_ret', 'r')]))]}, True)
holder('CL.lj')
lj = dict(entry=S('CL.lj'), **fr('CL.lj', [('open_ok', 'e'), ('open_test', 'b'), ('next1_ret', 'r'), ('expr1_ret', 'r'), ('comma_ok', 'e'), ('comma_test', 'b'), ('next2_ret', 'r'), ('expr2_ret', 'r'), ('close_ok', 'e'), ('close_test', 'b'), ('next3_ret', 'r'), ('postix_ret', 'r')]))
lj.update(open_token="tok.(", comma_token="tok.,", close_token="tok.)")
tmpl('longjmp', '=h_CL.lj', {"longjmp": [lj]}, True)
sec('part12')
for k, (base, suffix) in enumerate((('DBL', 'd'), ('FLT', 's'))):
    sticky['text12'] = 'sqrt' + suffix
    sec('sqrt', "sqrt_entry=@str:CL.sqrt%d,sqrt_body=@str:SQ%d,sqrt_next=@str:%s,sqrt_convert=@str:TO.%s,sqrt_base=consts.%s,sqrt_register=@str:sqrt%d" % (k, k, 'CL.sqrt1' if k == 0 else 'CL.def', suffix, base, k))
ln("let", "-", "-", "-", "-", "-", "-", "-", '{"accumulate":"cb","keep":"cb"}')
open(R + 'callcontrol-begin-manifest.tsv', 'w').write("\n".join(out) + "\n")
# ---- finish
sticky.clear()
out = [H, "# call control, phase finish (was callcontrol.py): env cb = the begin phase's accumulated bindings"]
sec('part15', first=["cb", "consts"])
ln("rows", "call-conversion", "main", "-", F, "-", "-", "-", '{"accumulate":"cb","classes":"convclasses"}')
sec('part17')
for k, (_, op, width) in enumerate(SYS, 1):
    sticky['text35'] = 'sys%d' % k
    sec('sysemit', "sys_entry=@str:%s,sys_hit=@str:CL.y%d,sys_arity=@str:%s,host_arity_test=@str:CL.art%d,sys_test=@str:CL.z%d,sys_zero=@str:CL.zz%d,sys_emit=@str:CL.x%d,sys_next=@str:%s,sys_width=n.%d,sys_index=n.%d" % (
        'CL.sysz' if k == 1 else 'CL.w%d' % k, k, ('CL.ar%d' % k) if op.startswith('host') else 'CL.y%d' % k, k, k, k, k,
        'CL.w%d' % (k + 1) if k < len(SYS) else 'DEAD', width, k))
    if op.startswith('host'):
        sec('hostarity')
sec('part19')
open(R + 'callcontrol-finish-manifest.tsv', 'w').write("\n".join(out) + "\n")
g = open(R + 'gen2.py').read()
a = g.index('    from callcontrol import install as call_control\n')
b = g.index('    call_control(E, P, warnings, TEMPL, addr, call_facts, SYSCALLS, fpu, "finish", call_bindings)\n')
mid = g[a:b]
conv = mid[mid.index('    from truth import conversions'):]
fpu_line = '    fpu = {row[1]: row[2] for row in E.gold("irsel") if row[0] == "fpu"}\n'
g = g[:a] + '''    import assemble
    call_env = assemble.run(Path(__file__).resolve().parent / 'callcontrol-begin-manifest.tsv', E, P,
                            dict(warnings=warnings), dict(warnings=warnings))
''' + fpu_line + conv + '''    assemble.run(Path(__file__).resolve().parent / 'callcontrol-finish-manifest.tsv', E, P,
                 dict(warnings=warnings), dict(warnings=warnings, cb=call_env['cb']))
''' + g[b + len('    call_control(E, P, warnings, TEMPL, addr, call_facts, SYSCALLS, fpu, "finish", call_bindings)\n'):]
open(R + 'gen2.py', 'w').write(g)
