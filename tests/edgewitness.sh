#!/bin/sh
# 产品侧边级见证原件：参考侧录不到的边（e3->pfconv、elf->enc）用产品可见面冻结——
# 固定探针下该网的 asked 计数 + 生成程序的 stdout 字节 + 退出码。答案面由
# net-*-seal（金表全域）兜底；行为字节把「答案被用」钉住（答案错则输出变）。
# 这与 edgeseal.py 的键答映射件是不同强度的两类原件，seal-edges.tsv 分开计数；
# 口径见 tests/SEAL-MODULES.md「边级」节。
#
#   tests/edgewitness.sh --produce LEAF ROOT   # 双录判等才写 witness.tsv + out.bin
#   tests/edgewitness.sh --gate    LEAF ROOT   # 回放判等 + 钉扎 + 空见证即败 + 负例
#
# UA 约定与 backstages.sh 相同（tests/lib.sh ua_ready；不信任何旧 /tmp 二进制）。
set -u
mode=${1:?--gate|--produce}; L=${2:?leaf}; R=$(cd "${3:?root}" && pwd)
cd "$R" || exit 2
UA=${UA:-/tmp/ua_ref}
. "$R/tests/lib.sh"; ua_ready
sha() { (sha256sum "$1" 2>/dev/null || shasum -a 256 "$1") | cut -d' ' -f1; }
mf() { awk -F'\t' -v k="$1" '$1==k{print $2; exit}' "$L/$2"; }
net=$(mf net manifest.tsv); probe=$(mf probe manifest.tsv); flags=$(mf flags manifest.tsv)
[ -n "$net" ] && [ -n "$probe" ] || { echo "edgewitness: manifest lacks net/probe" >&2; exit 2; }

observe() {   # observe OUTDIR: compile + run, write asked / out.bin / exit
    "$R/tests/bound" 58 "$UA" "$R/$probe" $flags -o "$1/bin" -v 2>"$1/err" || { cat "$1/err" >&2; return 1; }
    n=$(sed -n "s/^asked .* $net \\([0-9][0-9]*\\).*/\\1/p" "$1/err" | tail -1)
    [ -n "$n" ] || n=$(sed -n "s/^asked $net \\([0-9][0-9]*\\).*/\\1/p" "$1/err" | tail -1)
    [ -n "$n" ] || { echo "edgewitness: no asked line for net $net" >&2; return 1; }
    "$R/tests/bound" 20 "$1/bin" > "$1/out.bin"; rc=$?
    printf 'asked\t%s\nexit\t%s\n' "$n" "$rc" > "$1/witness.tsv"
    return 0
}

case $mode in
--produce)
    A=$(mktemp -d); B=$(mktemp -d); trap 'sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge -- "${A:?}" "${B:?}"' EXIT
    observe "$A" || exit 1
    observe "$B" || exit 1
    cmp -s "$A/witness.tsv" "$B/witness.tsv" && cmp -s "$A/out.bin" "$B/out.bin" \
        || { echo "edgewitness: refuse to freeze: two observations differ" >&2; exit 1; }
    [ "$(mf asked "$A/witness.tsv" 2>/dev/null || awk -F'\t' '$1=="asked"{print $2}' "$A/witness.tsv")" != 0 ] \
        || { echo "edgewitness: refuse to freeze a zero-ask edge (probe never crossed it)" >&2; exit 1; }
    cp "$A/witness.tsv" "$L/witness.tsv"; cp "$A/out.bin" "$L/out.bin"
    python3 - "$L" "$R/$probe" "$R/weights/gold/$net.tsv" <<'PY'
import hashlib, pathlib, re, sys
leaf, probe, gold = sys.argv[1], sys.argv[2], sys.argv[3]
h = lambda p: hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()
m = pathlib.Path(leaf) / "manifest.tsv"; s = m.read_text()
s = re.sub(r"(?m)^probe_sha256\t.*$", "probe_sha256\t" + h(probe), s)
s = re.sub(r"(?m)^table_sha256\t.*$", "table_sha256\t" + h(gold), s)
s = re.sub(r"(?m)^out_sha256\t.*$", "out_sha256\t" + h(str(pathlib.Path(leaf) / "out.bin")), s)
m.write_text(s)
PY
    echo "edgewitness: froze product witness for net $net (asked $(awk -F'\t' '$1=="asked"{print $2}' "$L/witness.tsv"), double-observed)"
    ;;
--gate)
    for f in witness.tsv out.bin; do [ -s "$L/$f" ] || { echo "edgewitness: leaf lacks $f" >&2; exit 1; }; done
    [ "$(sha "$R/$probe")" = "$(mf probe_sha256 manifest.tsv)" ] || { echo "edgewitness: FAIL probe $probe changed since recording"; exit 1; }
    [ "$(sha "$R/weights/gold/$net.tsv")" = "$(mf table_sha256 manifest.tsv)" ] || { echo "edgewitness: FAIL gold table $net.tsv evolved; re-record"; exit 1; }
    [ "$(sha "$L/out.bin")" = "$(mf out_sha256 manifest.tsv)" ] || { echo "edgewitness: FAIL out.bin does not match manifest out_sha256"; exit 1; }
    W=$(mktemp -d); trap 'sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge -- "${W:?}"' EXIT
    observe "$W" || exit 1
    live_asked=$(awk -F'\t' '$1=="asked"{print $2}' "$W/witness.tsv")
    [ "$live_asked" != 0 ] || { echo "edgewitness: FAIL probe never crossed edge (net $net asked 0; nothing checked)"; exit 1; }
    cmp -s "$W/witness.tsv" "$L/witness.tsv" || { echo "edgewitness: FAIL witness differs"; diff "$L/witness.tsv" "$W/witness.tsv" | head -4; exit 1; }
    cmp "$W/out.bin" "$L/out.bin" || { echo "edgewitness: FAIL program output bytes differ from frozen out.bin"; exit 1; }
    # 负例：三类冻结数据变异必须被比较抓住（证明比较真的读数据）
    sed 's/^asked\t/asked\t9/' "$L/witness.tsv" > "$W/mut1"      # 计数前插 9
    cmp -s "$W/witness.tsv" "$W/mut1" && { echo "edgewitness: FAIL negative mutate-asked NOT caught"; exit 1; }
    sed 's/^exit\t/exit\t9/' "$L/witness.tsv" > "$W/mut2"
    cmp -s "$W/witness.tsv" "$W/mut2" && { echo "edgewitness: FAIL negative mutate-exit NOT caught"; exit 1; }
    { cat "$L/out.bin"; printf X; } > "$W/mut3"
    cmp -s "$W/out.bin" "$W/mut3" && { echo "edgewitness: FAIL negative mutate-out NOT caught"; exit 1; }
    echo "edgewitness: edge witness PASS net $net probe $probe: asked $live_asked, output bytes equal; negatives mutate-asked/exit/out all caught"
    ;;
*)  echo "edgewitness: unknown mode $mode" >&2; exit 2;;
esac
