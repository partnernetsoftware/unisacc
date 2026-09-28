#!/usr/bin/env python3
"""Actual product calls, independent from parser-fact/query assertions."""
import argparse,json,pathlib,subprocess,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--package',type=pathlib.Path,required=True);a=p.parse_args()
product=a.package.resolve(strict=True)
source=(ROOT/'exec/parse2/probes/callback_full_signature.c').read_text()
assert source.count('int main(void)')==1
head=source[:source.index('int main(void)')]
variants={
 'direct9':'return weighted(1,2,3,4,5,6,7,8,9.0)!=285.0f;',
 'ptr9':'F9 a=weighted;return a(1,2,3,4,5,6,7,8,9.0)!=285.0f;',
 'direct17':'return seventeen(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17.0)!=153.0f;',
 'ptr17':'F17 a=seventeen;return a(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17.0)!=153.0f;',
 'returned9':'Relay r=pick;return r(weighted)(1,2,3,4,5,6,7,8,9.0)!=285.0f;',
 'pick9':'F9 a=pick(weighted);return a(1,2,3,4,5,6,7,8,9.0)!=285.0f;'}
receipts=[]
with tempfile.TemporaryDirectory(prefix='r10-fullsignature-') as name:
 t=pathlib.Path(name)
 for label,text in [*( (k,head+'int main(void){'+v+'}\n') for k,v in variants.items()),('nested',source)]:
  f=t/(label+'.c');f.write_text(text)
  for opt in (0,1,2):
   command=[str(ROOT/'tests/bound'),'10','sh',str(product),'-O'+str(opt),'-run',str(f)]
   r=subprocess.run(command,capture_output=True,timeout=12)
   if r.returncode or r.stdout or r.stderr:
    raise SystemExit(f'full signature {label} O{opt}: rc={r.returncode}, stdout={r.stdout!r}, stderr={r.stderr!r}')
   receipts.append(dict(probe=label,opt=opt,exit=0))
assert len(receipts)==21
print(json.dumps(dict(actual_product_calls=21,fp9=True,mixed17=True,returned_function_pointer=True,nested_callback_types=True,native_callback_bridge=False)))
