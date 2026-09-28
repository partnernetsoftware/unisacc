#!/usr/bin/env python3
"""Host binary IO regression; Windows uses runtimeiobytes through vmcheck."""
import pathlib,subprocess,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='unisacc-binary-io-') as tmp:
 d=pathlib.Path(tmp);fixture=d/'bytes.bin';fixture.write_bytes(bytes(i%256 for i in range(65539)))
 for name,flags in [('native',[]),('sanitised',['-fsanitize=address,undefined','-fno-sanitize-recover=all'])]:
  exe=d/name;subprocess.run(['cc','-O2',*flags,str(ROOT/'tests/runtimeiobytes.c'),'-o',str(exe)],check=True,timeout=30)
  r=subprocess.run([str(exe),str(fixture)],capture_output=True,check=True,timeout=10)
  assert r.stdout==b'binary IO: 65539 bytes preserved\n' and not r.stderr,(r.stdout,r.stderr)
 print('binary IO: native and ASan/UBSan preserve 65539 bytes')
