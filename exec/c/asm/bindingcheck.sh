#!/bin/sh
# Real unisacc ABI, carried library and assembly kernel, not a host-C bridge.
set -eu
R=$(cd "$(dirname "$0")/../../.." && pwd); cd "$R"
. ./tests/lib.sh; ua_ready
[ "$(uname -s)" = Darwin ] || { echo 'blob seed construction requires macOS' >&2; exit 2; }
ARCH=${CORE_ASM_ARCH:-$(uname -m)}
case $ARCH in arm64|x86_64) ;; *) exit 2;; esac
T=$(mktemp -d);trap 'rm -rf "$T"' EXIT
b() { perl -e 'alarm 60; exec @ARGV' "$@"; }
b python3 exec/c/asm/blob.py "$ARCH" "$T/kernel.blob"
mkdir "$T/kernels"
for ISA in arm64 x86_64; do b python3 exec/c/asm/blob.py "$ISA" "$T/kernels/$ISA"; done
export CORE_ASM_ARCH=$ARCH UNISA_KERNEL="$T/kernel.blob"
export EXEC_CC="$R/exec/c/asm/blobcc.sh" TARGET="osx/$ARCH"
b python3 exec/c/netcheck.py
b ./exec/pipeline/elf.sh "$T" examples/hello.c examples/fib.c tests/c/b_strderef.c exec/c/run.c > "$T/build.log" 2>&1 || { cat "$T/build.log"; exit 1; }
b python3 exec/opt/gen.py "$T/o1.json" 1
b python3 exec/c/tbl.py "$T/o1.json" "$T/o1.tbl"
b python3 exec/c/net.py "$T/o1.tbl" "$T/o1.net"
b python3 exec/c/compilerpack.py --o1 "$T/o1.net" --include include --kernels "$T/kernels" -o "$T/compiler.pkg" "$T/route.tsv"
b "$EXEC_CC" -O2 exec/c/compiler.c -o "$T/compiler"
b python3 - "$T" "$TARGET" "$UA" <<'PY'
import os,pathlib,subprocess,sys
p=pathlib.Path(sys.argv[1]);target=sys.argv[2];ua=sys.argv[3]
# Compiler invocations must use the carried kernel, not the standalone test's
# environment path. Standalone negative probes below supply explicit envs.
os.environ.pop('UNISA_KERNEL',None)
def run(args,**kw):return subprocess.run(list(map(str,args)),capture_output=True,timeout=60,**kw)
def ok(args,**kw):
 r=run(args,**kw);assert r.returncode==0,(r.args,r.returncode,r.stderr);return r.stdout
for src in ['examples/hello.c','examples/fib.c','tests/c/b_strderef.c','exec/c/run.c']:
 image=p/(pathlib.Path(src).stem+'.macho')
 assert image.read_bytes()==ok([ua,'-O2',src,'-b',target]),src
 if src!='exec/c/run.c':
  r=run([image]);ref=p/'ref';ok([ua,'-O2',src,'-b',target,'-o',ref]);q=run([ref])
  assert (r.returncode,r.stdout,r.stderr)==(q.returncode,q.stdout,q.stderr),src
 print('product-ABI assembly route image equal:',src)
base=[p/'compiler','--models',p/'compiler.pkg']
for level in (0,1,2):
 for mode in ('-E','-S','-b'):
  flags=['-b',target,mode]+([target] if mode=='-b' else [])+['-O'+str(level)]
  assert ok([*base,'examples/hello.c',*flags])==ok([ua,'examples/hello.c',*flags]),flags
 for src in ('examples/hello.c','tests/c/b_argv.c','tests/c/b_static.c'):
  flags=['-O'+str(level),'-run',src,'one','two']
  r=run([*base,*flags]);q=run([ua,*flags]);assert 0<=q.returncode<128
  assert (r.returncode,r.stdout,r.stderr)==(q.returncode,q.stdout,q.stderr),(src,level,r,q)
for pair in [('tests/multi/m1.c','tests/multi/m2.c'),('tests/multi/static1.c','tests/multi/static2.c')]:
 flags=['-run',*pair,'-O2'];r=run([*base,*flags]);q=run([ua,*flags])
 assert 0<=q.returncode<128 and (r.returncode,r.stdout,r.stderr)==(q.returncode,q.stdout,q.stderr),(pair,r,q)
# Rebuild the same assembly-bound driver through its networks. The fixed
# assembly blob remains an explicit source artifact; no assembler is invoked
# at runtime. Both generations must match the seed driver's complete image.
selfsrc='exec/c/compiler.c'
n1=p/'compiler';n2=p/'compiler-n2';n3=p/'compiler-n3'
for old,new in ((n1,n2),(n2,n3)):
 image=ok([old,'--models',p/'compiler.pkg','-DUNISA_CORE_BLOB',selfsrc,'-O2','-b',target])
 assert image==n1.read_bytes(), 'assembly-bound driver self-rebuild differs'
 new.write_bytes(image);new.chmod(0o755)
assert ok([n3,'--models',p/'compiler.pkg','-run','examples/hello.c'])==b'hello from C99\n'
print('product-ABI assembly driver: N1=N2=N3, rebuilt through networks, kernel carried in package')
# The runtime requires the explicit binding; it cannot silently run core.c.
badenv=dict(os.environ);badenv.pop('UNISA_KERNEL',None)
r=run([p/'run',p/'e1.net',p/'hello.e2'],env=badenv)
assert r.returncode==2 and b'assembly kernel not specified' in r.stderr and not r.stdout
for change in ('isa','entry','slot','length','short'):
 data=bytearray((p/'kernel.blob').read_bytes())
 if change=='short':data=data[:16]
 else:
  off={'isa':8,'entry':16,'slot':24,'length':32}[change]
  data[off:off+8]=(3 if change=='isa' else len(data)+1).to_bytes(8,'little')
 invalid=p/'invalid.blob';invalid.write_bytes(data)
 r=run([p/'run',p/'e1.net',p/'hello.e2'],env=dict(os.environ,UNISA_KERNEL=str(invalid)))
 assert r.returncode==2 and b'kernel' in r.stderr and not r.stdout,(change,r)
print('product-ABI binding: four images, nine CLI modes, nine memory runs, two multi-unit runs, required blob/bounds passed')
PY
