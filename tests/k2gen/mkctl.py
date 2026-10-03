import json, re, sys
p='exec/assemble.py';s=open(p).read()
o='owner = out.get(owner, facts.get(owner, owner))'
if o in s: open(p,'w').write(s.replace(o,'owner = out.get(owner, owner)'))
sys.argv=['x']; sys.path[:0]=['exec/parse2','exec','exec/facts']
import gen2 as G
E=G.E
consts=dict(TYPERANK=G.TYPERANK, MEMBERRANK=G.MEMBERRANK, RETURNRANK=G.RETURNRANK, PARAMRANK=G.PARAMRANK, SHAPE=G.SHAPE, VLDEP=G.VLDEP, CSV=G.CSV, CSL=G.CSL, U32M=G.U32M, DIM=G.DIM, ARR=E.ARR, TDIM=G.TDIM, FPB=G.FPB, FPV=G.FPV,
                UNSIGNED_INT=G.UNS + 4, UNSIGNED_LONG=G.UNS + 8)
consts.update((n, getattr(G,n)) for n in ("STAG","TAGLEVEL","TAGUNDO","ETAG","SBB","SSZ","SAL","SMN","SMEM","SFLAT","MOF","MSZ","MPT","MBS","MAR","MFLAT","MEMBER_STRIDE","BFW","BFO","BFS","TDE"))
consts.update(STRUCT_LIMIT=G.STRUCT_MAX+1, MEMBER_MASK=-G.MEMBER_STRIDE, TAGUNDO1=G.TAGUNDO+1, TAGUNDO2=G.TAGUNDO+2, TAGUNDO3=G.TAGUNDO+3)
consts.update(TDN=E.TDN, TDB=E.TDB, TDD=E.TDD, UNS=G.UNS, UNSIGNED_CHAR=G.UNS+1, UNSIGNED_SHORT=G.UNS+2)
consts.update(STATICF=35<<40)
seqs={name: E.O(re.split(r"(\{[^}]*\})", G.TEMPL[t])[int(f)]) for name,t,f in G.tape_rows("control-text.tsv")}
seqs.update((name, E.rej(m)) for name,m in G.tape_rows("control-reject.tsv"))
p=G.P("control.k2facts")
for name, method, slots in G.tape_rows("control-stack.tsv"):
    p.acts=[]; seqs[name]=getattr(p,method)(*slots.split(",")).acts
E.WORDS.append("type=extern"); E.TK["type=extern"] = max(E.TK.values()) + 1
E.WORDS.append("type=_Bool"); E.TK["type=_Bool"] = max(E.TK.values()) + 1
E.tokenizer(("type=const", "type=volatile", "type=restrict", "type=inline"))
tokens=dict(G.TK, identifier=G.TK_ID, number=G.TK_NUM, string=E.TK_STR, floating=E.TK_FNUM)
classes={name: [G.AX.index("f32"), G.AX.index("f64")] if kind=="float_axes" else [G.TK[t] for t in G.TWORDS] if kind=="typewords" else [tokens[v]] for name,kind,v in G.tape_rows("control-classes.tsv")}
J=lambda v: json.dumps(v, separators=(',',':'))
open('exec/facts/k2-control.tsv','w').write("# structured control facts (was gen2.structured_control): graph constants, text/reject/stack sequences, token classes\n=consts\tjson\t%s\n=seqs\tjson\t%s\n=classes\tjson\t%s\n"%(J(consts),J(seqs),J(classes)))
m=["# op\tstem\tsection\twhen\tfacts\tfresh\tseq\tbind\topts",
   "# structured control (was gen2.structured_control): env control_section, statement, extra (dict), seqb (dict)",
   "\t".join(["rows","control","@str:{control_section}","-","k2-control","-","-","statement=$statement",
     J({"bindmap":["consts","extra"],"freshrows":[{"file":"control-fresh.tsv","where":{"section":"{control_section}"},"key":"{key}","owner":"{prefix}","kind":"{kind}","holder":True,"lookup":True}],"seqlist":["seqs","seqb"],"classes":"classes"})])]
open('exec/parse2/control-manifest.tsv','w').write("\n".join(m)+"\n")
g=open('exec/parse2/gen2.py').read()
a=g.index('def structured_control(section, warnings, extra=None, sequence_bindings=None):')
b=g.index('def ordinary_control(section, warnings, extra=None):')
g=g[:a]+'''def structured_control(section, warnings, extra=None, sequence_bindings=None):
    """Structured control: exec/parse2/control-manifest.tsv (K2 sub-manifest)."""
    import assemble
    section += "-warnings" if warnings and section in ("block", "if") else ""
    assemble.run(Path(__file__).resolve().parent / 'control-manifest.tsv', E, P, {},
                 dict(control_section=section, statement="STMT.body" if warnings else "STMT",
                      extra=dict(extra or {}), seqb=dict(sequence_bindings or {})))


'''+g[b:]
open('exec/parse2/gen2.py','w').write(g)
