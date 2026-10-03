import json, sys
sys.argv = ['x']; sys.path[:0] = ['exec/parse2', 'exec', 'exec/facts']
import gen2 as G
from load import facts
E = G.E
J = lambda v: json.dumps(v, separators=(',', ':'))
rej = {r['name']: E.rej(r['value']) for r in facts('strings') if r['kind'] == 'reject'}
reserved = {int(b) for line in open('exec/parse2/strings-escape-policy.tsv').read().splitlines()[1:] for b in line.split('\t')[1].split(',')}
escape = [{'byte': ord(ch), 'value': v} for ch, v in G.ESC.items() if ord(ch) not in reserved]
open('exec/facts/k2-strings.tsv', 'w').write(
    "# string walk facts (was strings.walk/rules): reject sequences, escape map, escape domain\n"
    "=rej\tjson\t%s\n=escape\tjson\t%s\n=escbytes\tjson\t%s\n=TK_STR\tint\t%d\n" % (J(rej), J(escape), J([e['byte'] for e in escape]), E.TK_STR))
walk = ",".join("%s=@str:{pre}%s" % (r['name'], r['value']) for r in facts('strings') if r['kind'] == 'walk')
walk += ",body=$body,done=$done,TK_STR=TK_STR"
tail = ",".join("%s=fresh:U:{pre}:%s" % (n, k) for s, n, p, k in
                (l.split('\t') for l in open('exec/parse2/strings-names.tsv').read().splitlines() if not l.startswith('#'))
                if s == 'walk_tail')
H = "# op\tstem\tsection\twhen\tfacts\tfresh\tseq\tbind\topts"
m = [H, "# narrow/wide code-point walk (was strings.walk): env pre, body, done",
     "\t".join(["rows", "strings", "walk_head", "-", "k2-strings", "-", "-", walk, '{"seqlist":["rej"]}']),
     "\t".join(["template", "strings", "walk_escape", "-", "k2-strings", "-", "-", walk, '{"mode":"b","domain_keys":"escbytes"}']),
     "\t".join(["rows", "strings", "walk_tail", "-", "k2-strings", "-", "-", walk + "," + tail, '{"seqlist":["rej"]}'])]
open('exec/parse2/strwalk-manifest.tsv', 'w').write("\n".join(m) + "\n")
def call(stem, bind, merge=False):
    return "\t".join(["call", stem, "-", "-", "-", "-", "-", bind, '{"merge":true}' if merge else "-"])
pc = lambda i: call("printfcontrol-%d" % i, "alphabet=@str:0123456789abcdef,section=@str:part%d" % i, True)
p = [H, "# printf family (was gen2.printf): flags warnings; printfcontrol segments chain their env (merge)",
     call("printfallback", "-"), pc(0),
     call("strwalk", "pre=@str:FMT.walk,body=@str:FMT.byte,done=@str:FMT.end"), pc(1),
     call("fmtwalk", "pre=@str:PF,on_byte=@str:PF.b,on_d=@str:PF.d,on_end=@str:PF.end"), pc(2),
     call("strwalk", "pre=@str:PL,body=@str:PL.cp,done=@str:PL.end"),
     "\t".join(["rows", "strings", "wide_hooks", "-", "k2-strings", "-", "-", "TK_STR=TK_STR", '{"seqlist":["rej"]}']), pc(3),
     call("fmtwalk", "pre=@str:PO,on_byte=@str:PO.b,on_d=@str:PO.d,on_end=@str:PO.end"), pc(4)]
open('exec/parse2/printf-manifest.tsv', 'w').write("\n".join(p) + "\n")
g = open('exec/parse2/gen2.py').read()
a = g.index('def printf(warnings=False):'); b = g.index('\nUNS = E.UNS')
g = g[:a] + '''def printf(warnings=False):
    """printf family: exec/parse2/printf-manifest.tsv (K2 sub-manifest)."""
    import assemble
    assemble.run(Path(__file__).resolve().parent / 'printf-manifest.tsv', E, P, dict(warnings=warnings), dict(warnings=warnings))

''' + g[b:]
a = g.index('def strwalk(pre, body, done):'); b = g.index('def ladder(prefix, bottom):')
g = g[:a] + '''def strwalk(pre, body, done):
    import assemble
    assemble.run(Path(__file__).resolve().parent / 'strwalk-manifest.tsv', E, P, {}, dict(pre=pre, body=body, done=done))


''' + g[b:]
open('exec/parse2/gen2.py', 'w').write(g)
s = open('exec/assemble.py').read()
o = '''            res = sub.run(self.root / (stem + "-manifest.tsv"))'''
assert s.count(o) == 1
if 'opts merge' not in s:
    s = s.replace(o, o + '''
            if o.get("merge"):   # opts merge: the sub-manifest's env flows back (segment chains)
                self.env.update(res)''')
open('exec/assemble.py', 'w').write(s)
