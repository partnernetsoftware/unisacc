"""Table-declared conservative original-span prune delta.

The table is the transition source.  Its opcode/register facts were expanded
from unisa.tape at the recorded schema digest; a schema edit requires a table
review and update rather than silently changing the product network.
"""
import hashlib
import importlib.util
import json
import sys
from pathlib import Path

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
sys.path.insert(0, str(ROOT / 'exec'))
from finite_rules import install as install_rules
from unisa.tape import SHAPE, REGS

SCHEMA_SHA = '2008f75dfb75ecd52509ecb3e0b6536796691003451bbdffc28071d906e3cf42'
shape_bytes = json.dumps({'shape': SHAPE, 'regs': REGS}, sort_keys=True,
                         separators=(',', ':')).encode()
if hashlib.sha256(shape_bytes).hexdigest() != SCHEMA_SHA:
    raise SystemExit('prune table schema stale: review opcode/register facts')

spec = importlib.util.spec_from_file_location('prune_assembler', ROOT / 'exec/parse/gen.py')
E = importlib.util.module_from_spec(spec)
spec.loader.exec_module(E)
g = E.g
SRCMAX, ROWMAX, NAMEMAX, EDGEMAX = 2097152, 32768, 8192, 65536


def construct():
    install_rules(g, Path(__file__).parent, 'prune', section='main')
    # The original constructor replaced CLASS after allocating its first
    # continuation; keep that unreachable return-key for a full-domain match.
    g.labels.add('CLASS.r8')
    g.finish()
    return {'start': 'START', 'states': {n: [mode, {str(k): v for k, v in row.items()}]
                                        for n, (mode, row) in g.st.items()}, 'seqs': g.seqs}


if __name__ == '__main__':
    if len(sys.argv) != 2:
        raise SystemExit('usage: gen.py PRIVATE_DELTA.json')
    d = construct()
    Path(sys.argv[1]).write_text(json.dumps(d, separators=(',', ':')) + '\n')
    print(json.dumps({'states': len(d['states']), 'sequences': len(d['seqs']),
                      'schema_sha256': hashlib.sha256((ROOT / 'unisa/tape.py').read_bytes()).hexdigest(),
                      'capacities': [SRCMAX, ROWMAX, NAMEMAX, EDGEMAX]}))
