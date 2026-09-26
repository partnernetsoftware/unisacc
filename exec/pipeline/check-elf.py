#!/usr/bin/env python3
"""Referees only: source compiler -O2 tape and its complete Linux ELF image."""
import pathlib,subprocess,sys
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[2]))
from unisa.tape import parse,DATA_BASE
from unisa.lower import lower
from unisa.__main__ import _oracle
from unisa.assemble import assemble
from unisa import image
if len(sys.argv)<4:raise SystemExit('usage: check-elf.py UA OUTPUT_DIR SOURCE...')
p=pathlib.Path(sys.argv[2]);oracle=_oracle('built')
for f in sys.argv[3:]:
 name=pathlib.Path(f).stem
 r=subprocess.run([sys.argv[1],'-O2','-S',f,'-o','-'],capture_output=True,timeout=60)
 if r.returncode or r.stdout!=(p/(name+'.e4')).read_bytes():raise RuntimeError(f+' reference failed or tape differs')
 tp=lower(parse(r.stdout.decode()),'lnx/x86_64',oracle,drive='built');code,st=assemble(tp)
 if st['encoded']!=st['insns']:raise RuntimeError('reference has unencoded instructions')
 want=image.build(tp,code,image.relocate(tp,tp.data,st['data_va']-DATA_BASE),st['entry'])
 if want!=(p/(name+'.elf')).read_bytes():raise RuntimeError(f+' image differs')
 print('source-to-ELF',f,len(want),'bytes equal',flush=True)
