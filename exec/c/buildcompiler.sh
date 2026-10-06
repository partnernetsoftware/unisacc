#!/bin/sh
# Offline construction only; shared/target domains permit external parallel scheduling.
set -eu
PYTHONHASHSEED=${PYTHONHASHSEED:-0}; export PYTHONHASHSEED
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
[ $# -ge 1 ] && [ $# -le 2 ] || { echo 'usage: buildcompiler.sh OUTPUT_DIR [shared|OS/ARCH|pack]' >&2; exit 2; }
step=${2:-all}
case $step in all|shared|pack|pack-prep-1|pack-prep-2|pack-prep-3|pack-models|pack-driver|lnx/arm64|lnx/x86_64|osx/arm64|osx/x86_64|win/arm64|win/x86_64) ;; *) echo "unknown build step: $step" >&2; exit 2;; esac
[ "$(uname -s)" = Darwin ] || { echo 'kernel seed assembler requires macOS' >&2; exit 2; }
mkdir -p "$1"; T=$(cd "$1" && pwd)
b() { "$R/tests/bound" 50 "$@"; }   # the native watchdog (0.0.32 B5: no python3 for the bound itself)
# 0.0.25 B3: SEED_C=1 turns delta JSON -> table -> network into the C99 seed tools (seed/tbl.c,
# seed/net.c) built by the installed previous release, unisacc.com, instead of exec/c/tbl.py and
# net.py.  Both routes give the same bytes (tests/seedconstructmatrix.py; 0.0.25 B3 built byte-identical
# candidates both ways).  Default since 0.0.26 B3': the C tools; SEED_C=0 keeps the Python pair.
tn() {   # tn JSON TBL NET
    if [ "${SEED_C:-1}" = 1 ]; then
        if [ ! -x "$T/seedbin/seed-net" ] || [ ! -x "$T/seedbin/seed-tbl" ]; then
            mkdir -p "$T/seedbin"
            b sh "$R/unisacc.com" "$R/seed/tbl.c" -o "$T/seedbin/seed-tbl"
            b sh "$R/unisacc.com" "$R/seed/net.c" -o "$T/seedbin/seed-net"
        fi
        b "$T/seedbin/seed-tbl" "$1" "$2"
        b "$T/seedbin/seed-net" "$2" "$3"
    else
        b python3 exec/c/tbl.py "$1" "$2"
        b python3 exec/c/net.py "$2" "$3"
    fi
}
# 0.0.32 B5: SEED_GEN=1 (default) constructs the delta JSON of the stages seed/gen.c covers with the
# C99 constructor instead of exec/build/gen.py; same bytes (tests/seedgencheck.sh).  Stages it does not
# cover yet (lower) stay on gen.py.  Built by the host cc for now, not unisacc.com: a .com -O2
# seed-gen is byte-equal but runs parse2 in ~26 s against ~7 s (plans/v0.0.32.md D2).
gen() {   # gen STAGE OUT.json [FLAG...]
    if [ "${SEED_GEN:-1}" = 1 ]; then
        if [ ! -x "$T/seedbin/seed-gen" ]; then
            mkdir -p "$T/seedbin"; b ${SEED_GEN_CC:-cc} -std=c99 -O2 -w -I"$R/seed" "$R/seed/gen.c" -o "$T/seedbin/seed-gen"
        fi
        b "$T/seedbin/seed-gen" "$@" >/dev/null
    else
        b python3 exec/build/gen.py "$@"
    fi
}
# The product source identity and build record (= exec/c/provenance.py identity / write) from
# seed/ident.c; SEED_GEN=0 keeps Python.  `ident` prints the identity, `ident write ART START` records.
ident() {
    if [ "${SEED_GEN:-1}" = 1 ]; then
        if [ ! -x "$T/seedbin/ident" ]; then
            mkdir -p "$T/seedbin"; b ${SEED_GEN_CC:-cc} -std=c99 -D_POSIX_C_SOURCE=200809L -O2 -w "$R/seed/ident.c" -o "$T/seedbin/ident.$$" && mv -f "$T/seedbin/ident.$$" "$T/seedbin/ident"   # parts build in parallel
        fi
        b "$T/seedbin/ident" "$R" "$@"
    elif [ $# = 0 ]; then
        b python3 exec/c/provenance.py identity
    else
        b python3 exec/c/provenance.py "$@"
    fi
}
# Reuse the model preparation identity/digest implementation. These are completion
# records, not a result cache: requested stages always rebuild their own outputs.
# seed/ident.c's `manifest` mode is the default (B5); SEED_GEN=0 keeps this Python.  The two
# keys differ, so a tree switched between them reads as stale and rebuilds, never as fresh.
manifest() {
    if [ "${SEED_GEN:-1}" = 1 ]; then ident manifest "$1" "$T" "$2"; return; fi
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
    # 0.0.32 B5: seed/blob.c cuts the kernel blob (byte-equal to exec/c/asm/blob.py); SEED_GEN=0 keeps Python
    if [ "${SEED_GEN:-1}" = 1 ]; then
        if [ ! -x "$T/seedbin/blob" ]; then
            mkdir -p "$T/seedbin"; b ${SEED_GEN_CC:-cc} -std=c99 -D_DARWIN_C_SOURCE -D_POSIX_C_SOURCE=200809L -O2 -w "$R/seed/blob.c" -o "$T/seedbin/blob.$$" && mv -f "$T/seedbin/blob.$$" "$T/seedbin/blob"
        fi
        for arch in arm64 x86_64; do b "$T/seedbin/blob" "$R" "$arch" "$T/kernels/$arch"; done
    else
        for arch in arm64 x86_64; do b python3 exec/c/asm/blob.py "$arch" "$T/kernels/$arch"; done
    fi
    gen pp "$T/shared/e2.json" --shared-predefines
    gen lex "$T/shared/e1.json" --typed
    gen parse2 "$T/shared/e3.json"
    gen opt "$T/shared/e4.json" --o2
    gen opt "$T/shared/o1.json"
    gen prune "$T/shared/prune.json"
    gen nativeabi "$T/shared/nativeabi.json"
    for s in e2 e1 e3 e4 o1 prune nativeabi; do
        tn "$T/shared/$s.json" "$T/shared/$s.tbl" "$T/shared/$s.net"
    done
    manifest write shared
    echo 'completed compiler shared models and kernels'
}
target() {
    os=${1%/*}; arch=${1#*/}; name=$os-$arch
    start "$name"; d="$T/$name"
    case $os in lnx) osflag=; image=elf;; osx) osflag=--osx; image=macho;; win) osflag=--win; image=pe;; esac
    case $arch in arm64) archflag=--arm64; enc=enc/arm;; x86_64) archflag=; enc=enc;; esac
    b python3 exec/build/gen.py lower "$d/lower.json" --full $osflag $archflag
    gen "$enc" "$d/elf.json" "--$image"
    for s in lower elf; do
        tn "$d/$s.json" "$d/$s.tbl" "$d/$s.net"
    done
    awk -v route="$os/$arch" '!/^#/ && NF {
        file=$4; if ($1=="e2" || $1=="e1" || $1=="e3" || $1=="e4" || $1=="prune") file="../shared/" file;
        print route "\t" $1 "\t" $2 "\t" $3 "\t" file
    }' exec/pipeline/image-stages.tsv > "$d/route.tsv"
    manifest write "$name"
    echo "completed compiler target $os/$arch"
}
pack_prep() {
    # Part $1 of the model construction (compilerpack.py PREP_PARTS) into this
    # build's own cache, so each bounded step stays well under 55 s (0.0.23: one
    # pack-models constructing every model ran past the bound).  No package here.
    manifest check shared
    routes=
    for os in lnx osx win; do
        for arch in arm64 x86_64; do
            manifest check "$os-$arch"
            routes="$routes $T/$os-$arch/route.tsv"
        done
    done
    UNISACC_MODEL_CACHE="$T/model-cache" b python3 exec/c/compilerpack.py --prepare-only --part "$1/3" --shared-e2 "$T/shared/e2.net" --shared-nativeabi "$T/shared/nativeabi.net" --o1 "$T/shared/o1.net" --include include -o /dev/null $routes
    ident > "$T/pack-prep-$1.done"   # completion record (comboot step manifest)
    echo "prepared model part $1/3"
}
pack_models() {
    source_start=$(ident)
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
    # Fresh construction per candidate: the model cache lives inside this build's own
    # directory (empty before pack-prep-1); pack-prep-1..3 fill it and pack-models only
    # packages (--require-cached: a missing model is an error, not a slow rebuild).
    # Cache hits re-check the product digests.
    UNISACC_MODEL_CACHE="$T/model-cache" b python3 exec/c/compilerpack.py --require-cached $codec_flag --shared-e2 "$T/shared/e2.net" --shared-nativeabi "$T/shared/nativeabi.net" --o1 "$T/shared/o1.net" --include include --kernels "$T/kernels" --audit-dir "$T/model-audit" -o "$T/compiler.pkg" "$@"
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
    [ "$(ident)" = "$source_start" ] || { echo "pack-driver: sources changed since pack-models" >&2; exit 1; }
    . ./tests/lib.sh
    b sh -c 'R=$1; . "$R/tests/lib.sh"; ua_ready' seed "$R"
    product_version=$(awk -F'"' '/^#define UNISACC_VERSION "[0-9.]+"/ { n++; v = $2 } END { if (n != 1) exit 1; print v }' src/version.h)
    b python3 -m unisa ape exec/c/asmcompiler.c --via "$UA" -O2 --payload "$T/compiler.pkg" --product-name Unisacc --product-version "$product_version" -o "$T/unisacc-next.com"
    [ -s "$T/unisacc-next.com" ]
    manifest check shared
    for os in lnx osx win; do for arch in arm64 x86_64; do manifest check "$os-$arch"; done; done
    ident write "$T/unisacc-next.com" "$source_start"
    chmod +x "$T/unisacc-next.com"
    echo "development assembly/network compiler: $T/unisacc-next.com"
}
pack() { pack_models; pack_driver; }
case $step in
    all)
        # One process-tree budget for the entire build, including all children.
        "$R/tests/bound" 55 sh -c '
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
            sh "$script" "$out" pack-prep-1
            sh "$script" "$out" pack-prep-2
            sh "$script" "$out" pack-prep-3
            sh "$script" "$out" pack
        ' model-build "$R/exec/c/buildcompiler.sh" "$T";;
    shared) shared;; pack) pack;; pack-prep-[123]) pack_prep "${step#pack-prep-}";; pack-models) pack_models;; pack-driver) pack_driver;; *) target "$step";;
esac
