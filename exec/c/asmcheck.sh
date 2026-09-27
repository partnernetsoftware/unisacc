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
KERNEL_ASM=""
for unit in transition arith buffer memory intern bytes format stack action run; do
    KERNEL_ASM="$KERNEL_ASM exec/c/asm/${unit}_${ARCH}.S"
done
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
b cc $ARCHFLAG -Os -Wall -Wextra -DCORE_BUFFER_LINKAGE= -Dcore_put=core_put_c -Drealloc=buffer_realloc \
    exec/c/core.c "exec/c/asm/buffer_$ARCH.S" exec/c/asm/layoutcheck.c exec/c/asm/buffercheck.c -o "$T/buffer"
b "$T/buffer"
b python3 - "$T/buffer" <<'PYBUFFER'
import subprocess,sys
for case in range(1,4):
 for version in ['c','a']:
  r=subprocess.run([sys.argv[1],version,str(case)],capture_output=True,timeout=10)
  reason='buffer capacity overflow' if case==3 else 'out of memory'
  calls=0 if case==3 else case
  message=f'SIMULATED panic checked: {reason}, calls {calls}\n'.encode()
  assert (r.returncode,r.stdout,r.stderr)==(2,message,b''),(case,version,r)
print('buffer failure paths: C/ASM both reject, 6 simulated checks')
PYBUFFER
b cc $ARCHFLAG -c "exec/c/asm/buffer_$ARCH.S" -o "$T/buffer.o"
if [ "$OS" = osx ]; then b size -m "$T/buffer.o"; else b size -A "$T/buffer.o"; fi
b cc $ARCHFLAG -Os -Wall -Wextra -DCORE_MEMORY_LINKAGE= -Dcore_mget=core_mget_c -Dcore_mset=core_mset_c -Dcalloc=memory_calloc \
    exec/c/core.c "exec/c/asm/memory_$ARCH.S" exec/c/asm/layoutcheck.c exec/c/asm/memorycheck.c -o "$T/memory"
b "$T/memory"
b python3 - "$T/memory" <<'PYMEMORY'
import subprocess,sys
for case in range(1,5):
 for version in ['c','a']:
  r=subprocess.run([sys.argv[1],version,str(case)],capture_output=True,timeout=10)
  reason='memory capacity overflow' if case==4 else 'out of memory'
  calls=0 if case==4 else 3
  message=f'SIMULATED map panic checked: {reason}, calls {calls}\n'.encode()
  assert (r.returncode,r.stdout,r.stderr)==(2,message,b''),(case,version,r)
print('sparse memory failure paths: C/ASM both reject, 8 simulated checks')
PYMEMORY
b cc $ARCHFLAG -c "exec/c/asm/memory_$ARCH.S" -o "$T/memory.o"
if [ "$OS" = osx ]; then b size -m "$T/memory.o"; else b size -A "$T/memory.o"; fi
b cc $ARCHFLAG -Os -Wall -Wextra -DCORE_INTERN_LINKAGE= -Dcore_hbytes=core_hbytes_c -Dcore_intern=core_intern_c -Dcalloc=intern_calloc -Drealloc=intern_realloc \
    exec/c/core.c "exec/c/asm/intern_$ARCH.S" exec/c/asm/layoutcheck.c exec/c/asm/interncheck.c -o "$T/intern"
b "$T/intern"
b python3 - "$T/intern" <<'PYINTERN'
import subprocess,sys
for case in range(1,5):
 for version in ['c','a']:
  r=subprocess.run([sys.argv[1],version,str(case)],capture_output=True,timeout=10)
  reason='intern capacity overflow' if case==3 else 'out of memory'
  nc=0 if case==3 else 1; nr=1 if case==2 else 0
  message=f'SIMULATED intern panic checked: {reason}, calloc {nc}, realloc {nr}\n'.encode()
  assert (r.returncode,r.stdout,r.stderr)==(2,message,b''),(case,version,r)
