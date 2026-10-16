#!/usr/bin/env python3
"""Warm the shared model cache (exec/c/compilerpack.built_model) for the parse2 warning/error checks.

Gate consumers require these models and verified run/ref tools; misses refuse rc75 without construction.
Explicit preparation moves cold construction out of the checks.  Usage: warnprep.py GROUP (0..3)."""
import pathlib,sys,tempfile
R=pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0,str(R/'exec/c'))
from compilerpack import built_model
GEN=R/'exec/build/gen.py'
# Same (script, args) as the checks, so the content keys match.
GROUPS=[[('parse',['parse2','--warnings'])],
        [('warnparse',['parse2','--warnings','--errors'])],
        [('errorparse',['parse2','--errors'])],
        [('plain',['parse2','--locations']),
         ('pp',['pp','--locations']),('lex',['lex','--locations'])]]
def cached_inputs(out, specs, tools, label):
    """Consumer preflight: all models then already-built tools; never constructs."""
    import compilerpack
    compilerpack.REQUIRE_CACHED=True
    try:
        for name,args in specs: compilerpack.built_model(out,name,GEN,args)
    except ValueError as error:
        print('NEED_PREPARE '+label+': '+str(error),file=sys.stderr);raise SystemExit(75)
    tools=tool_directory() if tools is None else pathlib.Path(tools)
    from provenance import verify
    try:
        for name in ('run','ref'): verify(tools/name)
    except SystemExit as error:
        print('NEED_PREPARE '+label+': '+str(error),file=sys.stderr);raise SystemExit(75)
    return tools/'run',tools/'ref'

def tool_directory():
    import os
    from provenance import source_digest
    base=pathlib.Path(os.environ.get('UNISACC_MODEL_CACHE',tempfile.gettempdir()+'/unisacc-model-cache'))
    return base/('tools-warn-'+source_digest())

def prepare_tools(out):
    """Explicit preparation; shared source identity and per-artifact provenance checks."""
    import subprocess,os,fcntl,shutil
    from provenance import source_digest,write,verify
    out=tool_directory() if out is None else pathlib.Path(out)
    out.parent.mkdir(parents=True,exist_ok=True)
    with pathlib.Path(str(out)+'.lock').open('a') as lock:
        fcntl.flock(lock,fcntl.LOCK_EX)
        try:
            for name in ('run','ref'): verify(out/name)
        except SystemExit: pass
        else:
            print('warning tools: hit',out,flush=True);return
        before=source_digest()
        with tempfile.TemporaryDirectory(prefix='tools-warn-build-',dir=out.parent) as td:
            work=pathlib.Path(td)
            subprocess.run([sys.executable,str(R/'tests/bound.py'),'55',os.environ.get('EXEC_CC','cc'),'-O2',str(R/'exec/c/run.c'),'-o',str(work/'run')],check=True)
            write(work/'run',before)
            subprocess.run([sys.executable,str(R/'tests/bound.py'),'55','sh',str(R/'tests/build_ref.sh'),str(work/'ref.c'),str(work/'ref')],check=True)
            write(work/'ref',before)
            try:out.stat()
            except FileNotFoundError:pass
            else:shutil.rmtree(out)
            work.rename(out)
        print('warning tools: prepared',out,flush=True)

if __name__=='__main__':
    import argparse
    ap=argparse.ArgumentParser();ap.add_argument('group',nargs='?',type=int,choices=range(4));ap.add_argument('--tools',nargs='?',const=True,type=pathlib.Path);a=ap.parse_args()
    if (a.group is None)==(a.tools is None):ap.error('choose a group or --tools OUT')
    if a.tools is not None:prepare_tools(None if a.tools is True else a.tools)
    else:
        with tempfile.TemporaryDirectory(prefix='warnprep-') as td:
            for name,args in GROUPS[a.group]:
                built_model(td,name,GEN,args);print('cached',name,*args,flush=True)
