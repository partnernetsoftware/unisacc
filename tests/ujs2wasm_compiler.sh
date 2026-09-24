#!/usr/bin/env bash
# M2 gate: compiler.wasm callable from Node — no python3 on the product path.
#
# Subset: arith · fact · branch · f64 · list · setidx · dict · globals_fold · list_f64
# + setidx_globals (host inject + run_step) · sim.ujs compile · ship builders no python3 emit
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

echo "== ujs2wasm_compiler (M2 subset) =="

COMPILER_WASM=""
for cand in ujs/core/compiler.wasm ujs/compiler.wasm ujs/uxe/ship/compiler.wasm; do
  if [ -f "$cand" ]; then COMPILER_WASM="$cand"; break; fi
done

if [ -z "$COMPILER_WASM" ]; then
  echo "skip: no compiler.wasm yet (M2 product artifact missing)"
  echo "build: ./ujs/scripts/build-compiler-wasm.sh"
  echo "scaffold: ujs/compile.mjs still shells python3 when artifact absent"
  exit 0
fi

command -v node >/dev/null || { echo "needs node"; exit 1; }
OUT="${TMPDIR:-/tmp}/ujs2wasm-compiler.$$"
mkdir -p "$OUT"
trap 'rm -rf "$OUT"' EXIT

echo "artifact: $COMPILER_WASM ($(wc -c < "$COMPILER_WASM") bytes)"

TRAPBIN="$OUT/bin"
mkdir -p "$TRAPBIN"
cat > "$TRAPBIN/python3" <<'STUB'
#!/bin/sh
echo "ujs2wasm_compiler: python3 must not run on product path" >&2
exit 127
STUB
chmod +x "$TRAPBIN/python3"
export PATH="$TRAPBIN:$PATH"

EXPECT="$ROOT/tests/ujs2wasm/expect.json"
RUNNER="$ROOT/tests/ujs2wasm/run_wasm.mjs"
STEP_RUNNER="$ROOT/tests/ujs2wasm/run_step.mjs"
STEP_EXPECT="$ROOT/tests/ujs2wasm/step_expect.json"

for name in arith fact branch f64_arith list setidx dict globals_fold list_f64; do
  src="tests/ujs2wasm/corpus/${name}.ujs"
  echo "-- compile $name via compile.mjs (no python3)"
  log=$(perl -e 'alarm 60; exec @ARGV' node ujs/compile.mjs "$src" -o "$OUT/${name}.wasm")
  echo "$log"
  [ -f "$OUT/${name}.wasm" ] || { echo "FAIL: no wasm for $name"; exit 1; }
  magic=$(head -c 4 "$OUT/${name}.wasm" | od -An -tx1 | tr -d ' \n')
  [ "$magic" = "0061736d" ] || { echo "FAIL: $name not \\0asm ($magic)"; exit 1; }
  echo "$log" | grep -q '"bridge":"compiler.wasm"' \
    || { echo "FAIL: expected bridge compiler.wasm, got: $log"; exit 1; }
  got=$(perl -e 'alarm 30; exec @ARGV' node "$RUNNER" "$OUT/${name}.wasm")
  want=$(node -e "const e=JSON.parse(require('fs').readFileSync(process.argv[1],'utf8')); process.stdout.write(String(e[process.argv[2]]))" "$EXPECT" "$name")
  [ "$got" = "$want" ] || { echo "FAIL: fold $name got=$got want=$want"; exit 1; }
  echo "OK $name fold=$got"
done

if perl -e 'alarm 30; exec @ARGV' node ujs/compile.mjs tests/ujs2wasm/corpus/str.ujs \
    -o "$OUT/str.wasm" 2>"$OUT/str.err"; then
  echo "FAIL: str.ujs should be out of subset"
  exit 1
fi
echo "OK subset rejects str.ujs"

echo "-- setidx_globals via compile.mjs + run_step (no python3)"
perl -e 'alarm 60; exec @ARGV' node ujs/compile.mjs \
  tests/ujs2wasm/step_corpus/setidx_globals.ujs -o "$OUT/setidx_globals.wasm" >/dev/null
perl -e 'alarm 30; exec @ARGV' node --input-type=module -e '
import fs from "fs";
const {instance}=await WebAssembly.instantiate(fs.readFileSync(process.argv[1]));
const text=fs.readFileSync(process.argv[2],"utf8");
const b=new TextEncoder().encode(text);
const p=instance.exports.alloc(b.length+1);
new Uint8Array(instance.exports.memory.buffer,p,b.length).set(b);
if(instance.exports.compile(p,b.length)!==0) throw new Error("compile");
const meta=JSON.parse(new TextDecoder().decode(new Uint8Array(
  instance.exports.memory.buffer, instance.exports.meta_ptr(), instance.exports.meta_len())));
fs.writeFileSync(process.argv[3], JSON.stringify(meta));
' "$COMPILER_WASM" tests/ujs2wasm/step_corpus/setidx_globals.ujs "$OUT/setidx_globals.meta.json"
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

echo "-- ship builders emit via compiler.wasm (no python3)"
for f in ujs/uxe/ship/build-asteroid-pages.mjs ujs/uxe/ship/build-drone-pages.mjs; do
  if grep -E 'spawnSync\([\"'\'']python3|emit_wasm' "$f" >/dev/null; then
    echo "FAIL: $f still emits via python3/emit_wasm"
    exit 1
  fi
done
# compile.mjs under PATH trap must succeed for both game cores
TRAPBIN2="$OUT/bin"
got=$(perl -e 'alarm 60; exec @ARGV' env PATH="$TRAPBIN2:$PATH" UJS_REQUIRE_COMPILER_WASM=1 \
  node ujs/compile.mjs ujs/web/game/sim.ujs -o "$OUT/ship_sim.wasm")
echo "$got" | grep -q '"bridge":"compiler.wasm"' || { echo "FAIL ship sim emit: $got"; exit 1; }
got=$(perl -e 'alarm 60; exec @ARGV' env PATH="$TRAPBIN2:$PATH" UJS_REQUIRE_COMPILER_WASM=1 \
  node ujs/compile.mjs ujs/web/game/drone.ujs -o "$OUT/ship_drone.wasm")
echo "$got" | grep -q '"bridge":"compiler.wasm"' || { echo "FAIL ship drone emit: $got"; exit 1; }
echo "OK ship emit compiler.wasm (PATH without python3)"

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
perl -e 'alarm 90; exec @ARGV' node ujs/scripts/run-compiler-core.mjs \
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

echo "ujs2wasm_compiler OK (M2 + M3 v8 dict · len u32 · stage2≡stage1)"
