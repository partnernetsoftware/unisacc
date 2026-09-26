#!/bin/sh
# The same network/action runtime compiled by cc and by unisacc.
# Models are freshly constructed; every stage's bytes and status must agree.
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
UA=${UA:-/tmp/ua_ref}; . ./tests/lib.sh; ua_ready
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { perl -e 'alarm 60; exec @ARGV' "$@"; }
b env EXEC_CC=cc NETWORK=1 TARGET=osx/arm64 ./exec/pipeline/elf.sh "$T" examples/hello.c examples/fib.c tests/c/b_toknames.c > "$T/log" 2>&1 || { cat "$T/log"; exit 1; }
b "$UA" -O2 unisacc.c -o "$T/compiler"
b "$T/compiler" -O2 exec/c/run.c -o "$T/run-ua"
b env EXEC_CC="$T/compiler" python3 exec/c/netcheck.py
b env EXEC_CC="$T/compiler" ./exec/c/neg.sh
b python3 - "$T" "$R" <<'PY'
import os,pathlib,resource,signal,subprocess,sys
p=pathlib.Path(sys.argv[1]);root=pathlib.Path(sys.argv[2]);ua=p/'run-ua'
def run(cmd):
    r=subprocess.run(list(map(str,cmd)),capture_output=True,timeout=60,
                     env=dict(os.environ,UNISA_MAXSTEPS='400000000000'))
    if r.returncode:raise SystemExit(f'{cmd}: rc {r.returncode}: {r.stderr.decode(errors="replace")}')
    return r.stdout
for stage in ['e2','e1','e3','e4','lower','elf']:
    run([ua,'--check-net',p/(stage+'.tbl'),p/(stage+'.net')])
for src in ['examples/hello.c','examples/fib.c','tests/c/b_toknames.c']:
    name=pathlib.Path(src).stem;inp=pathlib.Path(src)
    for stage in ['e2','e1','e3','e4','lower','elf']:
        got=run([ua,p/(stage+'.net'),inp,src,root/'include'])
        want=p/(name+'.'+('macho' if stage=='elf' else stage))
        if got!=want.read_bytes():raise SystemExit(f'{name} {stage}: executor output differs')
        inp=p/(name+'.ua.'+stage);inp.write_bytes(got)
    print('native executor:',src,'six stages equal')
# Both buffered host stdio and the unbuffered carried libc must report a
# real short output. Only the disposable output file has its size limited.
def limit():
    signal.signal(signal.SIGXFSZ,signal.SIG_IGN)
    resource.setrlimit(resource.RLIMIT_FSIZE,(1024,1024))
model=p/'write.net';model.write_text('N 1 1 0 0 0 0\nQ 2049 '+'12 65 '*2048+'49\nH 0 0 256 0 0 0\n')
empty=p/'empty';empty.write_bytes(b'')
for exe in [p/'run',ua]:
    with (p/'limited').open('wb') as out:
        r=subprocess.run([str(exe),str(model),str(empty)],stdout=out,stderr=subprocess.PIPE,preexec_fn=limit,timeout=60)
    assert r.returncode==2 and b'cannot write output' in r.stderr,(exe,r.returncode,r.stderr)
print('native executor: six full-domain checks, 18 outputs, two real write failures passed')
PY
