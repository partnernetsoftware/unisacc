#!/usr/bin/env bash
# M2/M3/P0 product gate (Paper B 出货脊 — not IntNet):
#   default compile.mjs → compiler_core.wasm; stage2≡stage1; sim/drone body≡stage0;
#   no python3 emit on ship path; no A-core copy for Pages.
#
# UJS-1_ship fold: arith · fact · branch · f64 · list · setidx · dict · globals_fold ·
#   list_f64 · unary_minus · elseif · logic · str · setidx_globals · sim/drone ship emit
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
# shellcheck source=ujs2wasm/gate_prelude.sh
source "$ROOT/tests/ujs2wasm/gate_prelude.sh"
# shellcheck source=ujs2wasm/gate_classify.sh
source "$ROOT/tests/ujs2wasm/gate_classify.sh"

echo "== ujs2wasm_compiler (M2/M3/P0 · UJS-1_ship) =="

ujs_gate_prelude
# UJS_ALLOW_MISSING_COMPILER_WASM is intentionally unsupported (never skip-as-green).

command -v node >/dev/null || { echo "needs node"; exit 1; }
OUT="${TMPDIR:-/tmp}/ujs2wasm-compiler.$$"
mkdir -p "$OUT"
trap 'rm -rf "$OUT"' EXIT

if [[ -n "${TINYVM:-}" ]]; then
  echo "tinyvm: $TINYVM (fold+ship module validate; not execute twin)"
else
  echo "tinyvm: skip validate (not found; Node instantiate still gates)"
fi

echo "artifact: $COMPILER_WASM ($(wc -c < "$ROOT/$COMPILER_WASM") bytes)"

TRAPBIN="$OUT/bin"
mkdir -p "$TRAPBIN"
cat > "$TRAPBIN/python3" <<'STUB'
#!/bin/sh
echo "ujs2wasm_compiler: python3 must not run on product path" >&2
exit 127
STUB
chmod +x "$TRAPBIN/python3"
export PATH="$TRAPBIN:$PATH"
# Product-path compiles default to core; stage0 compares set REQUIRE locally.
unset UJS_REQUIRE_COMPILER_WASM || true
unset UJS_COMPILER || true

EXPECT="$ROOT/tests/ujs2wasm/expect.json"
RUNNER="$ROOT/tests/ujs2wasm/run_wasm.mjs"
STEP_RUNNER="$ROOT/tests/ujs2wasm/run_step.mjs"
STEP_EXPECT="$ROOT/tests/ujs2wasm/step_expect.json"

for name in arith fact branch f64_arith list setidx dict globals_fold list_f64 unary_minus elseif logic str; do
  src="tests/ujs2wasm/corpus/${name}.ujs"
  echo "-- compile $name via compile.mjs (no python3)"
  log=$(perl -e 'alarm 60; exec @ARGV' node ujs/compile.mjs "$src" -o "$OUT/${name}.wasm")
  echo "$log"
  [ -f "$OUT/${name}.wasm" ] || { echo "FAIL: no wasm for $name"; exit 1; }
  magic=$(head -c 4 "$OUT/${name}.wasm" | od -An -tx1 | tr -d ' \n')
  [ "$magic" = "0061736d" ] || { echo "FAIL: $name not \\0asm ($magic)"; exit 1; }
  echo "$log" | grep -q '"bridge":"compiler_core.wasm"' \
    || { echo "FAIL: fold corpus expects compiler_core bridge, got: $log"; exit 1; }
  got=$(perl -e 'alarm 30; exec @ARGV' node "$RUNNER" "$OUT/${name}.wasm" | node -e \
    'let d="";process.stdin.on("data",c=>d+=c);process.stdin.on("end",()=>{process.stdout.write(String(JSON.parse(d.trim())))})')
  want=$(node -e "const e=JSON.parse(require('fs').readFileSync(process.argv[1],'utf8')); process.stdout.write(String(e[process.argv[2]]))" "$EXPECT" "$name")
  [ "$got" = "$want" ] || { echo "FAIL: fold $name got=$got want=$want"; exit 1; }
  if [[ -n "$TINYVM" ]]; then
    perl -e 'alarm 30; exec @ARGV' "$TINYVM" module validate "$OUT/${name}.wasm" >/dev/null \
      || { echo "FAIL: tinyvm validate $name"; exit 1; }
  fi
  echo "OK $name fold=$got"
done
if [[ -n "$TINYVM" ]]; then
  echo "OK tinyvm validate fold corpus"
fi

echo "-- v18 str bounds (accept + reject; stage0/core recorded separately)"
# Literal 0–255 ASCII / no escapes ≠ concat results (two contracts).
# Expects: tests/ujs2wasm/str_bounds_expect.json (hand-authored; not from either compiler).
BOUNDS_EXPECT="$ROOT/tests/ujs2wasm/str_bounds_expect.json"

