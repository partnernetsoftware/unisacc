F='U:{entry}'
def f(k):return 'fresh:%s:%s'%(F,k)
rows=[
 "# op\tstem\tsection\twhen\tfacts\tfresh\tseq\tbind\topts",
 "# variable address selection (was gen2.addr): env in entry (state), pending (acts); out done",
 "\t".join(["let","-","-","-","-","-","-",",".join("%s=%s"%(n,f(k)) for n,k in zip(('global','local','done','static','frame','auto','entry_test'),('ga','la','ad','sa','fa','auto','b'))),"-"]),
 "\t".join(["template","librarydata","address","-","librarydata",F,
   "addr=@out:  .libraryaddr r0\\x2c g_,comma=@out:\\x2c ,nl=@out:\\n",
   "EXTERN=librarydata!.EXTERN,USED=librarydata!.USED,DEPTH=librarydata!.DEPTH,BASE=librarydata!.BASE",
   '{"let":{"entry":["$global"],"classic":["@str:{global}.classic"],"done":["$done"]}}']),
 "\t".join(["let","-","-","-","-","-","-",",".join("%s=%s"%(n,f(k)) for n,k in (('local_test','b'),('static_return','r'),('frame_test','b'),('auto_end','r'))),"-"]),
 "\t".join(["rows","helpers","address-auto","-","-","-","auto_prefix=@out:  imm r2\\x2c ","auto=$auto,auto_end=$auto_end","-"]),
 "\t".join(["rows","helpers","address","-","parse-constants","-",
   "pending=$pending,auto_tail=@out:  sub64 r0\\x2c r6\\x2c r2\\n,static_prefix=@out:  .lea r0\\x2c ls,newline=@out:\\n",
   "entry=$entry,entry_test=$entry_test,global=$global,local=$local,done=$done,static=$static,frame=$frame,auto=$auto,local_test=$local_test,static_return=$static_return,frame_test=$frame_test,auto_end=$auto_end,global_end=@str:{global}.classic,GMARK=parse-constants!.GMARK",
   '{"mapseq":{"global_tail":[{"acts":[["@out","  .lea r0, g_"],["SPAN2","ips","ipe"],["@out","\\n"]]}]}}']),
]
open('exec/parse2/addr-manifest.tsv','w').write("\n".join(rows)+"\n")
s=open('exec/parse2/helpers-result.tsv').read()
r='address-auto\t$auto\t*\tPRN\t[["@","auto_prefix"],["COPYW","n","s"],["PUSH",["constant","auto_end"]]]\n'
if r not in s: open('exec/parse2/helpers-result.tsv','w').write(s+r)
g=open('exec/parse2/gen2.py').read()
a=g.index('def addr(p):');b=g.index('def emit(p, name):')
new='''def addr(p):
    """Variable address selection: exec/parse2/addr-manifest.tsv (K2 sub-manifest)."""
    import assemble
    env = assemble.run(Path(__file__).resolve().parent / 'addr-manifest.tsv', E, P, {}, dict(entry=p.cur, pending=p.acts))
    p.cur, p.acts = env['done'], []
    return p


'''
open('exec/parse2/gen2.py','w').write(g[:a]+new+g[b:])
