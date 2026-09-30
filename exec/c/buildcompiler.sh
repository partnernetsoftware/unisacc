#!/bin/sh
# Offline construction only; shared/target domains permit external parallel scheduling.
set -eu
PYTHONHASHSEED=${PYTHONHASHSEED:-0}; export PYTHONHASHSEED
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
[ $# -ge 1 ] && [ $# -le 2 ] || { echo 'usage: buildcompiler.sh OUTPUT_DIR [shared|OS/ARCH|pack]' >&2; exit 2; }
step=${2:-all}
case $step in all|shared|pack|pack-models|pack-driver|lnx/arm64|lnx/x86_64|osx/arm64|osx/x86_64|win/arm64|win/x86_64) ;; *) echo "unknown build step: $step" >&2; exit 2;; esac
[ "$(uname -s)" = Darwin ] || { echo 'kernel seed assembler requires macOS' >&2; exit 2; }
mkdir -p "$1"; T=$(cd "$1" && pwd)
b() { python3 "$R/tests/bound.py" 50 "$@"; }
# Reuse the model preparation identity/digest implementation. These are completion
# records, not a result cache: requested stages always rebuild their own outputs.
manifest() {
    b python3 - "$T" "$1" "$2" <<'PYCODE'
import hashlib,json,os,pathlib,sys
sys.path.insert(0,'exec/pipeline')
from models import identity,digest
root=pathlib.Path(sys.argv[1]);mode,stage=sys.argv[2:]
key=identity('compiler-container','1','cc')
settings={k:v for k,v in os.environ.items() if k.startswith(('E1','E2','E3','E4','PYTHONHASHSEED'))}
assembly={str(p):digest(p) for p in sorted(pathlib.Path('exec/c/asm').glob('*.S'))}
key=hashlib.sha256((key+json.dumps([settings,assembly],sort_keys=True)).encode()).hexdigest()
if mode=='identity': print(key);raise SystemExit
names=([f'shared/{s}.{e}' for s in ('e2','e1','e3','e4','o1','prune','nativeabi') for e in ('json','tbl','net')]
       + ['kernels/arm64','kernels/x86_64']) if stage=='shared' else (
       [f'{stage}/{s}.{e}' for s in ('lower','elf') for e in ('json','tbl','net')]+[f'{stage}/route.tsv'])
record=root/stage/'manifest.json'
if mode=='write':
    expected=(root/stage/'input.sha256').read_text().strip()
    if key!=expected: raise SystemExit('model source changed during '+stage)
    data={'inputs':key,'outputs':{name:digest(root/name) for name in names}}
    if any((root/name).stat().st_size==0 for name in names): raise SystemExit('empty stage output')
    tmp=record.with_suffix('.tmp');tmp.write_text(json.dumps(data,sort_keys=True));tmp.replace(record)
else:
    try: data=json.loads(record.read_text())
    except FileNotFoundError: raise SystemExit('missing completed dependency: '+stage)
    if data.get('inputs')!=key or set(data.get('outputs',{}))!=set(names):
        raise SystemExit('stale stage dependency: '+stage)
    for name,sha in data['outputs'].items():
        if digest(root/name)!=sha: raise SystemExit('changed stage output: '+name)
PYCODE
}
start() {
    mkdir -p "$T/$1"
    rm -f "$T/$1/manifest.json"
    manifest identity "$1" > "$T/$1/input.sha256"
}
shared() {
    start shared
    mkdir -p "$T/kernels"
    for arch in arm64 x86_64; do b python3 exec/c/asm/blob.py "$arch" "$T/kernels/$arch"; done
    b python3 exec/pp/gen.py "$T/shared/e2.json" --shared-predefines
    b python3 exec/lex/gen.py --typed "$T/shared/e1.json"
    b python3 exec/parse2/gen2.py "$T/shared/e3.json"
    b python3 exec/opt/gen.py "$T/shared/e4.json" 2
    b python3 exec/opt/gen.py "$T/shared/o1.json" 1
    b python3 exec/prune/gen.py "$T/shared/prune.json"
    b python3 exec/nativeabi/gen.py "$T/shared/nativeabi.json"
    for s in e2 e1 e3 e4 o1 prune nativeabi; do
        b python3 exec/c/tbl.py "$T/shared/$s.json" "$T/shared/$s.tbl"
        b python3 exec/c/net.py "$T/shared/$s.tbl" "$T/shared/$s.net"
    done
    manifest write shared
    echo 'completed compiler shared models and kernels'
}
target() {
    os=${1%/*}; arch=${1#*/}; name=$os-$arch
    start "$name"; d="$T/$name"
    case $os in lnx) osflag=; image=elf;; osx) osflag=--osx; image=macho;; win) osflag=--win; image=pe;; esac
    case $arch in arm64) archflag=--arm64; enc=arm.py;; x86_64) archflag=; enc=gen.py;; esac
    b python3 exec/lower/gen.py "$d/lower.json" --full $osflag $archflag
    b python3 "exec/enc/$enc" "$d/elf.json" "--$image"
    for s in lower elf; do
        b python3 exec/c/tbl.py "$d/$s.json" "$d/$s.tbl"
        b python3 exec/c/net.py "$d/$s.tbl" "$d/$s.net"
    done
    awk -v route="$os/$arch" '!/^#/ && NF {
        file=$4; if ($1=="e2" || $1=="e1" || $1=="e3" || $1=="e4" || $1=="prune") file="../shared/" file;
        print route "\t" $1 "\t" $2 "\t" $3 "\t" file
    }' exec/pipeline/image-stages.tsv > "$d/route.tsv"
    manifest write "$name"
    echo "completed compiler target $os/$arch"
}
pack_models() {
    source_start=$(b python3 exec/c/provenance.py identity)
    manifest check shared
    set --
    for os in lnx osx win; do
        for arch in arm64 x86_64; do
            manifest check "$os-$arch"
            set -- "$@" "$T/$os-$arch/route.tsv"
        done
    done
    # Only packaging needs the classic seed; never race its creation in target jobs.
    . ./tests/lib.sh
    b sh -c 'R=$1; . "$R/tests/lib.sh"; ua_ready' seed "$R" # caller-selected UA honored
    codec_flag=
    case ${PACK_COMPRESSED:-1} in 0) codec_flag=--legacy-package;; 1) codec_flag=--compressed;; *) echo "PACK_COMPRESSED must be 0 or 1" >&2; exit 2;; esac
    b python3 exec/c/compilerpack.py $codec_flag --no-model-cache --shared-e2 "$T/shared/e2.net" --shared-nativeabi "$T/shared/nativeabi.net" --o1 "$T/shared/o1.net" --include include --kernels "$T/kernels" --audit-dir "$T/model-audit" -o "$T/compiler.pkg" "$@"
    [ -s "$T/compiler.pkg" ]
    printf '%s\n' "$source_start" > "$T/compiler.pkg.source"   # the identity pack-driver seals with
    echo "model package: $T/compiler.pkg"
}
pack_driver() {
    # The second half of pack: the APE driver around the package pack-models wrote.
    # Split out because with the SEED as UA (comboot stage 2/3) the whole of pack
    # took ~50 s of CPU against a 55 s bound (R14-2 (2)).
    [ -s "$T/compiler.pkg" ] && [ -s "$T/compiler.pkg.source" ] || { echo "pack-driver: run pack-models first" >&2; exit 1; }
    source_start=$(cat "$T/compiler.pkg.source")
    [ "$(b python3 exec/c/provenance.py identity)" = "$source_start" ] || { echo "pack-driver: sources changed since pack-models" >&2; exit 1; }
    . ./tests/lib.sh
    b sh -c 'R=$1; . "$R/tests/lib.sh"; ua_ready' seed "$R"
    product_version=$(python3 -c 'import re; from pathlib import Path; m=re.findall(r"#define UNISACC_VERSION \"([0-9.]+)\"",Path("src/version.h").read_text()); assert len(m)==1; print(m[0])')
    b python3 -m unisa ape exec/c/asmcompiler.c --via "$UA" -O2 --payload "$T/compiler.pkg" --product-name Unisacc --product-version "$product_version" -o "$T/unisacc-next.com"
    [ -s "$T/unisacc-next.com" ]
    manifest check shared
    for os in lnx osx win; do for arch in arm64 x86_64; do manifest check "$os-$arch"; done; done
    b python3 exec/c/provenance.py write "$T/unisacc-next.com" "$source_start"
    chmod +x "$T/unisacc-next.com"
    echo "development assembly/network compiler: $T/unisacc-next.com"
}
pack() { pack_models; pack_driver; }
case $step in
    all)
        # One process-tree budget for the entire build, including all children.
        python3 "$R/tests/bound.py" 55 sh -c '
            set -eu
            script=$1; out=$2
            pair() {
                sh "$script" "$out" "$1" & a=$!
                sh "$script" "$out" "$2" & b=$!
                ra=0; wait "$a" || ra=$?
                rb=0; wait "$b" || rb=$?
                [ "$ra" -eq 0 ] || return "$ra"
                [ "$rb" -eq 0 ] || return "$rb"
            }
            pair shared lnx/arm64
            pair lnx/x86_64 osx/arm64
            pair osx/x86_64 win/arm64
            sh "$script" "$out" win/x86_64
            sh "$script" "$out" pack
        ' model-build "$R/exec/c/buildcompiler.sh" "$T";;
    shared) shared;; pack) pack;; pack-models) pack_models;; pack-driver) pack_driver;; *) target "$step";;
esac
