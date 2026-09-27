#!/bin/sh
# Offline construction of the development compiler container. No deployment or
# overwrite of the shipped unisacc.com. Each child is bounded independently.
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
[ $# = 1 ] || { echo 'usage: buildcompiler.sh OUTPUT_DIR' >&2; exit 2; }
[ "$(uname -s)" = Darwin ] || { echo 'kernel seed assembler requires macOS' >&2; exit 2; }
. ./tests/lib.sh; ua_ready
mkdir -p "$1"; T=$(cd "$1" && pwd)
b() { perl -e 'alarm 60; exec @ARGV' "$@"; }
mkdir -p "$T/shared" "$T/kernels"
for arch in arm64 x86_64; do b python3 exec/c/asm/blob.py "$arch" "$T/kernels/$arch"; done
b python3 exec/lex/gen.py --typed "$T/shared/e1.json"
b python3 exec/parse2/gen2.py "$T/shared/e3.json"
b python3 exec/opt/gen.py "$T/shared/e4.json" 2
b python3 exec/opt/gen.py "$T/shared/o1.json" 1
for s in e1 e3 e4 o1; do
    b python3 exec/c/tbl.py "$T/shared/$s.json" "$T/shared/$s.tbl"
    b python3 exec/c/net.py "$T/shared/$s.tbl" "$T/shared/$s.net"
done
set --
for os in lnx osx win; do
    case $os in lnx) osflag=; image=elf;; osx) osflag=--osx; image=macho;; win) osflag=--win; image=pe;; esac
    for arch in arm64 x86_64; do
        case $arch in arm64) archflag=--arm64; enc=arm.py;; x86_64) archflag=; enc=gen.py;; esac
        d="$T/$os-$arch"; mkdir -p "$d"
        b python3 exec/pp/gen.py "$d/e2.json" "$os/$arch"
        b python3 exec/lower/gen.py "$d/lower.json" --full $osflag $archflag
        b python3 "exec/enc/$enc" "$d/elf.json" "--$image"
        for s in e2 lower elf; do
            b python3 exec/c/tbl.py "$d/$s.json" "$d/$s.tbl"
            b python3 exec/c/net.py "$d/$s.tbl" "$d/$s.net"
        done
        awk -v route="$os/$arch" '!/^#/ && NF {
            file=$4; if ($1=="e1" || $1=="e3" || $1=="e4") file="../shared/" file;
            print route "\t" $1 "\t" $2 "\t" $3 "\t" file
        }' exec/pipeline/image-stages.tsv > "$d/route.tsv"
        set -- "$@" "$d/route.tsv"
    done
done
b python3 exec/c/compilerpack.py --o1 "$T/shared/o1.net" --include include --kernels "$T/kernels" -o "$T/compiler.pkg" "$@"
b python3 -m unisa ape exec/c/asmcompiler.c --via "$UA" -O2 --payload "$T/compiler.pkg" -o "$T/unisacc-next.com"
chmod +x "$T/unisacc-next.com"
echo "development assembly/network compiler: $T/unisacc-next.com"
