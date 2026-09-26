#!/usr/bin/env python3
"""Raw tape lowering on the generic executor.
Default: data pass only. --full: Linux x86_64 target instructions and metadata.
"""
import importlib.util,json,pathlib,sys
ROOT=pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT))
spec=importlib.util.spec_from_file_location('lowerbase',ROOT/'exec/parse/gen.py')
E=importlib.util.module_from_spec(spec);spec.loader.exec_module(E)
from data import install
if len(sys.argv) not in (2,3) or (len(sys.argv)==3 and sys.argv[2]!="--full"):
    sys.exit("usage: gen.py OUT.json [--full]")
full=len(sys.argv)==3
install(E,code_start="C.prelude" if full else "H.code")
if full:
    from code import install as install_code
    install_code(E)
E.P('ACCEPTDATA').a(('ACCEPT',)).goto('DEAD')
E.g.finish()
d={'start':'START','states':{n:[m,{str(k):v for k,v in row.items()}] for n,(m,row) in E.g.st.items()},'seqs':[list(map(list,s)) for s in E.g.seqs]}
pathlib.Path(sys.argv[1]).write_text(json.dumps(d,separators=(',',':')))
print('lower states',len(d['states']),file=sys.stderr)
