#!/usr/bin/env python3
"""Check diagnostic envelopes against a scratch-only instrumented reference.
Every subprocess is bounded. No product source or generated kernel is edited.
"""
import importlib.util,json,os,pathlib,struct,subprocess,sys,tempfile
R=pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0,str(R/'exec/c'));from pack import build as package
sys.path.insert(0,str(R/'exec/pp'));import sim

def call(args,**kw):
    p=subprocess.run(list(map(str,args)),capture_output=True,timeout=60,**kw)
    assert p.returncode==0,(p.args,p.returncode,p.stderr[-1000:])
    return p.stdout

def decode(b):
    assert b.startswith(b'UNIPP1\0');i=7
    def word():
        nonlocal i
        assert i+4<=len(b);v=struct.unpack_from('<I',b,i)[0];i+=4;return v
    length,forced,auto,nsp,ninc=[word() for _ in range(5)]
    sp=[word() for _ in range(nsp)];inc=[]
    for _ in range(ninc):
        line,n,nm=word(),word(),word();assert i+nm<=len(b)
        inc.append((line,n,b[i:i+nm]));i+=nm
    assert i+length==len(b)
    return b[i:],forced,auto,sp,inc

with tempfile.TemporaryDirectory(prefix='pp-locations-') as td:
    t=pathlib.Path(td)
    # Assemble the current reference in scratch, just as build_ref.sh does,
    # adding a dump at the actual preprocessed-buffer boundary.
    pieces=[R/'tests/refshim.h',R/'src/version.h',R/'kernel/unisa_model.inc',R/'kernel/unisa_headers.inc']
    source=''.join(p.read_text() for p in pieces)
    source+=''.join(l for l in (R/'kernel/unisa_core.c').read_text().splitlines(True) if not l.startswith('#include "unisa_'))
    source+=''.join((R/'src'/n).read_text() for n in ['front_pp.c','front_parse.c','opt.c','main.c','back_lower.c','host_dl.h','back_encode.c','back_image.c'])
    assert source.count('#include "host_dl.h"\n') == 1
    source = source.replace('#include "host_dl.h"\n', '')
    source+=(R/'tests/reffoot.h').read_text()
    anchor='int fe_load(char *path, char *t) {';assert source.count(anchor)==1
    helper='''static void probe_word(long n) { unsigned char b[4]; int i; for(i=0;i<4;i++) b[i]=(n>>(8*i))&255; __write(1,b,4); }
static void probe_map(void) { int i,n; char *s; __write(1,"UNIPP1\\0",7);
probe_word(nsrc);probe_word(prelines);probe_word(nautoinc);probe_word(nspl);probe_word(nireg);
for(i=0;i<nspl;i++)probe_word(spl_at[i]);
for(i=0;i<nireg;i++){probe_word(ireg_ln[i]);probe_word(ireg_nl[i]);s=fnpool+ireg_nm[i];n=blen(s);probe_word(n);__write(1,s,n);}
__write(1,src,nsrc); }
'''
    source=source.replace(anchor,helper+anchor)
    anchor='    if (pponly) {                       /* -E: the text, not a program */'
    assert source.count(anchor)==1
    source=source.replace(anchor,'    if (getenv("UA_LOCATION_MAP")) { probe_map(); return 2; }\n'+anchor)
    (t/'ref.c').write_text(source)
    call(['cc','-w','-O1',t/'ref.c','-o',t/'ref'])
    call([os.environ.get('EXEC_CC','cc'),'-O2',R/'exec/c/run.c','-o',t/'run'])
    call([sys.executable,R/'exec/pp/gen.py',t/'pp.json','--locations'])
    call([sys.executable,R/'exec/c/tbl.py',t/'pp.json',t/'pp.tbl'])
    call([sys.executable,R/'exec/c/net.py',t/'pp.tbl',t/'pp.net'])
    delta=json.loads((t/'pp.json').read_text());loaded=sim.load(delta)
    (t/'outer.h').write_text('#include "inner.h"\n#define V 3\n')
    (t/'inner.h').write_text('int in_header;\n')
    cases=[('plain','int x;\n',[]),('macro','#define V 12345\nint x=V;\n',[]),
      ('splice','int x=1+\\\n2;\n',[]),('crlf','int x=1+\\\r\n2;\n',[]),
      ('comment','/* a\nb */ int x;\n',[]),('nested','#include "outer.h"\nint x=V;\n',[]),
      ('repeated','#include "inner.h"\n#include "inner.h"\nint x;\n',[]),
      ('forced','int x;\n',['-include',str(t/'outer.h')]),
      ('automatic','int main(void){printf("ok\\n");return 0;}\n',[])]
    for name,text,flags in cases:
        src=t/(name+'.c');src.write_bytes(text.encode())
        resources=t/'cli';resources.mkdir(exist_ok=True)
        (resources/'includes').write_bytes((flags[1]+'\0').encode() if flags else b'')
        route=t/'route.tsv';route.write_text('map\te2\tsrc.c\tpp.locations\tpp.net\n')
        pkg=t/'model.pkg';pkg.write_bytes(package([route],[('00636c692f',resources),('006864722f',R/'include')]))
        want=call([t/'ref',src,*flags,'-E'],env=dict(os.environ,UA_LOCATION_MAP='1'))
        got=call([t/'run','--bundle',pkg,'map',src,src,R/'include'])
        assert decode(got)==decode(want),(name,decode(got)[1:],decode(want)[1:])
        assert got==want,name
        plain=call([t/'ref',src,*flags,'-E'])
        assert decode(got)[0]==plain,name
        if name in ['plain','macro','splice','crlf','comment']:
            result,val,_=sim.run(delta,src.read_bytes(),str(src),sim.Files(str(R/'include')),loaded=loaded,maxsteps=2000000)
            assert result=='accept' and val==want,(name,result)
        print('location envelope',name,'matches reference',flush=True)
    print('location envelopes: 9 reference cases; 5 Python oracle cases pass')
