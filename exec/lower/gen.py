#!/usr/bin/env python3
"""Raw tape lowering on the generic executor.
Default: data pass only, with --arm64 selecting its target header.
--full: Linux target instructions and metadata (ARM fusions still pending).
"""
import importlib.util,json,pathlib,sys
ROOT=pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT))
spec=importlib.util.spec_from_file_location('lowerbase',ROOT/'exec/parse/gen.py')
E=importlib.util.module_from_spec(spec);spec.loader.exec_module(E)
from data import install
args=sys.argv[2:]
full='--full' in args
target='lnx/arm64' if '--arm64' in args else 'lnx/x86_64'
if len(sys.argv)<2 or len(args)!=len(set(args)) or any(a not in ('--full','--arm64') for a in args):
    sys.exit("usage: gen.py OUT.json [--full] [--arm64]")
install(E,code_start="C.prelude" if full else "H.code",target=target)
if full:
    from code import install as install_code
    install_code(E,arch=target.split("/")[1])
E.P('ACCEPTDATA').a(('ACCEPT',)).goto('DEAD')
E.g.finish()
d={'start':'START','states':{n:[m,{str(k):v for k,v in row.items()}] for n,(m,row) in E.g.st.items()},'seqs':[list(map(list,s)) for s in E.g.seqs]}
pathlib.Path(sys.argv[1]).write_text(json.dumps(d,separators=(',',':')))
print('lower states',len(d['states']),file=sys.stderr)
