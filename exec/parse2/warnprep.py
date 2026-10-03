#!/usr/bin/env python3
"""Warm the shared model cache (exec/c/compilerpack.built_model) for the parse2 warning/error checks.

errorcheck.py and returnwarningcheck.py build these models themselves on a miss, so a queue may run
them in any order; the prep jobs only move the cold construction (~30-45 s per parse2 model) out of
the checks so each check stays far inside the 60 s watchdog.  Usage: warnprep.py GROUP (0..3)."""
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
with tempfile.TemporaryDirectory(prefix='warnprep-') as td:
    for name,args in GROUPS[int(sys.argv[1])]:
        built_model(td,name,GEN,args);print('cached',name,*args,flush=True)