print('intern failure paths: C/ASM both reject, 8 simulated checks')
PYINTERN
b cc $ARCHFLAG -c "exec/c/asm/intern_$ARCH.S" -o "$T/intern.o"
if [ "$OS" = osx ]; then b size -m "$T/intern.o"; else b size -A "$T/intern.o"; fi
b cc $ARCHFLAG -Os -Wall -Wextra -DCORE_BYTES_LINKAGE= -Dcore_badd=core_badd_c -Dcore_rfind=core_rfind_c -Drealloc=bytes_realloc -Dfree=bytes_free \
    exec/c/core.c "exec/c/asm/bytes_$ARCH.S" exec/c/asm/layoutcheck.c exec/c/asm/bytescheck.c -o "$T/bytes"
b "$T/bytes"
b python3 - "$T/bytes" <<'PYBYTES'
import subprocess,sys
for case in range(1,6):
 for version in ['c','a']:
  r=subprocess.run([sys.argv[1],version,str(case)],capture_output=True,timeout=10)
  reason='blob capacity overflow' if case==5 else 'out of memory'
  calls=0 if case==5 else case if case<3 else case-2
  fetches=1 if case in [3,4] else 0
  message=f'SIMULATED byte-store panic checked: {reason}, allocations {calls}, fetches {fetches}\n'.encode()
  assert (r.returncode,r.stdout,r.stderr)==(2,message,b''),(case,version,r)
print('byte store failure paths: C/ASM both reject, 10 simulated checks')
PYBYTES
b cc $ARCHFLAG -c "exec/c/asm/bytes_$ARCH.S" -o "$T/bytes.o"
if [ "$OS" = osx ]; then b size -m "$T/bytes.o"; else b size -A "$T/bytes.o"; fi
b cc $ARCHFLAG -c "exec/c/asm/arith_$ARCH.S" -o "$T/arith.o"
if [ "$OS" = osx ]; then b size -m "$T/arith.o"; else b size -A "$T/arith.o"; fi
b cc $ARCHFLAG -c "exec/c/asm/transition_$ARCH.S" -o "$T/transition.o"
if [ "$OS" = osx ]; then b size -m "$T/transition.o"; else b size -A "$T/transition.o"; fi
b cc $ARCHFLAG -Os -Wall -Wextra -DCORE_FORMAT_LINKAGE= -Ddecimal=decimal_c -Dfield_fill=field_fill_c \
    exec/c/core.c "exec/c/asm/format_$ARCH.S" exec/c/asm/layoutcheck.c exec/c/asm/formatcheck.c -o "$T/format"
b "$T/format"
b python3 - "$T/format" <<'PYFORMAT'
import subprocess,sys
for case in range(5):
 for version in ['c','a']:
  r=subprocess.run([sys.argv[1],version,str(case)],capture_output=True,timeout=10)
  assert (r.returncode,r.stdout,r.stderr)==(2,b'field bounds: rejected before writing\n',b''),(case,version,r)
print('field bounds: C/ASM both reject, 10 checks')
PYFORMAT
b cc $ARCHFLAG -Os -Wall -Wextra -DCORE_STACK_LINKAGE= -Dstack_push=stack_push_c -Dstack_pop=stack_pop_c -Dframe_push=frame_push_c -Dframe_pop=frame_pop_c -Drealloc=stack_realloc \
    exec/c/core.c "exec/c/asm/stack_$ARCH.S" exec/c/asm/layoutcheck.c exec/c/asm/stackcheck.c -o "$T/stack"
b "$T/stack"
b python3 - "$T/stack" <<'PYSTACK'
import subprocess,sys
for case in range(1,8):
 for version in ['c','a']:
  r=subprocess.run([sys.argv[1],version,str(case)],capture_output=True,timeout=10)
  reason='out of memory' if case<=4 else 'stack capacity overflow' if case==5 else 'frame capacity overflow' if case==6 else 'pop of an empty stack'
  assert (r.returncode,r.stdout,r.stderr)==(2,('SIMULATED stack panic: '+reason+'\n').encode(),b''),(case,version,r)