for name in str_empty str_lit7 str_lit8 str_lit32 str_lit255 str_cat_long; do
  want=$(node -e 'const e=JSON.parse(require("fs").readFileSync(process.argv[1],"utf8")); process.stdout.write(String(e.accept[process.argv[2]]))' "$BOUNDS_EXPECT" "$name")
  src="tests/ujs2wasm/corpus/${name}.ujs"
  if ! perl -e 'alarm 60; exec @ARGV' node ujs/compile.mjs "$src" -o "$OUT/${name}.wasm" >"$OUT/${name}_core.compile.log" 2>&1; then
    echo "FAIL: core compile $name (expected accept)"; cat "$OUT/${name}_core.compile.log"; exit 1
  fi
  if ! perl -e 'alarm 60; exec @ARGV' env UJS_REQUIRE_COMPILER_WASM=1 \
      node ujs/compile.mjs "$src" -o "$OUT/${name}_s0.wasm" >"$OUT/${name}_s0.compile.log" 2>&1; then
    echo "FAIL: stage0 compile $name (expected accept)"; cat "$OUT/${name}_s0.compile.log"; exit 1
  fi
  magic=$(head -c 4 "$OUT/${name}.wasm" | od -An -tx1 | tr -d ' \n')
  magic0=$(head -c 4 "$OUT/${name}_s0.wasm" | od -An -tx1 | tr -d ' \n')
  [ "$magic" = "0061736d" ] || { echo "FAIL: $name core not \\0asm"; exit 1; }
  [ "$magic0" = "0061736d" ] || { echo "FAIL: $name stage0 not \\0asm"; exit 1; }
  set +e
  got=$(perl -e 'alarm 30; exec @ARGV' node "$RUNNER" "$OUT/${name}.wasm" | node -e \
    'let d="";process.stdin.on("data",c=>d+=c);process.stdin.on("end",()=>{process.stdout.write(String(JSON.parse(d.trim())))})')
  rec=$?
  got0=$(perl -e 'alarm 30; exec @ARGV' node "$RUNNER" "$OUT/${name}_s0.wasm" | node -e \
    'let d="";process.stdin.on("data",c=>d+=c);process.stdin.on("end",()=>{process.stdout.write(String(JSON.parse(d.trim())))})')
  re0=$?
  set -e
  [ "$rec" -eq 0 ] || { echo "FAIL: $name core run trap/timeout ec=$rec"; exit 1; }
  [ "$re0" -eq 0 ] || { echo "FAIL: $name stage0 run trap/timeout ec=$re0"; exit 1; }
  [ "$got" = "$want" ] || { echo "FAIL: $name core exec got!=hand expect"; exit 1; }
  [ "$got0" = "$want" ] || { echo "FAIL: $name stage0 exec got!=hand expect"; exit 1; }
  echo "OK $name core_compile=ok stage0_compile=ok core_exec=ok stage0_exec=ok"
done
for neg in str_lit256 str_escape str_nonascii; do
  src="tests/ujs2wasm/neg/${neg}.ujs"
  rm -f "$OUT/${neg}.wasm" "$OUT/${neg}_s0.wasm"
  set +e
  perl -e 'alarm 30; exec @ARGV' node ujs/compile.mjs "$src" -o "$OUT/${neg}.wasm" >"$OUT/${neg}_core.err" 2>&1
  ec=$?
  set -e
  require_parse_reject "core $neg" "$ec" "$OUT/${neg}_core.err" "$OUT/${neg}.wasm"
  set +e
  perl -e 'alarm 30; exec @ARGV' env UJS_REQUIRE_COMPILER_WASM=1 \
    node ujs/compile.mjs "$src" -o "$OUT/${neg}_s0.wasm" >"$OUT/${neg}_s0.err" 2>&1
  ec0=$?
  set -e
  require_parse_reject "stage0 $neg" "$ec0" "$OUT/${neg}_s0.err" "$OUT/${neg}_s0.wasm"
  echo "OK reject $neg class=parse_reject core_ec=$ec stage0_ec=$ec0"
done

