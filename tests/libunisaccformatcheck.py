"""Bounded private-snapshot loader and lifetime assertions, host + sanitizers."""
import pathlib,shutil,subprocess,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
def main():
 with tempfile.TemporaryDirectory(prefix='r10-library-format-') as name:
  td=pathlib.Path(name);(td/'exec/c').mkdir(parents=True);(td/'src').mkdir();(td/'tests').mkdir()
  for p in (ROOT/'exec/c').iterdir():
   if p.is_file() and p.suffix in ('.h','.c','.S'):shutil.copy2(p,td/'exec/c'/p.name)
  shutil.copy2(ROOT/'src/host_dl.h',td/'src/host_dl.h');shutil.copy2(ROOT/'tests/libunisaccformatcheck.c',td/'tests/libunisaccformatcheck.c')
  for flags,stem in [([], 'plain'),(['-fsanitize=address,undefined'],'sanitised')]:
   subprocess.run(['cc','-std=c11','-O1',*flags,str(td/'tests/libunisaccformatcheck.c'),str(td/'exec/c'/('librarycall_'+('arm64' if __import__('platform').machine() in ('arm64','aarch64') else 'x86_64')+'.S')),'-lffi','-o',str(td/stem)],check=True,timeout=20)
   subprocess.run([str(ROOT/'tests/bound'),'15',str(td/stem)],check=True,timeout=20)
if __name__=='__main__':main()
