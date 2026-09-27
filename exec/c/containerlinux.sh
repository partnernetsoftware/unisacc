#!/bin/sh
# Opt-in real Linux execution. Requires an already-running Lima VM with this
# source tree readable, cc and Python for the independent test referee.
# Does not start/stop a VM. Every command is bounded; skips are not passes.
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
[ $# = 1 ] || { echo 'usage: containerlinux.sh BUILD_DIR' >&2; exit 2; }
. ./tests/lib.sh; ua_ready
perl -e 'alarm 60; exec @ARGV' python3 - "$1" "$R" "$UA" <<'PY'
import os,pathlib,subprocess,sys,tempfile
build=pathlib.Path(sys.argv[1]).resolve();root=pathlib.Path(sys.argv[2]);ua=sys.argv[3]
vm=os.environ.get('LIMA_VM','default')
def run(args,**kw):
 r=subprocess.run(list(map(str,args)),capture_output=True,timeout=60,**kw)
 if r.returncode: raise RuntimeError((r.args,r.returncode,r.stdout,r.stderr))
 return r.stdout
arch=run(['limactl','shell',vm,'uname','-m']).decode().strip()
target={'aarch64':'lnx/arm64','x86_64':'lnx/x86_64'}[arch]
guest=run(['limactl','shell',vm,'mktemp','-d','/tmp/unisacc-carried.XXXXXX']).decode().strip()
try:
 with tempfile.TemporaryDirectory(prefix='carried-linux-') as td:
  d=pathlib.Path(td);refs=[]
  for os_ in ('lnx','osx','win'):
   for isa in ('arm64','x86_64'):
    ref=d/(os_+'-'+isa+'.ref');refs.append(ref)
    ref.write_bytes(run([ua,'-O2',root/'examples/hello.c','-b',os_+'/'+isa]))
  ref=d/'seed.ref';refs.append(ref)
  ref.write_bytes(run([ua,'-O2',root/'exec/c/asmcompiler.c','-b',target]))
  run(['limactl','copy',build/'unisacc-next.com',vm+':'+guest+'/compiler.com'])
  run(['limactl','copy',*refs,vm+':'+guest+'/'])
  result=run(['limactl','shell',vm,'python3',root/'exec/c/containerlinux.py',guest,root,target])
  print(result.decode(),end='')
finally:
 run(['limactl','shell',vm,'rm','-rf',guest])
PY
