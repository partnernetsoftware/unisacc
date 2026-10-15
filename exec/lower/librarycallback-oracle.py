"""Callback materialization projection only; not full lowering/frame/ABI proof.
Reuse E3 callback_frames (single signature3 canonical+cycle qualification), Tape,
model-owned selection and existing UNILOW1 framing. No second frame matcher.
Explicit --reference-root pins the paired E3 reference source, not host state.
"""
import argparse,sys,struct,hashlib,json
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--reference-root',required=True);p.add_argument('--tape',required=True);p.add_argument('--contract',required=True);p.add_argument('--selection',required=True);p.add_argument('--graphs',required=True);p.add_argument('--target',type=int,default=1);a=p.parse_args()
sys.path.insert(0,str(Path(a.reference_root).resolve()))
from unisa.librarytypedreader import callback_frames
from unisa.libraryfnlower import frame,selection,check_provenance
from unisa.tape import parse
try:
 tape=Path(a.tape).read_text();contract=Path(a.contract).read_bytes();chosen=Path(a.selection).read_bytes();graphs=Path(a.graphs).read_bytes()
 check_provenance(tape)
 expected=callback_frames(contract,chosen,'used',None)
 if expected!=graphs:raise ValueError('library callback graph mismatch')
 if a.target not in (1,2,3,4) or contract[8]!=a.target:raise ValueError('not covered: callback target')
 owners=selection(chosen);ident=struct.unpack_from('<I',contract,20)[0];text=bytearray();spans=[]
 for insn in parse(tape).code:
  if insn.op!='.libraryfn':continue
  reg,symbol=insn.args
  if reg!='r1' or symbol!=ident or owners.get(symbol)!=1:raise ValueError('not covered: callback materialization projection')
  text.extend(('setreg '+('x1' if a.target in (2,4) else 'rdi')+', imm ').encode());spans.append((len(text),symbol));text.extend(b'0\n')
 if not spans:raise ValueError('not covered: callback materialization absent')
 sys.stdout.buffer.write(frame(bytes(text),spans))
except (ValueError,AssertionError,struct.error,IndexError,UnicodeError) as e:print(e,file=sys.stderr);sys.exit(1)
