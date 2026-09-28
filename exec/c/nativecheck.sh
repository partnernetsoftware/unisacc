#!/bin/sh
_BOUND=$(cd "$(dirname "$0")/../.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
# The same network/action runtime compiled by cc and by unisacc.
# Models are freshly constructed; every stage's bytes and status must agree.
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
UA=${UA:-/tmp/ua_ref}; . ./tests/lib.sh; ua_ready
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { "$_BOUND" 60 "$@"; }
PART=${NATIVE_PART:-all}
case $PART in
 all|stages) INPUTS="examples/hello.c examples/fib.c tests/c/b_toknames.c exec/c/run.c";;
 chain) INPUTS="examples/hello.c tests/c/b_toknames.c exec/c/run.c";;
 resources) INPUTS="exec/c/run.c";;
 *) echo 'unknown NATIVE_PART' >&2; exit 2;;
esac
b env EXEC_CC=cc NETWORK=1 TARGET=osx/arm64 ./exec/pipeline/elf.sh "$T" $INPUTS > "$T/log" 2>&1 || { cat "$T/log"; exit 1; }
b "$UA" -O2 unisacc.c -o "$T/compiler"
b "$T/compiler" -O2 exec/c/run.c -o "$T/run-ua"
if [ "$PART" = all ] || [ "$PART" = stages ]; then
 b env EXEC_CC="$T/compiler" python3 exec/c/netcheck.py
 b env EXEC_CC="$T/compiler" ./exec/c/neg.sh
fi
b python3 - "$T" "$R" "$PART" <<'PY'
import os,pathlib,resource,shutil,signal,subprocess,sys
p=pathlib.Path(sys.argv[1]);root=pathlib.Path(sys.argv[2]);ua=p/'run-ua'
def run(cmd, cwd=None):
    r=subprocess.run(list(map(str,cmd)),capture_output=True,timeout=60,cwd=cwd,
                     env=dict(os.environ,UNISA_MAXSTEPS='400000000000'))
    if r.returncode:raise SystemExit(f'{cmd}: rc {r.returncode}: {r.stderr.decode(errors="replace")}')
    return r.stdout
part=sys.argv[3]; netrun=p/'run.macho'
assert netrun.read_bytes()==ua.read_bytes(), 'network runtime != compiler-built runtime'
if part in ('all', 'stages'):
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
    # The six network stages now compile their own C runtime.  The resulting
    # runtime repeats those stages on its own source, without Python processing it.
    for stage in ['e2','e1','e3','e4','lower','elf']:
        run([netrun,'--check-net',p/(stage+'.tbl'),p/(stage+'.net')])
    src='exec/c/run.c';inp=pathlib.Path(src)
    for stage in ['e2','e1','e3','e4','lower','elf']:
        got=run([netrun,p/(stage+'.net'),inp,src,root/'include'])
        want=p/('run.'+('macho' if stage=='elf' else stage))
        if got!=want.read_bytes():raise SystemExit(f'network-built runtime self stage {stage}: differs')
        inp=p/('run.self.'+stage);inp.write_bytes(got)
    print('network-built runtime: six self stages and image equal; six domain checks passed')
if part in ('all', 'chain'):
    # The same stages also run within a single process, passing only bytes.
    models=[p/(stage+'.net') for stage in ['e2','e1','e3','e4','lower','elf']]
    for exe in [p/'run',ua,netrun]:
        for src in ['examples/hello.c','tests/c/b_toknames.c','exec/c/run.c']:
            got=run([exe,'--chain',src,src,root/'include',*models])
            want=p/(pathlib.Path(src).stem+'.macho')
            if got!=want.read_bytes():raise SystemExit(f'{exe} in-memory chain differs: {src}')
    print('stream chain: three runtime builds, including network self-rebuild, equal')
if part in ('all', 'resources'):
    isolated=p/'isolated';isolated.mkdir()
    shutil.copyfile(root/'exec/c/run.c',isolated/'runtime.c')
    for name in ['core.c','core.h','codec.h','packagefooter.h']: shutil.copyfile(root/'exec/c'/name,isolated/name)
    shutil.copyfile(p/'models.pkg',isolated/'models.pkg')
    for exe in [p/'run',ua,netrun]:
        got=run([exe,'--bundle','models.pkg','osx/arm64','runtime.c','runtime.c'],cwd=isolated)
        if got!=netrun.read_bytes():raise SystemExit(f'{exe} resource-packaged self route differs')
    print('resource package: three runtime builds reproduce self, no include directory, isolated cwd')
    sys.path.insert(0,str(root/'exec/c'))
    from pack import build as package_build
    (isolated/'without-resources.pkg').write_bytes(package_build([p/'route.tsv']))
    r=subprocess.run([str(netrun),'--bundle','without-resources.pkg','osx/arm64','runtime.c','runtime.c'],cwd=isolated,capture_output=True,timeout=60)
    assert r.returncode==2 and not r.stdout and b'no include directory' in r.stderr,(r.returncode,r.stderr)
    print('resource-free control: isolated self build refuses, no partial output')


    # Both buffered host stdio and the unbuffered carried libc must report a
    # real short output. Only the disposable output file has its size limited.
    def limit():
        signal.signal(signal.SIGXFSZ,signal.SIG_IGN)
        resource.setrlimit(resource.RLIMIT_FSIZE,(1024,1024))
    model=p/'write.net';model.write_text('N 1 1 0 0 0 0\nQ 2049 '+'12 65 '*2048+'49\nH 0 0 256 0 0 0\n')
    empty=p/'empty';empty.write_bytes(b'')
    for exe in [p/'run',ua,netrun]:
        with (p/'limited').open('wb') as out:
            r=subprocess.run([str(exe),str(model),str(empty)],stdout=out,stderr=subprocess.PIPE,preexec_fn=limit,timeout=60)
        assert r.returncode==2 and b'cannot write output' in r.stderr,(exe,r.returncode,r.stderr)
    print('native executor resources: isolated packages and three real write failures passed')
PY
