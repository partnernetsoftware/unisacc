#!/usr/bin/env python3
"""Lowering data pass: raw E4 tape -> data header plus untouched tape code.
Instruction lowering is not yet applied. Uses the generic executor unchanged.
"""
import importlib.util,json,pathlib,sys
ROOT=pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT))
spec=importlib.util.spec_from_file_location('lowerbase',ROOT/'exec/parse/gen.py')
E=importlib.util.module_from_spec(spec);spec.loader.exec_module(E)
from data import install
install(E)
E.P('ACCEPTDATA').a(('ACCEPT',)).goto('DEAD')
E.g.finish()
d={'start':'START','states':{n:[m,{str(k):v for k,v in row.items()}] for n,(m,row) in E.g.st.items()},'seqs':[list(map(list,s)) for s in E.g.seqs]}
pathlib.Path(sys.argv[1]).write_text(json.dumps(d,separators=(',',':')))
print('lower data states',len(d['states']),file=sys.stderr)
