#!/usr/bin/env python3
"""Model diagnostic bytes vs the actual reference diag_at/warn_at routines.
Scratch-only reference instrumentation and a model test continuation. The
continuation calls the production renderer, not a Python reimplementation.
"""
import importlib.util,json,os,pathlib,struct,subprocess,sys,tempfile
R=pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0,str(R/'exec/c'));from pack import build as package
from refsource import source_text,compile_command

def run(args,**kw):return subprocess.run(list(map(str,args)),capture_output=True,timeout=60,**kw)
def call(args,**kw):
    p=run(args,**kw);assert p.returncode==0,(p.args,p.returncode,p.stderr[-1200:]);return p.stdout


def probe_model():
    spec=importlib.util.spec_from_file_location('diag_grammar',R/'exec/build/parsebase.py')
    E=importlib.util.module_from_spec(spec);spec.loader.exec_module(E)
    import assemble
    _tl=assemble.load_facts('tokenlocations')['tokenlocations!']  # exec/facts/tokenlocations.tsv
    locations=lambda E,P: assemble.run(R/'exec/parse2/tokenlocations-manifest.tsv',E,P,dict(multi=True,record=False,ordinal=False),
        dict(ready='START',token_record='RET',ordinal_table=0))['start']
    diagnostics=lambda E,P: assemble.run(R/'exec/parse2/diagnostics-manifest.tsv',E,P,{},
        {k:_tl[k] for k in ('SPLICES','INCLUDE_LINE','INCLUDE_LINES','INCLUDE_NAME')})
    P,g=E.P,E.g
    P('NEXT').goto('DEAD');P('NX').goto('DEAD')
    start=locations(E,P);diagnostics(E,P)
    P('START').a(('SBCLR',),*[('SBOUT',c) for c in b'probe [-Wtest]'],('SBSAVE','diag_message'),('LDI','probe_count',0)).goto('PROBE.mode')
    for c in (0,1):g.on('PROBE.mode',[c],'PROBE.0',[('LDI','diag_warning',c),('LDI','diag_pos',0),('ADV',)])
    g.on('PROBE.mode',[256],'PROBE.done',[])
    g.els('PROBE.mode','DEAD',E.rej('bad diagnostic test request'))
    for k in range(8):
        g.on('PROBE.'+str(k),range(256),'PROBE.'+str(k+1) if k<7 else 'PROBE.call',
             [('BYTE','probe_byte'),('A64I','shl','probe_byte','probe_byte',8*k),
              ('A64','or','diag_pos','diag_pos','probe_byte'),('ADV',)])
        g.els('PROBE.'+str(k),'DEAD',E.rej('bad diagnostic test request'))
    P('PROBE.call').call('DIAG.report').a(('ALU','add','probe_count','probe_count','diag_reported')).goto('PROBE.mode')
    p=P('PROBE.done')
    for k in range(4):p.a(('ALUI','sar','probe_byte','probe_count',8*k),('OUTW','probe_byte'))
    p.a(('ACCEPT',)).goto('DEAD')
    g.finish()
    return {'start':start,'states':{n:[m,{str(k):v for k,v in row.items()}] for n,(m,row) in g.st.items()},'seqs':g.seqs}


