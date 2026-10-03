import json, re, sys
sys.argv = ['x']; sys.path[:0] = ['exec/parse2', 'exec', 'exec/facts']
import gen2 as G
E = G.E
J = lambda v: json.dumps(v, separators=(',', ':'))
R = 'exec/parse2/'
rows = lambda s: [l.split('\t') for l in open(R + 'unarycontrol-' + s + '.tsv').read().splitlines()[1:]]
seqs = {n: E.O(json.loads(v)) for n, v in rows('text')}
for n, v in rows('template'):
    t, i = json.loads(v); seqs[n] = E.O(re.split(r'(\{[^}]*\})', G.TEMPL[t])[i])
p = G.P('unarycontrol.k2facts')
for n, v in rows('stack'):
    m, sl = json.loads(v); p.acts = []; seqs[n] = getattr(p, m)(*sl).acts
E.WORDS.append("type=extern"); E.TK["type=extern"] = max(E.TK.values()) + 1
E.WORDS.append("type=_Bool"); E.TK["type=_Bool"] = max(E.TK.values()) + 1
E.tokenizer(("type=const", "type=volatile", "type=restrict", "type=inline"))
tokens = dict(E.TK, identifier=E.TK_ID)
classes = {n: [tokens[t]] for n, t in rows('tokens')}
classes['typewords'] = [tokens[t] for t in (*G.TWORDS, 'struct', 'union', 'enum')]
open('exec/facts/k2-unary.tsv', 'w').write(
    "# unary-expression control facts (was unarycontrol.py): text/template/stack sequences, token classes\n"
    "=seqs\tjson\t%s\n=classes\tjson\t%s\n=empty\tjson\t{}\n=nil\tjson\t[]\n" % (J(seqs), J(classes)))
sections = {l.split('\t')[0] for l in open(R + 'unarycontrol-result.tsv').read().splitlines()[1:]}
H = "# op\tstem\tsection\twhen\tfacts\tfresh\tseq\tbind\topts"
out = [H, "# unary expressions (was unarycontrol.py): env ufacts (graph constants), ucx (compound extra); callbacks are call rows"]
first = [True]
def section(name, cells="-", export=None):
    o = {"accumulate": "ub", "freshrows": [{"file": "unarycontrol-fresh.tsv", "where": {"section": name},
                                            "key": "{key}", "owner": "{owner}", "kind": "{kind}", "holder": True}]}
    if first[0]:
        o["bindmap"] = "ufacts"; first[0] = False
    if export:
        o["export"] = export
    if name + '.all' in sections:
        o.update(seqlist=["seqs"], classes="classes")
        out.append("\t".join(["rows", "unarycontrol", name + ".all", "-", "k2-unary", "-", "-", cells, J(o)]))
    else:
        out.append("\t".join(["let", "-", "-", "-", "k2-unary", "-", "-", cells, J(o)]))
section('part0')
out.append("\t".join(["call", "control", "-", "-", "k2-unary", "-", "-",
                      "control_section=@str:compound,statement=@str:STMT,extra=$ucx,seqb=empty", "-"]))
out.append("\t".join(["rows", "width", "cast-void", "-", "-", "-", "-", "-", "-"]))
section('part4')
for suffix, rule in rows('conversions'):
    section(rule, "convert_entry=@str:UC.to_%s,convert_target=@str:TO.%s" % (suffix, suffix))
section('part6')
out.append("\t".join(["call", "printf", "-", "-", "-", "-", "-", "warnings=$warnings", "-"]))
section('part8', export=['f_part8_1216_U_r_1'])
out.append("\t".join(["call", "addr", "-", "-", "k2-unary", "-", "-", "entry=$f_part8_1216_U_r_1,pending=nil", '{"result":"addrenv"}']))
out.append("\t".join(["let", "-", "-", "-", "-", "-", "-", "f_part9_1217_U_ad_1=$$addrenv:done", '{"accumulate":"ub"}']))
section('part10')
open(R + 'unarycontrol-manifest.tsv', 'w').write("\n".join(out) + "\n")
g = open(R + 'gen2.py').read()
a = g.index('    from unarycontrol import install as unary_control\n')
b = g.index('    shape_control("value-load")')
g = g[:a] + '''    import assemble
    unit_span = int(re.search(r"^#define MAXTOK ([0-9]+)\\b", Path(E.ROOT, "src/front_pp.c").read_text(), re.M).group(1))
    assemble.run(Path(__file__).resolve().parent / 'unarycontrol-manifest.tsv', E, P, dict(warnings=warnings),
                 dict(warnings=warnings, ucx=dict(TIX=TIX, MAXTOK=unit_span),
                      ufacts=dict(DBL=DBL, FLT=FLT, BOOL=BOOL, UNS1=UNS+1, UNS3=UNS+3, UNS4=UNS+4, UNS8=UNS+8,
                                  U32M=U32M, ENV=ENV, END_=END_, FNSTR=FNSTR, TIX=TIX, MAXTOK=unit_span)))
''' + g[b:]
open(R + 'gen2.py', 'w').write(g)
