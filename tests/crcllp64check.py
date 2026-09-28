"""CRC32 width and real P3 header reader; private host, unisacc and Windows COFF probes."""
import argparse,hashlib,json,pathlib,shutil,subprocess,tempfile,time,re
ROOT=pathlib.Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser();p.add_argument('--ua',type=pathlib.Path,required=True);p.add_argument('--evidence',type=pathlib.Path);p.add_argument('--keep',type=pathlib.Path);p.add_argument('--native-only',action='store_true');a=p.parse_args()
 td=a.keep or pathlib.Path(tempfile.mkdtemp(prefix='r10-crc-llp64-'));td.mkdir(parents=True,exist_ok=True)
 source=td/'source';(source/'exec/c').mkdir(parents=True,exist_ok=True);(source/'tests').mkdir(exist_ok=True)
 for f in (ROOT/'exec/c').iterdir():
  if f.is_file() and f.suffix in ('.h','.c'):shutil.copy2(f,source/'exec/c'/f.name)
 shutil.copy2(ROOT/'tests/crcllp64check.c',source/'tests/crcllp64check.c')
 shutil.copytree(ROOT/'include',source/'include',dirs_exist_ok=True)
 shutil.copytree(ROOT/'exec/c/asm',source/'exec/c/asm',dirs_exist_ok=True)
 def flatten(path):
  return re.sub(r'^#include "([^"\n]+)"\s*$',lambda m: flatten(path.parent/m.group(1)),path.read_text(),flags=re.M)
 (source/'tests/probe-flat.c').write_text(flatten(source/'tests/crcllp64check.c'))
 candidate=td/'ua-candidate.com';shutil.copy2(a.ua,candidate)
 ua_sha=hashlib.sha256(candidate.read_bytes()).hexdigest()
 rows=[]
 def run(cmd,expected=0,message=None):
  start=time.monotonic();r=subprocess.run(cmd,cwd=source,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,timeout=40)
  row=dict(command=list(map(str,cmd)),rc=r.returncode,elapsed=time.monotonic()-start,stdout=r.stdout,stderr=r.stderr);rows.append(row)
  if r.returncode!=expected or (message is not None and message not in r.stderr):raise RuntimeError(json.dumps(row))
  return r
 try:
  for name,cc in [('host',['cc','-std=c11','-O2']),('unisacc',['sh',str(candidate.resolve()),'-O2','-I',str(source/'include')]),('sanitised',['cc','-std=c11','-O1','-fsanitize=address,undefined'])]:
   binary=td/name;run(cc+['tests/probe-flat.c','-o',str(binary)])
   run([str(binary)])
   for value,extra in [('2147483648',[]),('4294967295',['max'])]:
    f=td/('good-'+value+'.pkg');f.write_bytes(('P 3 1 1 0\nD r s a b 0\nM 1 1 1 '+value+'\nX').encode());run([str(binary),str(f),*extra])
   for value in ['-1','4294967296','6442450944','8589934591','9223372036854775807','9223372036854775808']:
    f=td/('bad-'+value+'.pkg');f.write_bytes(('P 3 1 1 0\nD r s a b 0\nM 1 1 1 '+value+'\nX').encode());run([str(binary),str(f)],2,'model number overflow' if value=='9223372036854775808' else 'bad compressed model header')
  zig=shutil.which('zig')
  if not zig and not a.native_only:raise RuntimeError('Zig required for actual Windows LLP64 COFF proof')
  for target in ([] if a.native_only else ['x86_64-windows-gnu','aarch64-windows-gnu']):
   obj=td/(target+'.o');run([zig,'cc','-target',target,'-std=c11','-DCRC_COFF_ABI','-Werror','-c','tests/crcllp64check.c','-o',str(obj)])
   run(['file',str(obj)]);rows.append(dict(object=str(obj),sha256=hashlib.sha256(obj.read_bytes()).hexdigest(),bytes=obj.stat().st_size))
 finally:
  result=dict(windows_cross_compiled=not a.native_only,scope='native host/unisacc execution; optional Windows two-ISA COFF compilation only, not Windows execution',source=str(source),ua_sha256=ua_sha,source_sha256={str(f.relative_to(source)):hashlib.sha256(f.read_bytes()).hexdigest() for f in [source/'exec/c/codec.h',source/'exec/c/run.c',source/'tests/crcllp64check.c']},steps=rows)
  dest=a.evidence or td/'evidence.json';dest.write_text(json.dumps(result,indent=2)+'\n');print(dest)
if __name__=='__main__':main()
