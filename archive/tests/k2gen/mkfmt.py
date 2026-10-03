import json, sys
sys.argv = ['x']
sys.path[:0] = ['exec/parse2', 'exec', 'exec/facts']
import gen2
conv = gen2.PFCONV
classes = {n: json.loads(v) for n, v in gen2.tape_rows('helpers-classes.tsv')}
lenkeys = sorted(set(range(257)) - conv.keys())
f = ["# written by hand from exec/parse2/gen2.py fmtwalk (printf format scanner facts)",
     "=classes\tjson\t" + json.dumps(classes, separators=(',', ':')),
     "=lenkeys\tjson\t" + json.dumps(lenkeys, separators=(',', ':')),
     "@conv\tbyte:int\tkind:int"]
f += ["\t%d\t%d" % (b, k) for b, k in conv.items()]
open('exec/facts/k2-fmtwalk.tsv', 'w').write("\n".join(f) + "\n")
B = "walk=@str:{pre}.w,percent=@str:{pre}.pc,width=@str:{pre}.width,precision=@str:{pre}.precision,length=@str:{pre}.length,on_byte=$on_byte,on_d=$on_d,on_end=$on_end"
S = "reject=@rej:not covered: printf conversion"
m = ["# op\tstem\tsection\twhen\tfacts\tfresh\tseq\tbind\topts",
     "# decoded printf format scanning (was gen2.fmtwalk): env pre, on_byte, on_d, on_end",
     "\t".join(["rows", "helpers", "format", "-", "k2-fmtwalk", "-", S, B, '{"classes":"classes"}']),
     "\t".join(["rows", "helpers", "length", "-", "k2-fmtwalk", "-", S, B, '{"classes":"classes","domain_keys":"lenkeys"}']),
     "\t".join(["foreach", "-", "-", "-", "k2-fmtwalk", "-", "-", "-", '{"over":"conv","as":"c"}']),
     "\t".join([".rows", "helpers", "conversion", "-", "k2-fmtwalk", "-", S, B + ",kind=c.kind", '{"classes":"classes","let":{"keys":["c.byte"]},"domain_keys":"keys"}']),
     ]
open('exec/parse2/fmtwalk-manifest.tsv', 'w').write("\n".join(m) + "\n")