print('stack failure paths: C/ASM both reject, 14 simulated checks')
PYSTACK
b cc $ARCHFLAG -Os -Wall -Wextra -DCORE_RUN_NAME=core_run_c -DCORE_ACTION_LINKAGE= -Daction_run=action_run_c -DCORE_TRANSITION_NAME=core_transition_c \
    exec/c/core.c $KERNEL_ASM exec/c/asm/layoutcheck.c exec/c/asm/actioncheck.c -o "$T/action"
b "$T/action"
b python3 - "$T/action" <<'PYACTION'
import subprocess,sys
for version in ['c','a']:
 r=subprocess.run([sys.argv[1],version],capture_output=True,timeout=10)
 assert (r.returncode,r.stdout,r.stderr)==(2,b'bad action rejected\n',b''),r
print('bad action: C/ASM both reject')
PYACTION
b cc $ARCHFLAG -Os -Wall -Wextra -DCORE_RUN_NAME=core_run_c -DCORE_TRANSITION_NAME=core_transition_c -Dcalloc=run_calloc -Drealloc=run_realloc -Dfree=run_free \
    exec/c/core.c $KERNEL_ASM exec/c/asm/layoutcheck.c exec/c/asm/runcheck.c -o "$T/lifecycle"
b "$T/lifecycle"
b python3 - "$T/lifecycle" <<'PYRUN'
import subprocess,sys
for allocation in [1,2]:
 for version in ['c','a']:
  r=subprocess.run([sys.argv[1],version,str(allocation)],capture_output=True,timeout=10)
  assert (r.returncode,r.stdout,r.stderr)==(2,f'SIMULATED initial calloc {allocation} rejected\n'.encode(),b''),r
print('initial allocation failure: C/ASM both reject, 4 simulated checks')
PYRUN
b env EXEC_CC="$R/exec/c/asm/cc.sh" python3 exec/c/netcheck.py
b env EXEC_CC="$R/exec/c/asm/cc.sh" NETWORK=1 TARGET="$OS/$ARCH" \
    ./exec/pipeline/elf.sh "$T" examples/hello.c examples/fib.c tests/c/b_strderef.c exec/parse2/probes/prefix_members.c exec/parse2/probes/member_index_address.c exec/c/run.c > "$T/build.log" 2>&1 || { cat "$T/build.log"; exit 1; }
b python3 - "$T" "$OS/$ARCH" "$UA" <<'PY'
import pathlib,subprocess,sys
p=pathlib.Path(sys.argv[1]);target=sys.argv[2];ua=sys.argv[3]
def run(cmd):return subprocess.run(list(map(str,cmd)),capture_output=True,timeout=60)
def ok(cmd):
 r=run(cmd);assert r.returncode==0,(r.args,r.returncode,r.stderr);return r.stdout
ext='macho' if target.startswith('osx/') else 'elf'
for src in ['examples/hello.c','examples/fib.c','tests/c/b_strderef.c','exec/parse2/probes/prefix_members.c','exec/parse2/probes/member_index_address.c','exec/c/run.c']:
 image=p/(pathlib.Path(src).stem+'.'+ext)
 assert image.read_bytes()==ok([ua,'-O2',src,'-b',target]),src
 if src!='exec/c/run.c':
  host=p/'host';args=['cc','-O2','-include','stdio.h',src,'-o',host]
  if sys.platform=='darwin':args[1:1]=['-arch',target.split('/')[1]]
  ok(args);a=run([image]);b=run([host])
  assert (a.returncode,a.stdout,a.stderr)==(b.returncode,b.stdout,b.stderr),(src,a,b)
  if src.endswith('prefix_members.c'):assert (a.returncode,a.stdout,a.stderr)==(40,b'',b''),a
  if src.endswith('member_index_address.c'):assert (a.returncode,a.stdout,a.stderr)==(25,b'',b''),a
 print('assembly inference six-stage image equal:',src)
# The image produced through ASM inference is the same C runtime. Its
# execution must not be mistaken for a fully assembly-built action engine.
model=p/'e1.net';source=p/'hello.e2'
assert ok([p/('run.'+ext),model,source])==ok([p/'run',model,source])
print('ASM inference/arithmetic/storage route: six full images equal; five native runs equal; C-runtime output equal')
PY