echo "-- reject classifier fault-injection (timeout/trap/stack must NOT pass)"
FI="$OUT/fault_inject"
mkdir -p "$FI"
printf '' >"$FI/empty.err"
printf 'compile: bad primary\n' >"$FI/parse.err"
printf 'compile: compiler.ujs core compile failed\n' >"$FI/parse_core.err"
printf 'WebAssembly.RuntimeError: unreachable\n' >"$FI/trap.err"
# ec=1 + RuntimeError + compile.mjs in stack (must NOT be parse_reject)
cat >"$FI/trap_stack.err" <<'STACK'
WebAssembly.RuntimeError: unreachable
    at Object.run (file:///tmp/fake/ujs/iterate/compile.mjs:114:11)
    at async main (file:///tmp/fake/ujs/iterate/compile.mjs:240:5)
STACK
printf '\x00asm\x01\x00\x00\x00' >"$FI/realish.wasm"
cls=$(classify_compile_outcome 1 "$FI/parse.err" "$FI/missing.wasm")
[ "$cls" = "parse_reject" ] || { echo "FAIL: inject parse_reject got $cls"; exit 1; }
cls=$(classify_compile_outcome 1 "$FI/parse_core.err" "$FI/missing.wasm")
[ "$cls" = "parse_reject" ] || { echo "FAIL: inject parse_reject core-diag got $cls"; exit 1; }
cls=$(classify_compile_outcome 142 "$FI/empty.err" "$FI/missing.wasm")
[ "$cls" = "timeout_or_signal" ] || { echo "FAIL: inject timeout got $cls"; exit 1; }
cls=$(classify_compile_outcome 139 "$FI/parse.err" "$FI/missing.wasm")
[ "$cls" = "timeout_or_signal" ] || { echo "FAIL: inject signal got $cls"; exit 1; }
cls=$(classify_compile_outcome 1 "$FI/trap.err" "$FI/missing.wasm")
[ "$cls" = "runtime_or_timeout_diag" ] || { echo "FAIL: inject RuntimeError got $cls"; exit 1; }
cls=$(classify_compile_outcome 1 "$FI/trap_stack.err" "$FI/missing.wasm")
[ "$cls" = "runtime_or_timeout_diag" ] || { echo "FAIL: inject RuntimeError+compile.mjs stack got $cls"; exit 1; }
cls=$(classify_compile_outcome 1 "$FI/parse.err" "$FI/realish.wasm")
[ "$cls" = "wrote_wasm" ] || { echo "FAIL: inject wrote_wasm got $cls"; exit 1; }
cls=$(classify_compile_outcome 1 "$FI/empty.err" "$FI/missing.wasm")
[ "$cls" = "empty_diag" ] || { echo "FAIL: inject empty_diag got $cls"; exit 1; }
cls=$(classify_compile_outcome 2 "$FI/parse.err" "$FI/missing.wasm")
[ "$cls" = "other_nonzero" ] || { echo "FAIL: inject other_nonzero got $cls"; exit 1; }
if ( require_parse_reject "inject-timeout" 142 "$FI/empty.err" "$FI/missing.wasm" ) >/dev/null 2>&1; then
  echo "FAIL: require_parse_reject accepted timeout"; exit 1
fi
if ( require_parse_reject "inject-trap-stack" 1 "$FI/trap_stack.err" "$FI/missing.wasm" ) >/dev/null 2>&1; then
  echo "FAIL: require_parse_reject accepted RuntimeError+compile.mjs stack"; exit 1
fi
echo "OK fault-inject: timeout/signal/trap(+compile.mjs stack)/wrote_wasm/empty/other ≠ parse_reject"

echo "-- product gate iso: REAL ujs2wasm_compiler.sh in temp tree (shared artifacts intact)"
ISO="$OUT/iso_real_gate"
# Minimal layout: copy real gate + shared prelude/classify; no compiler.wasm under ISO.
mkdir -p "$ISO/tests/ujs2wasm" "$ISO/ujs/core" "$ISO/ujs/iterate"
cp "$ROOT/tests/ujs2wasm_compiler.sh" "$ISO/tests/"
cp "$ROOT/tests/ujs2wasm/gate_prelude.sh" "$ISO/tests/ujs2wasm/"
cp "$ROOT/tests/ujs2wasm/gate_classify.sh" "$ISO/tests/ujs2wasm/"
# 1) missing compiler → FAIL (real script)
set +e
(cd "$ISO" && env -u UJS_REQUIRE_TINYVM -u UJS_ALLOW_MISSING_COMPILER_WASM \
  bash tests/ujs2wasm_compiler.sh) >"$ISO/missing.out" 2>&1
mec=$?
set -e
[ "$mec" -eq 1 ] || { echo "FAIL: real gate missing-compiler exit=$mec want=1"; cat "$ISO/missing.out"; exit 1; }
grep -q 'FAIL: no compiler.wasm' "$ISO/missing.out" || { echo "FAIL: real gate missing message"; cat "$ISO/missing.out"; exit 1; }
# 2) old ALLOW env must NOT green-skip (real script)
set +e
(cd "$ISO" && env -u UJS_REQUIRE_TINYVM UJS_ALLOW_MISSING_COMPILER_WASM=1 \
  bash tests/ujs2wasm_compiler.sh) >"$ISO/allow.out" 2>&1
aec=$?
set -e
[ "$aec" -eq 1 ] || { echo "FAIL: ALLOW env still skip-greens exit=$aec"; cat "$ISO/allow.out"; exit 1; }
grep -q 'FAIL: no compiler.wasm' "$ISO/allow.out" || { echo "FAIL: ALLOW path message"; cat "$ISO/allow.out"; exit 1; }
# 3) REQUIRE_TINYVM: real gate with compiler present, tinyvm unreachable
cp "$ROOT/ujs/iterate/compiler.wasm" "$ISO/ujs/core/compiler.wasm"
set +e
(cd "$ISO" && env PATH="/usr/bin:/bin" HOME="$ISO/empty_home" UJS_REQUIRE_TINYVM=1 \
  bash tests/ujs2wasm_compiler.sh) >"$ISO/tv.out" 2>&1
tec=$?
set -e
[ "$tec" -eq 1 ] || { echo "FAIL: real gate REQUIRE_TINYVM exit=$tec want=1"; cat "$ISO/tv.out"; exit 1; }
grep -q 'FAIL: UJS_REQUIRE_TINYVM=1 but tinyvm not found' "$ISO/tv.out" \
  || { echo "FAIL: REQUIRE_TINYVM message"; cat "$ISO/tv.out"; exit 1; }
[ -f "$ROOT/ujs/iterate/compiler.wasm" ] || { echo "FAIL: shared compiler.wasm gone"; exit 1; }
[ -f "$ROOT/ujs/iterate/compiler_core.wasm" ] || { echo "FAIL: shared compiler_core.wasm gone"; exit 1; }
echo "OK iso real-gate: missing FAIL; ALLOW ignored; REQUIRE_TINYVM FAIL; shared intact"

echo "-- default compile.mjs → compiler_core (product path)"
log=$(perl -e 'alarm 60; exec @ARGV' node ujs/compile.mjs \
  tests/ujs2wasm/corpus/arith.ujs -o "$OUT/arith_core.wasm")
echo "$log" | grep -q '"bridge":"compiler_core.wasm"' \
  || { echo "FAIL: default bridge should be compiler_core.wasm: $log"; exit 1; }
got=$(perl -e 'alarm 30; exec @ARGV' node "$RUNNER" "$OUT/arith_core.wasm")
[ "$got" = "7" ] || { echo "FAIL: default-core arith fold=$got"; exit 1; }
echo "OK default core arith fold=7"

echo "-- setidx_globals via compile.mjs (default core) + run_step (no python3)"
# Exercise compiler_core → rebuild-main splice (stub NG patch), not stage0 alone.
comp_json=$(perl -e 'alarm 60; exec @ARGV' env -u UJS_REQUIRE_COMPILER_WASM -u UJS_COMPILER \
  node ujs/compile.mjs \
  tests/ujs2wasm/step_corpus/setidx_globals.ujs -o "$OUT/setidx_globals.wasm")
echo "$comp_json" | grep -q 'compiler_core.wasm' || {
  echo "FAIL setidx_globals must use compiler_core.wasm (got: $comp_json)"
  exit 1
}
node -e '
const e=JSON.parse(require("fs").readFileSync(process.argv[1],"utf8"));
require("fs").writeFileSync(process.argv[2], JSON.stringify(e.setidx_globals.globals));
' "$STEP_EXPECT" "$OUT/setidx_globals.g.json"
got=$(perl -e 'alarm 30; exec @ARGV' node "$STEP_RUNNER" \
  "$OUT/setidx_globals.wasm" "$OUT/setidx_globals.meta.json" "$OUT/setidx_globals.g.json" 1)
want=$(node -e 'const e=JSON.parse(require("fs").readFileSync(process.argv[1],"utf8")); process.stdout.write(JSON.stringify(e.setidx_globals.want))' "$STEP_EXPECT")
node -e '
const got=JSON.parse(process.argv[1]), want=JSON.parse(process.argv[2]);
function close(a,b){
  if(typeof a==="number"&&typeof b==="number") return Math.abs(a-b)<1e-9;
  if(a&&b&&typeof a==="object") return Object.keys(want).every(k=>close(got[k],want[k]));
  return a===b;
}
if(!close(got,want)){ console.error("FAIL setidx_globals",got,want); process.exit(1); }
console.log("OK setidx_globals", JSON.stringify(got));
' "$got" "$want"

echo "-- sim.ujs compile + instantiate (no python3)"
perl -e 'alarm 60; exec @ARGV' node ujs/compile.mjs ujs/web/game/sim.ujs -o "$OUT/sim.wasm" >/dev/null
perl -e 'alarm 30; exec @ARGV' node --input-type=module -e '
import fs from "fs";
await WebAssembly.instantiate(fs.readFileSync(process.argv[1]));
console.log("OK sim.ujs instantiate", fs.statSync(process.argv[1]).size, "B");
' "$OUT/sim.wasm"

echo "-- ship builders emit via compiler_core.wasm (no python3)"
for f in ujs/uxe/ship/build-asteroid-pages.mjs ujs/uxe/ship/build-drone-pages.mjs; do
  if grep -E 'spawnSync\([\"'\'']python3|emit_wasm' "$f" >/dev/null; then
    echo "FAIL: $f still emits via python3/emit_wasm"
    exit 1
  fi
done
# default compile.mjs (no UJS_COMPILER) under PATH trap → core
TRAPBIN2="$OUT/bin"
got=$(perl -e 'alarm 60; exec @ARGV' env PATH="$TRAPBIN2:$PATH" \
  node ujs/compile.mjs ujs/web/game/sim.ujs -o "$OUT/ship_sim.wasm")
echo "$got" | grep -q '"bridge":"compiler_core.wasm"' || { echo "FAIL ship sim emit: $got"; exit 1; }
got=$(perl -e 'alarm 60; exec @ARGV' env PATH="$TRAPBIN2:$PATH" \
  node ujs/compile.mjs ujs/web/game/drone.ujs -o "$OUT/ship_drone.wasm")
echo "$got" | grep -q '"bridge":"compiler_core.wasm"' || { echo "FAIL ship drone emit: $got"; exit 1; }
echo "OK ship emit compiler_core.wasm default (PATH without python3)"

echo "-- ship builders: no A-core / web-build"
for f in ujs/uxe/ship/build-asteroid-pages.mjs ujs/uxe/ship/build-drone-pages.mjs \
         ujs/scripts/ship-engine.sh ujs/scripts/ship-pages.sh; do
  if grep -E 'ujs_full\.wasm|python3 -m ujs web-build' "$f" >/dev/null; then
    echo "FAIL: $f still depends on ujs_full / web-build"
    exit 1
  fi
done
echo "OK ship path free of web-build / ujs_full"

echo "-- M3 seed: compiler.ujs → core → splice return N"
[ -f ujs/core/compiler_rt_stub.wasm ] || { echo "FAIL: missing ujs/core/compiler_rt_stub.wasm"; exit 1; }
[ -f ujs/core/compiler.ujs ] || { echo "FAIL: missing ujs/core/compiler.ujs"; exit 1; }
perl -e 'alarm 60; exec @ARGV' env PATH="$TRAPBIN2:$PATH" UJS_REQUIRE_COMPILER_WASM=1 \
  node ujs/compile.mjs ujs/core/compiler.ujs -o "$OUT/compiler_core.wasm" >/dev/null
printf 'return 42;\n' > "$OUT/ret42.ujs"
perl -e 'alarm 30; exec @ARGV' node ujs/scripts/run-compiler-core.mjs \
  "$OUT/compiler_core.wasm" "$OUT/ret42.ujs" -o "$OUT/ret42.wasm" >/dev/null
perl -e 'alarm 30; exec @ARGV' node --input-type=module -e '
import fs from "fs";
const { instance } = await WebAssembly.instantiate(fs.readFileSync(process.argv[1]));
const ex = instance.exports;
ex.host_reset();
const r = ex.run_step();
if (ex.tag_of_export(r) !== 2 || Number(ex.i64_of_export(r)) !== 42) {
  console.error("FAIL M3 seed ret42", r);
  process.exit(1);
}
console.log("OK M3 seed return 42 via compiler.ujs");
' "$OUT/ret42.wasm"
printf '// c\n  return 7;\n' > "$OUT/ret7.ujs"
perl -e 'alarm 30; exec @ARGV' node ujs/scripts/run-compiler-core.mjs \
  "$OUT/compiler_core.wasm" "$OUT/ret7.ujs" -o "$OUT/ret7.wasm" >/dev/null
perl -e 'alarm 30; exec @ARGV' node --input-type=module -e '
import fs from "fs";
const { instance } = await WebAssembly.instantiate(fs.readFileSync(process.argv[1]));
const ex = instance.exports;
ex.host_reset();
const r = ex.run_step();
if (ex.tag_of_export(r) !== 2 || Number(ex.i64_of_export(r)) !== 7) {
  console.error("FAIL M3 seed ret7", r);
  process.exit(1);
}
console.log("OK M3 seed comment/ws return 7");
' "$OUT/ret7.wasm"
printf 'let x = 42;\nreturn x;\n' > "$OUT/letx.ujs"
perl -e 'alarm 30; exec @ARGV' node ujs/scripts/run-compiler-core.mjs \
  "$OUT/compiler_core.wasm" "$OUT/letx.ujs" -o "$OUT/letx.wasm" >/dev/null
perl -e 'alarm 30; exec @ARGV' node --input-type=module -e '
import fs from "fs";
const { instance } = await WebAssembly.instantiate(fs.readFileSync(process.argv[1]));
const ex = instance.exports;
ex.host_reset();
const r = ex.run_step();
if (ex.tag_of_export(r) !== 2 || Number(ex.i64_of_export(r)) !== 42) {
  console.error("FAIL M3 letx", r);
  process.exit(1);
}
console.log("OK M3 let x=42; return x");
' "$OUT/letx.wasm"
printf 'let a = 3;\nlet b = 4;\nreturn a * b + 1;\n' > "$OUT/arith.ujs"
perl -e 'alarm 30; exec @ARGV' node ujs/scripts/run-compiler-core.mjs \
  "$OUT/compiler_core.wasm" "$OUT/arith.ujs" -o "$OUT/arith.wasm" >/dev/null
# body ≡ stage0
perl -e 'alarm 60; exec @ARGV' env UJS_REQUIRE_COMPILER_WASM=1 \
  node ujs/compile.mjs "$OUT/arith.ujs" -o "$OUT/arith_s0.wasm" >/dev/null
perl -e 'alarm 30; exec @ARGV' node --input-type=module -e '
import fs from "fs";
function mainBody(path) {
  const u = fs.readFileSync(path);
  function readUleb(b,i){let v=0,s=0;for(;;){const x=b[i++];v|=(x&0x7f)<<s;if(!(x&0x80))return[v,i];s+=7;}}
  let i=8;
  while(i<u.length){const sid=u[i++];const[ln,i2]=readUleb(u,i);i=i2;
    if(sid===10){const code=u.subarray(i,i+ln);let[nf,o]=readUleb(code,0);
      for(let fi=0;fi<nf;fi++){const[sz,o2]=readUleb(code,o);const body=[...code.subarray(o2,o2+sz)];o=o2+sz;if(fi===1)return body;}}
    i+=ln;}
}
const a=mainBody(process.argv[1]), b=mainBody(process.argv[2]);
if(!a||!b||a.length!==b.length||!a.every((v,i)=>v===b[i])){
  console.error("FAIL M3 body≢stage0", a, b); process.exit(1);
}
const {instance}=await WebAssembly.instantiate(fs.readFileSync(process.argv[2]));
instance.exports.host_reset();
const r=instance.exports.run_step();
if(Number(instance.exports.i64_of_export(r))!==13){console.error("FAIL arith run");process.exit(1);}
console.log("OK M3 arith body≡stage0 run=13");
' "$OUT/arith_s0.wasm" "$OUT/arith.wasm"

printf 'let i = 0;\nwhile (i < 3) {\n  i = i + 1;\n}\nreturn i;\n' > "$OUT/while.ujs"
perl -e 'alarm 30; exec @ARGV' node ujs/scripts/run-compiler-core.mjs \
  "$OUT/compiler_core.wasm" "$OUT/while.ujs" -o "$OUT/while.wasm" >/dev/null
perl -e 'alarm 60; exec @ARGV' env UJS_REQUIRE_COMPILER_WASM=1 \
  node ujs/compile.mjs "$OUT/while.ujs" -o "$OUT/while_s0.wasm" >/dev/null
perl -e 'alarm 30; exec @ARGV' node --input-type=module -e '
import fs from "fs";
function mainBody(path) {
  const u = fs.readFileSync(path);
  function readUleb(b,i){let v=0,s=0;for(;;){const x=b[i++];v|=(x&0x7f)<<s;if(!(x&0x80))return[v,i];s+=7;}}
  let i=8;
  while(i<u.length){const sid=u[i++];const[ln,i2]=readUleb(u,i);i=i2;
    if(sid===10){const code=u.subarray(i,i+ln);let[nf,o]=readUleb(code,0);
      for(let fi=0;fi<nf;fi++){const[sz,o2]=readUleb(code,o);const body=[...code.subarray(o2,o2+sz)];o=o2+sz;if(fi===1)return body;}}
    i+=ln;}
}
const a=mainBody(process.argv[1]), b=mainBody(process.argv[2]);
if(!a||!b||a.length!==b.length||!a.every((v,i)=>v===b[i])){
  console.error("FAIL M3 while≢stage0", a, b); process.exit(1);
}
const {instance}=await WebAssembly.instantiate(fs.readFileSync(process.argv[2]));
instance.exports.host_reset();
const r=instance.exports.run_step();
if(Number(instance.exports.i64_of_export(r))!==3){console.error("FAIL while run");process.exit(1);}
console.log("OK M3 while body≡stage0 run=3");
' "$OUT/while_s0.wasm" "$OUT/while.wasm"
printf 'let x = 1;\nif (x == 1) {\n  x = 2;\n} else {\n  x = 3;\n}\nreturn x;\n' > "$OUT/iff.ujs"
perl -e 'alarm 30; exec @ARGV' node ujs/scripts/run-compiler-core.mjs \
  "$OUT/compiler_core.wasm" "$OUT/iff.ujs" -o "$OUT/iff.wasm" >/dev/null
perl -e 'alarm 60; exec @ARGV' env UJS_REQUIRE_COMPILER_WASM=1 \
  node ujs/compile.mjs "$OUT/iff.ujs" -o "$OUT/iff_s0.wasm" >/dev/null
perl -e 'alarm 30; exec @ARGV' node --input-type=module -e '
import fs from "fs";
function mainBody(path) {
  const u = fs.readFileSync(path);
  function readUleb(b,i){let v=0,s=0;for(;;){const x=b[i++];v|=(x&0x7f)<<s;if(!(x&0x80))return[v,i];s+=7;}}
  let i=8;
  while(i<u.length){const sid=u[i++];const[ln,i2]=readUleb(u,i);i=i2;
    if(sid===10){const code=u.subarray(i,i+ln);let[nf,o]=readUleb(code,0);
      for(let fi=0;fi<nf;fi++){const[sz,o2]=readUleb(code,o);const body=[...code.subarray(o2,o2+sz)];o=o2+sz;if(fi===1)return body;}}
    i+=ln;}
}
const a=mainBody(process.argv[1]), b=mainBody(process.argv[2]);
if(!a||!b||a.length!==b.length||!a.every((v,i)=>v===b[i])){
  console.error("FAIL M3 if≢stage0", a, b); process.exit(1);
}
const {instance}=await WebAssembly.instantiate(fs.readFileSync(process.argv[2]));
instance.exports.host_reset();
const r=instance.exports.run_step();
if(Number(instance.exports.i64_of_export(r))!==2){console.error("FAIL if run");process.exit(1);}
console.log("OK M3 if/else body≡stage0 run=2");
' "$OUT/iff_s0.wasm" "$OUT/iff.wasm"

echo "-- M3 v5: list/len/index/SRC body≡stage0"
printf 'let xs = list(2);\nxs[0] = 5;\nxs[1] = xs[0] + 1;\nreturn xs[1];\n' > "$OUT/listx.ujs"
perl -e 'alarm 30; exec @ARGV' node ujs/scripts/run-compiler-core.mjs \
  "$OUT/compiler_core.wasm" "$OUT/listx.ujs" -o "$OUT/listx.wasm" >/dev/null
perl -e 'alarm 60; exec @ARGV' env UJS_REQUIRE_COMPILER_WASM=1 \
  node ujs/compile.mjs "$OUT/listx.ujs" -o "$OUT/listx_s0.wasm" >/dev/null
perl -e 'alarm 30; exec @ARGV' node --input-type=module -e '
import fs from "fs";
function mainBody(path) {
  const u = fs.readFileSync(path);
  function readUleb(b,i){let v=0,s=0;for(;;){const x=b[i++];v|=(x&0x7f)<<s;if(!(x&0x80))return[v,i];s+=7;}}
  let i=8;
  while(i<u.length){const sid=u[i++];const[ln,i2]=readUleb(u,i);i=i2;
    if(sid===10){const code=u.subarray(i,i+ln);let[nf,o]=readUleb(code,0);
      for(let fi=0;fi<nf;fi++){const[sz,o2]=readUleb(code,o);const body=[...code.subarray(o2,o2+sz)];o=o2+sz;if(fi===1)return body;}}
    i+=ln;}
}
const a=mainBody(process.argv[1]), b=mainBody(process.argv[2]);
if(!a||!b||a.length!==b.length||!a.every((v,i)=>v===b[i])){
  console.error("FAIL M3 list≢stage0", a, b); process.exit(1);
}
const {instance}=await WebAssembly.instantiate(fs.readFileSync(process.argv[2]));
instance.exports.host_reset();
const r=instance.exports.run_step();
if(Number(instance.exports.i64_of_export(r))!==6){console.error("FAIL list run");process.exit(1);}
console.log("OK M3 list/setidx body≡stage0 run=6");
' "$OUT/listx_s0.wasm" "$OUT/listx.wasm"

echo "-- M3 v6: SRC cmp + if-no-else + self-host stage2≡stage1"
printf 'let x = 0;\nif (SRC[0] == 47) { x = 1; }\nreturn x;\n' > "$OUT/srccmp.ujs"
perl -e 'alarm 30; exec @ARGV' node ujs/scripts/run-compiler-core.mjs \
  "$OUT/compiler_core.wasm" "$OUT/srccmp.ujs" -o "$OUT/srccmp.wasm" >/dev/null
perl -e 'alarm 60; exec @ARGV' env UJS_REQUIRE_COMPILER_WASM=1 \
  node ujs/compile.mjs "$OUT/srccmp.ujs" -o "$OUT/srccmp_s0.wasm" >/dev/null
perl -e 'alarm 30; exec @ARGV' node --input-type=module -e '
import fs from "fs";
function mainBody(path) {
  const u = fs.readFileSync(path);
  function readUleb(b,i){let v=0,s=0;for(;;){const x=b[i++];v|=(x&0x7f)<<s;if(!(x&0x80))return[v,i];s+=7;}}
  let i=8;
  while(i<u.length){const sid=u[i++];const[ln,i2]=readUleb(u,i);i=i2;
    if(sid===10){const code=u.subarray(i,i+ln);let[nf,o]=readUleb(code,0);
      for(let fi=0;fi<nf;fi++){const[sz,o2]=readUleb(code,o);const body=[...code.subarray(o2,o2+sz)];o=o2+sz;if(fi===1)return body;}}
    i+=ln;}
}
const a=mainBody(process.argv[1]), b=mainBody(process.argv[2]);
if(!a||!b||a.length!==b.length||!a.every((v,i)=>v===b[i])){
  console.error("FAIL M3 srccmp≢stage0", a, b); process.exit(1);
}
console.log("OK M3 SRC[i]==N body≡stage0");
' "$OUT/srccmp_s0.wasm" "$OUT/srccmp.wasm"

echo "-- M3 v7: === + f64 lit/arith/mix body≡stage0"
printf 'let x = 0;\nif (x === 0) { x = 1; }\nreturn x;\n' > "$OUT/eq3.ujs"
perl -e 'alarm 30; exec @ARGV' node ujs/scripts/run-compiler-core.mjs \
  "$OUT/compiler_core.wasm" "$OUT/eq3.ujs" -o "$OUT/eq3.wasm" >/dev/null
perl -e 'alarm 60; exec @ARGV' env UJS_REQUIRE_COMPILER_WASM=1 \
  node ujs/compile.mjs "$OUT/eq3.ujs" -o "$OUT/eq3_s0.wasm" >/dev/null
printf 'let x = 1.5;\nx = x * 2.0 + 0.5;\nreturn x;\n' > "$OUT/f64a.ujs"
perl -e 'alarm 30; exec @ARGV' node ujs/scripts/run-compiler-core.mjs \
  "$OUT/compiler_core.wasm" "$OUT/f64a.ujs" -o "$OUT/f64a.wasm" >/dev/null
perl -e 'alarm 60; exec @ARGV' env UJS_REQUIRE_COMPILER_WASM=1 \
  node ujs/compile.mjs "$OUT/f64a.ujs" -o "$OUT/f64a_s0.wasm" >/dev/null
printf 'let i = 3;\nlet x = 0.0;\nx = i * 1.5;\nreturn x;\n' > "$OUT/f64m.ujs"
perl -e 'alarm 30; exec @ARGV' node ujs/scripts/run-compiler-core.mjs \
  "$OUT/compiler_core.wasm" "$OUT/f64m.ujs" -o "$OUT/f64m.wasm" >/dev/null
perl -e 'alarm 60; exec @ARGV' env UJS_REQUIRE_COMPILER_WASM=1 \
  node ujs/compile.mjs "$OUT/f64m.ujs" -o "$OUT/f64m_s0.wasm" >/dev/null
perl -e 'alarm 30; exec @ARGV' node --input-type=module -e '
import fs from "fs";
function mainBody(path) {
  const u = fs.readFileSync(path);
  function readUleb(b,i){let v=0,s=0;for(;;){const x=b[i++];v|=(x&0x7f)<<s;if(!(x&0x80))return[v,i];s+=7;}}
  let i=8;
  while(i<u.length){const sid=u[i++];const[ln,i2]=readUleb(u,i);i=i2;
    if(sid===10){const code=u.subarray(i,i+ln);let[nf,o]=readUleb(code,0);
      for(let fi=0;fi<nf;fi++){const[sz,o2]=readUleb(code,o);const body=[...code.subarray(o2,o2+sz)];o=o2+sz;if(fi===1)return body;}}
    i+=ln;}
}
function eq(a,b,tag){
  if(!a||!b||a.length!==b.length||!a.every((v,i)=>v===b[i])){
    console.error("FAIL M3 "+tag, a?.length, b?.length); process.exit(1);
  }
}
eq(mainBody(process.argv[1]), mainBody(process.argv[2]), "eq3≢stage0");
eq(mainBody(process.argv[3]), mainBody(process.argv[4]), "f64arith≢stage0");
eq(mainBody(process.argv[5]), mainBody(process.argv[6]), "f64mix≢stage0");
console.log("OK M3 === + f64 lit/arith/mix body≡stage0");
' "$OUT/eq3_s0.wasm" "$OUT/eq3.wasm" "$OUT/f64a_s0.wasm" "$OUT/f64a.wasm" "$OUT/f64m_s0.wasm" "$OUT/f64m.wasm"

# stage1 = stage0(compiler.ujs); stage2 = stage1(compiler.ujs); bodies must match
perl -e 'alarm 60; exec @ARGV' env UJS_REQUIRE_COMPILER_WASM=1 \
  node ujs/compile.mjs ujs/core/compiler.ujs -o "$OUT/stage1.wasm" >/dev/null
perl -e 'alarm 60; exec @ARGV' node ujs/scripts/run-compiler-core.mjs \
  "$OUT/stage1.wasm" ujs/core/compiler.ujs -o "$OUT/stage2.wasm" >/dev/null
perl -e 'alarm 30; exec @ARGV' node --input-type=module -e '
import fs from "fs";
function mainBody(path) {
  const u = fs.readFileSync(path);
  function readUleb(b,i){let v=0,s=0;for(;;){const x=b[i++];v|=(x&0x7f)<<s;if(!(x&0x80))return[v,i];s+=7;}}
  let i=8;
  while(i<u.length){const sid=u[i++];const[ln,i2]=readUleb(u,i);i=i2;
    if(sid===10){const code=u.subarray(i,i+ln);let[nf,o]=readUleb(code,0);
      for(let fi=0;fi<nf;fi++){const[sz,o2]=readUleb(code,o);const body=[...code.subarray(o2,o2+sz)];o=o2+sz;if(fi===1)return body;}}
    i+=ln;}
}
const a=mainBody(process.argv[1]), b=mainBody(process.argv[2]);
if(!a||!b||a.length!==b.length||!a.every((v,i)=>v===b[i])){
  console.error("FAIL M3 stage2≢stage1", a?.length, b?.length); process.exit(1);
}
console.log("OK M3 stage2≡stage1 mainBody", a.length);
' "$OUT/stage1.wasm" "$OUT/stage2.wasm"

echo "-- M3 v10: sim.ujs body≡stage0"
perl -e 'alarm 60; exec @ARGV' env UJS_REQUIRE_COMPILER_WASM=1 \
  node ujs/compile.mjs ujs/web/game/sim.ujs -o "$OUT/sim_s0.wasm" >/dev/null
perl -e 'alarm 60; exec @ARGV' node ujs/scripts/run-compiler-core.mjs \
  "$OUT/stage1.wasm" ujs/web/game/sim.ujs -o "$OUT/sim_s1.wasm" >/dev/null
perl -e 'alarm 30; exec @ARGV' node --input-type=module -e '
import fs from "fs";
function mainBody(path) {
  const u = fs.readFileSync(path);
  function readUleb(b,i){let v=0,s=0;for(;;){const x=b[i++];v|=(x&0x7f)<<s;if(!(x&0x80))return[v,i];s+=7;}}
  let i=8;
  while(i<u.length){const sid=u[i++];const[ln,i2]=readUleb(u,i);i=i2;
    if(sid===10){const code=u.subarray(i,i+ln);let[nf,o]=readUleb(code,0);
      for(let fi=0;fi<nf;fi++){const[sz,o2]=readUleb(code,o);const body=[...code.subarray(o2,o2+sz)];o=o2+sz;if(fi===1)return body;}}
    i+=ln;}
}
const a=mainBody(process.argv[1]), b=mainBody(process.argv[2]);
if(!a||!b||a.length!==b.length||!a.every((v,i)=>v===b[i])){
  console.error("FAIL M3 sim≢stage0", a?.length, b?.length); process.exit(1);
}
console.log("OK M3 sim.ujs body≡stage0", a.length);
' "$OUT/sim_s0.wasm" "$OUT/sim_s1.wasm"

echo "-- M3 v11: drone.ujs body≡stage0"
perl -e 'alarm 60; exec @ARGV' env UJS_REQUIRE_COMPILER_WASM=1 \
  node ujs/compile.mjs ujs/web/game/drone.ujs -o "$OUT/drone_s0.wasm" >/dev/null
perl -e 'alarm 60; exec @ARGV' node ujs/scripts/run-compiler-core.mjs \
  "$OUT/stage1.wasm" ujs/web/game/drone.ujs -o "$OUT/drone_s1.wasm" >/dev/null
perl -e 'alarm 30; exec @ARGV' node --input-type=module -e '
import fs from "fs";
function mainBody(path) {
  const u = fs.readFileSync(path);
  function readUleb(b,i){let v=0,s=0;for(;;){const x=b[i++];v|=(x&0x7f)<<s;if(!(x&0x80))return[v,i];s+=7;}}
  let i=8;
  while(i<u.length){const sid=u[i++];const[ln,i2]=readUleb(u,i);i=i2;
    if(sid===10){const code=u.subarray(i,i+ln);let[nf,o]=readUleb(code,0);
      for(let fi=0;fi<nf;fi++){const[sz,o2]=readUleb(code,o);const body=[...code.subarray(o2,o2+sz)];o=o2+sz;if(fi===1)return body;}}
    i+=ln;}
}
const a=mainBody(process.argv[1]), b=mainBody(process.argv[2]);
if(!a||!b||a.length!==b.length||!a.every((v,i)=>v===b[i])){
  console.error("FAIL M3 drone≢stage0", a?.length, b?.length); process.exit(1);
}
console.log("OK M3 drone.ujs body≡stage0", a.length);
' "$OUT/drone_s0.wasm" "$OUT/drone_s1.wasm"

if [[ -n "$TINYVM" ]]; then
  echo "-- tinyvm validate ship twins (sim/drone)"
  for w in "$OUT/sim_s0.wasm" "$OUT/sim_s1.wasm" "$OUT/drone_s0.wasm" "$OUT/drone_s1.wasm"; do
    perl -e 'alarm 30; exec @ARGV' "$TINYVM" module validate "$w" >/dev/null \
      || { echo "FAIL: tinyvm validate $(basename "$w")"; exit 1; }
  done
  echo "OK tinyvm validate sim+drone"
fi

echo "ujs2wasm_compiler OK (M2 + M3 v18 · fold corpus via compiler_core · stage2≡stage1 · tinyvm validate optional)"