with tempfile.TemporaryDirectory(prefix='model-diagnostics-') as td:
    t=pathlib.Path(td)
    source=source_text(R)
    helper='''static void probe_diag(void) { char *s; long v; int neg; int mode; int count; unsigned char out[4]; int i;
s=getenv("UA_DIAG_POS"); mode=getenv("UA_DIAG_WARN")[0]==49; count=0; warnall=1; nwarn=0;
while(*s) { neg=0; v=0; if(*s==45){neg=1;s=s+1;} while(*s>=48 && *s<=57){v=v*10+*s-48;s=s+1;}
if(neg)v=0-v; if(mode)warn_at(v,"probe [-Wtest]");else err_at(v,"probe [-Wtest]");count=count+1;if(*s==44)s=s+1; }
if(mode)count=nwarn; for(i=0;i<4;i++)out[i]=(count>>(8*i))&255;__write(1,out,4); }
'''
    anchor='int fe_load(char *path, char *t) {';assert source.count(anchor)==1
    source=source.replace(anchor,helper+anchor)
    anchor='    if (pponly) {                       /* -E: the text, not a program */';assert source.count(anchor)==1
    source=source.replace(anchor,'    if (getenv("UA_DIAG_POS")) { probe_diag(); return 2; }\n'+anchor)
    (t/'ref.c').write_text(source);call(compile_command(R,t/'ref.c',t/'ref'))
    call([os.environ.get('EXEC_CC','cc'),'-O2',R/'exec/c/run.c','-o',t/'run'])
    call([sys.executable,R/'exec/build/gen.py','pp',t/'pp.json','--locations'])
    (t/'diag.json').write_text(json.dumps(probe_model(),separators=(',',':')))
    for name in ('pp','diag'):
        call([sys.executable,R/'exec/c/tbl.py',t/(name+'.json'),t/(name+'.tbl')])
        call([sys.executable,R/'exec/c/net.py',t/(name+'.tbl'),t/(name+'.net')])
    (t/'inner.h').write_text('int probe_here;\n')
    (t/'outer.h').write_text('#include "inner.h"\nint probe_here_outer;\n')
    cases=[('plain','int\tprobe_here;\n',[]),('macro','#define V 12345\nint probe_here=V;\n',[]),
           ('splice','int probe_here=1+\\\n2;\n',[]),('crlf','int probe_here=1+\\\r\n2;\n',[]),
           ('comment','/* a\nb */ int probe_here;\n',[]),
           ('nested','#include "outer.h"\nint probe_here;\n',[]),
           ('repeated','#include "inner.h"\n#include "inner.h"\nint probe_here;\n',[]),
           ('forced','int probe_here;\n',['-include',str(t/'outer.h')]),
           ('automatic','int main(void){printf("probe_here\\n");return 0;}\n',[]),
           ('no-newline','int probe_here;',[]),('empty','',[])]
    for name,text,flags in cases:
        f=t/'source.c';f.write_text(text)
        resources=t/'cli';resources.mkdir(exist_ok=True)
        (resources/'includes').write_bytes((flags[1]+'\0').encode() if flags else b'')
        route=t/'route.tsv';route.write_text('map\te2\tsrc.c\tpp.locations\tpp.net\n')
        pkg=t/'pkg';pkg.write_bytes(package([route],[('00636c692f',resources),('006864722f',R/'include')]))
        pp=call([t/'run','--bundle',pkg,'map',f,f,R/'include'])
        n=struct.unpack_from('<I',pp,7)[0];body=pp[-n:] if n else b''
        offsets=[-7,0,1,max(n-1,0),n,n+7]
        offsets += [i for i in range(len(body)) if body.startswith(b'probe_here',i)]
        for mode in (0,1):
            data=b'UNITOK1\0'+struct.pack('<I',len(pp))+pp+b''.join(bytes([mode])+struct.pack('<q',p) for p in offsets)
            inp=t/'request';inp.write_bytes(data)
            ref=run([t/'ref',f,*flags,'-E'],env=dict(os.environ,UA_DIAG_POS=','.join(map(str,offsets)),UA_DIAG_WARN=str(mode)))
            got=run([t/'run',t/'diag.net',inp,f])
            assert ref.returncode==got.returncode==0,(name,mode,ref.returncode,got.returncode,got.stderr[-1000:])
            assert (ref.stdout,ref.stderr)==(got.stdout,got.stderr),(name,mode,ref.stdout,got.stdout,ref.stderr[:1500],got.stderr[:1500])
        print('diagnostic rendering',name,'errors/warnings match reference',flush=True)
    print('diagnostic rendering: 11 cases x 2 modes, including clamp and header suppression')
