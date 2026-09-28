#!/usr/bin/env python3
"""Explicit fixture certificates only; no model/public carrier acceptance claim."""
import argparse,os,platform,shutil,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--arch',choices=['arm64','x86_64']);a=p.parse_args()
host='arm64' if platform.machine() in ('arm64','aarch64') else 'x86_64';arch=a.arch or host
flags=['-arch',arch] if platform.system()=='Darwin' else []
def run(*args):subprocess.run([str(ROOT/'tests/bound'),'20',*map(str,args)],check=True)
with tempfile.TemporaryDirectory(prefix='r10-carrier-mechanical-') as tmp:
 t=Path(tmp);rt=t/'headers';rt.mkdir()
 for f in (ROOT/'exec/c').glob('*.h'):shutil.copy2(f,rt/f.name)
 shutil.copy2(ROOT/'tests/librarycarriercheck.c',t/'probe.c')
 for san in (False,True):
  exe=t/('san' if san else 'normal')
  run(os.environ.get('CC','cc'),*flags,'-std=c11','-O1','-Wall','-Wextra','-Wno-unused-function','-I',rt,t/'probe.c','-lffi','-o',exe,*(['-fsanitize=address,undefined','-fno-omit-frame-pointer','-g'] if san else []))
  run(*(['arch','-x86_64'] if host=='arm64' and arch=='x86_64' else []),exe)
print('explicit carrier mechanical normal + ASan/UBSan passed on '+arch)
