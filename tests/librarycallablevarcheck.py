#!/usr/bin/env python3
"""Concrete variadic callable ABI tests. No model/public API success claim."""
import pathlib, subprocess, sys, tempfile, platform
ROOT=pathlib.Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='unisacc-callablevar-') as name:
 d=pathlib.Path(name)
 for arch in (['arm64','x86_64'] if platform.system()=='Darwin' else [platform.machine()]):
  for sanitized in (False,True):
   exe=d/(arch+('-asan' if sanitized else '-normal'))
   args=['cc','-std=c11','-Wall','-Wextra','-Wno-unused-function','-Wno-misleading-indentation','-I'+str(ROOT/'exec/c'),str(ROOT/'tests/librarycallablevarcheck.c'),'-lffi','-o',str(exe)]
   if platform.system()=='Darwin':args += ['-arch',arch]
   if sanitized:args += ['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer']
   else:args += ['-O2']
   p=subprocess.run([str(ROOT/'tests/bound'),'20',*args],capture_output=True);assert p.returncode==0,(arch,sanitized,p.stderr)
   run=[str(exe)]
   if platform.system()=='Darwin' and arch=='x86_64':run=['arch','-x86_64',*run]
   p=subprocess.run([str(ROOT/'tests/bound'),'20',*run],capture_output=True);assert p.returncode==0,(arch,sanitized,p.stdout,p.stderr)
   print(arch,'ASan/UBSan' if sanitized else 'normal',p.stdout.decode().strip(),flush=True)
