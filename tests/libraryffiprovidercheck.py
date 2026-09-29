#!/usr/bin/env python3
"""Provider identity guards and queue dependency bytes; not a native ABI proof."""
import hashlib,importlib.util,json,os,sys,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'exec/c'))
from buildlibrary import provider_flags
FILES=('lib/libffi.a','include/ffi.h','include/ffitarget.h','include/ffi/ffi.h','include/ffi/ffitarget.h','LICENSE')
def main():
 records=[]
 with tempfile.TemporaryDirectory(prefix='ffi-provider-controls-') as folder:
  d=Path(folder).resolve();facts={}
  # Synthetic files test identity validation only. Real ABI qualification is
  # mandatory in buildlibrary before publication, independently of this test.
  for name in FILES:
   p=d/name;p.parent.mkdir(parents=True,exist_ok=True);data=('synthetic '+name).encode();p.write_bytes(data);facts[name]={'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}
  m={'schema':1,'target':'osx/x86_64','version':'3.5.2','source_tar_sha256':'f3a3082a23b37c293a4fcd1053147b371f2ff91fa7ea1b2a52e335676bac82dc','artifacts':facts};(d/'manifest.json').write_text(json.dumps(m))
  flags,links,_=provider_flags(d,'osx/x86_64');assert flags==['-I'+str(d/'include')] and links==[str(d/'lib/libffi.a')];records.append('exact identity')
  def reject(label,target='osx/x86_64'):
   try:provider_flags(d,target)
   except ValueError:records.append(label);return
   raise AssertionError('accepted '+label)
  for name in FILES:
   p=d/name;original=p.read_bytes();p.write_bytes(bytes([original[0]^1])+original[1:]);reject('tamper '+name);p.write_bytes(original)
  reject('wrong target','osx/arm64')
  original=(d/'manifest.json').read_text();(d/'manifest.json').write_text('[]');reject('non-object manifest');(d/'manifest.json').write_text(original)
  spec=importlib.util.spec_from_file_location('gq',ROOT/'tests/gatequeue.py');gq=importlib.util.module_from_spec(spec);spec.loader.exec_module(gq)
  old=os.environ.get('UNISACC_FFI_X86_PROVIDER');os.environ['UNISACC_FFI_X86_PROVIDER']=str(d)
  try:
   jobs={'provider-control':['python3','./tests/libraryunion16check.py']};before=gq.fingerprint(jobs);p=d/'lib/libffi.a';data=p.read_bytes();p.write_bytes(data+b'X');after=gq.fingerprint(jobs);assert before!=after;records.append('queue external archive change')
  finally:
   if old is None:os.environ.pop('UNISACC_FFI_X86_PROVIDER',None)
   else:os.environ['UNISACC_FFI_X86_PROVIDER']=old
 print(json.dumps({'controls':records,'scope':'identity and queue invalidation only; not actual ABI'}))
if __name__=='__main__':main()
