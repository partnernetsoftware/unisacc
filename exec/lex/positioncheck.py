#!/usr/bin/env python3
"""Compare optional E1 offsets with the actual reference lexer's tpos array.
The reference and input-buffer hook are built only in scratch. Every child
has a 60 second bound; ordinary typed output is checked independently too.
"""
import json, os, pathlib, struct, subprocess, sys, tempfile
R = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(R/'exec/pp'))
import sim


def call(args):
    p = subprocess.run(list(map(str,args)), capture_output=True, timeout=60)
    assert p.returncode == 0, (p.args, p.returncode, p.stderr[-1000:])
    return p.stdout


with tempfile.TemporaryDirectory(prefix='lex-positions-') as td:
    t = pathlib.Path(td)
    pieces = [R/'tests/refshim.h', R/'src/version.h', R/'kernel/unisa_model.inc', R/'kernel/unisa_headers.inc']
    source = ''.join(p.read_text() for p in pieces)
    source += ''.join(l for l in (R/'kernel/unisa_core.c').read_text().splitlines(True) if not l.startswith('#include "unisa_'))
    source += ''.join((R/'src'/n).read_text() for n in ['front_pp.c','front_parse.c','opt.c','main.c','back_lower.c','host_dl.h','back_encode.c','tapeprune.c','back_image.c'])
    assert source.count('#include "host_dl.h"\n') == 1
    source = source.replace('#include "host_dl.h"\n', '')
    source += (R/'tests/reffoot.h').read_text()
    helper = '''static void probe_word(long n) { unsigned char b[4]; int j; for(j=0;j<4;j++) b[j]=(n>>(8*j))&255; __write(1,b,4); }
'''
    anchor = 'int main(void) {'
    assert source.count(anchor) == 1
    source = source.replace(anchor, helper+anchor)
    anchor = '    if (lex() < 0) return 1;\n    i = 0;\n    while (i < ntok) {'
    assert source.count(anchor) == 1
    source = source.replace(anchor, '    probe_word(nsrc); __write(1,src,nsrc);\n'+anchor)
    anchor = '        p = voff(TOKV, tkind[i]);'
    assert source.count(anchor) == 1
    source = source.replace(anchor, '        __write(1,"@",1); probe_word(tpos[i]); __write(1,"\\n",1);\n'+anchor)
    anchor = '        if (tkind[i] == 2) {'
    assert source.count(anchor) == 1
    source = source.replace(anchor, '        if (tkind[i] == 1) { __write(1,"=",1); __write(1,src+tpos[i],tlen[i]); }\n'+anchor)
    (t/'ref.c').write_text(source)
    call(['cc','-w','-O1','-I',R/'kernel',t/'ref.c','-o',t/'ref'])
    call([os.environ.get('EXEC_CC','cc'),'-O2',R/'exec/c/run.c','-o',t/'run'])
    for flag,name in [('--positions','positions'),('--typed','typed')]:
        call([sys.executable,R/'exec/lex/gen.py',t/(name+'.json'),flag])
        call([sys.executable,R/'exec/c/tbl.py',t/(name+'.json'),t/(name+'.tbl')])
        call([sys.executable,R/'exec/c/net.py',t/(name+'.tbl'),t/(name+'.net')])
    delta=json.loads((t/'positions.json').read_text())
    # Generic simulator needs the explicit start and rowless halt target,
    # which tbl.py already supplies when loading the legacy E1 format.
    delta['start']='DISPATCH'; delta['states']['HALT']=['b',{}]
    # INC is E1's spelling of 32-bit add-one; pp.sim has ALUI instead.
    delta['seqs']=[[["ALUI","add",a[1],a[1],1] if a[0]=='INC' else a
                    for a in seq] for seq in delta['seqs']]
    loaded=sim.load(delta)
    cases = [('', 'empty'), ('int x=123;\n','plain'),
             ('#define VALUE 12345\nint x=VALUE;\n','macro'),
             ('int x=1+\\\n2;\n','splice'),
             ('/* a\nb */ unsigned long x=0xabcdefUL;\n','comment'),
             ('char *s="a" L"b"; int x=L\'z\';\n','prefixes'),
             ('double x=.12e+3; int y=a>>=2;\n','lookahead'),
             ('__attribute__((unused)) int x; __extension__ int y;\n','attributes'),
             ('\n'*270+'int end;\n','wide-offset')]
    for text,name in cases:
        src=t/(name+'.c'); src.write_text(text)
        raw=call([t/'ref','-dump-tokens',src]); n=struct.unpack_from('<I',raw)[0]
        buf,want=raw[4:4+n],raw[4+n:]; assert len(buf)==n
        inp=t/'input';inp.write_bytes(buf)
        got=call([t/'run',t/'positions.net',inp]);assert got==want,(name,got,want)
        result,val,_=sim.run(delta,buf,str(src),loaded=loaded,maxsteps=2000000)
        assert result=='accept' and val==want,(name,result,val,want)
        # Strip only the fixed six-byte prefix before each token (not a
        # broad line filter: an offset itself can contain newline or '@').
        i=0; plain=bytearray(); offsets=[]
        while i<len(got) and got[i:i+1]==b'@':
            assert got[i+5:i+6]==b'\n'
            offsets.append(struct.unpack_from('<I',got,i+1)[0]);i+=6
            end=got.index(b'\n',i)+1;plain.extend(got[i:end]);i=end
        plain.extend(got[i:]);assert offsets and offsets[-1]==len(buf)
        assert bytes(plain)==call([t/'run',t/'typed.net',inp]),name
        print('token positions',name,'match reference',flush=True)
    print('token positions: 9 reference and Python oracle cases pass')
