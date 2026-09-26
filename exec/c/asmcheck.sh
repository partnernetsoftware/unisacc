#!/bin/sh
# One real host ISA per bounded job; x86_64 on this ARM Mac uses Rosetta.
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
. ./tests/lib.sh; ua_ready
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { perl -e 'alarm 60; exec @ARGV' "$@"; }
ARCH=${CORE_ASM_ARCH:-$(uname -m)}
case $ARCH in aarch64) ARCH=arm64;; arm64|x86_64) ;; *) exit 2;; esac
case $(uname -s) in Darwin) OS=osx; ARCHFLAG="-arch $ARCH";; Linux) OS=lnx; ARCHFLAG=;; *) exit 2;; esac
export CORE_ASM_ARCH=$ARCH
b cc $ARCHFLAG -Os -Wall -Wextra -DCORE_TRANSITION_NAME=core_transition_c \
    exec/c/core.c "exec/c/asm/transition_$ARCH.S" exec/c/asm/layoutcheck.c \
    exec/c/asm/transitioncheck.c -o "$T/check"
b "$T/check"
b cc $ARCHFLAG -Os -Wall -Wextra -DCORE_ALU_LINKAGE= -Dalu32=core_alu32_c -Dalu64=core_alu64_c \
    exec/c/core.c "exec/c/asm/arith_$ARCH.S" exec/c/asm/arithcheck.c -o "$T/arith"
b "$T/arith"
b python3 - "$T/arith" <<'PYTEST'
import subprocess,sys
for bits,ops in [(32,[-1,10,2147483647]),(64,[-1,3,4,16,2147483647])]:
 for op in ops:
  for version in ['c','a']:
   r=subprocess.run([sys.argv[1],version+str(bits),str(op)],capture_output=True,timeout=10)
   message=b'panic:bad alu op\n' if bits==32 else b'panic:bad alu64 op\n'
   assert (r.returncode,r.stdout,r.stderr)==(2,b'',message),(bits,op,version,r)
print('word arithmetic invalid operations: C/ASM both reject, 16 checks')
PYTEST
b cc $ARCHFLAG -c "exec/c/asm/arith_$ARCH.S" -o "$T/arith.o"
if [ "$OS" = osx ]; then b size -m "$T/arith.o"; else b size -A "$T/arith.o"; fi
b cc $ARCHFLAG -c "exec/c/asm/transition_$ARCH.S" -o "$T/transition.o"
if [ "$OS" = osx ]; then b size -m "$T/transition.o"; else b size -A "$T/transition.o"; fi
b env EXEC_CC="$R/exec/c/asm/cc.sh" python3 exec/c/netcheck.py
b env EXEC_CC="$R/exec/c/asm/cc.sh" NETWORK=1 TARGET="$OS/$ARCH" \
    ./exec/pipeline/elf.sh "$T" examples/hello.c examples/fib.c tests/c/b_strderef.c exec/c/run.c > "$T/build.log" 2>&1 || { cat "$T/build.log"; exit 1; }
b python3 - "$T" "$OS/$ARCH" "$UA" <<'PY'
import pathlib,subprocess,sys
p=pathlib.Path(sys.argv[1]);target=sys.argv[2];ua=sys.argv[3]
def run(cmd):return subprocess.run(list(map(str,cmd)),capture_output=True,timeout=60)
def ok(cmd):
 r=run(cmd);assert r.returncode==0,(r.args,r.returncode,r.stderr);return r.stdout
ext='macho' if target.startswith('osx/') else 'elf'
for src in ['examples/hello.c','examples/fib.c','tests/c/b_strderef.c','exec/c/run.c']:
 image=p/(pathlib.Path(src).stem+'.'+ext)
 assert image.read_bytes()==ok([ua,'-O2',src,'-b',target]),src
 if src!='exec/c/run.c':
  host=p/'host';args=['cc','-O2','-include','stdio.h',src,'-o',host]
  if sys.platform=='darwin':args[1:1]=['-arch',target.split('/')[1]]
  ok(args);a=run([image]);b=run([host])
  assert (a.returncode,a.stdout,a.stderr)==(b.returncode,b.stdout,b.stderr),(src,a,b)
 print('assembly inference six-stage image equal:',src)
# The image produced through ASM inference is the same C runtime. Its
# execution must not be mistaken for a fully assembly-built action engine.
model=p/'e1.net';source=p/'hello.e2'
assert ok([p/('run.'+ext),model,source])==ok([p/'run',model,source])
print('ASM inference/arithmetic route: four full images equal; three native runs equal; C-runtime output equal')
PY
