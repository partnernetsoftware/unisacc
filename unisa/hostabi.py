"""Fixed six integer/pointer foreign ABI bridge declarations.
The declaration source is exec/enc/hostbridge.tsv; no language predicates.
"""
from pathlib import Path
_ROWS = [line.split('\t') for line in
         (Path(__file__).resolve().parents[1]/'exec/enc/hostbridge.tsv').read_text().splitlines()
         if line and not line.startswith('#')]
ARM_FN, ARM_ARGV = (int(next(v for a,p,v in _ROWS if a=='arm64' and p==name),16)
                    for name in ('fn','argv'))
ARM_BODY = tuple(int(v,16) for a,p,v in _ROWS if a=='arm64' and p=='body')
X86_BODY = bytes.fromhex(next(v for a,p,v in _ROWS if a=='x86_64' and p=='body'))

# Windows ARM GP ABI uses the same six integer argument registers; x18 untouched.
WIN_ARM_BODY = ARM_BODY
WIN_X86_BODY = bytes.fromhex(next(v for a,p,v in _ROWS if a=="win_x86_64" and p=="body"))
