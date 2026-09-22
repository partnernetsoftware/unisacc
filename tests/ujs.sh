#!/bin/bash
# ujs acceptance — acc + api probes + fold + web wasm
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

echo "== ujs acc =="
python3 -m ujs acc

echo "== wasm_run probes =="
python3 - <<'PY'
from ujs import wasm_run
from ujs.oracle import Oracle

o = Oracle(drive="gold")

def expect(src, val, G=None, L=None):
    r = wasm_run(src, G or {}, L or {}, oracle=o)
    assert r.ok, (src, r.err)
    from ujs import value as V
    got = V.to_py(r.value)
    assert got == val, (src, got, val)

expect("return 1 + 2;", 3)
expect("let x = 10; return x * 3;", 30)
expect('return "a" + "b";', "ab")
expect("let xs = [1,2,3]; return xs[1];", 2)
expect('let d = {a: 1}; return d.a;', 1)
expect("""
let s = 0;
let i = 0;
while (i < 5) { s = s + i; i = i + 1; }
return s;
""", 10)
expect("""
let x = 2;
switch (x) {
  case 1: return 10;
  case 2: return 20;
  default: return 0;
}
""", 20)
expect("""
function add(a, b) { return a + b; }
return add(3, 4);
""", 7)
expect("""
function sum(a, ...rest) {
  let s = a;
  let i = 0;
  while (i < len(rest)) { s = s + rest[i]; i = i + 1; }
  return s;
}
return sum(1, 2, 3);
""", 6)

from ujs import api
from ujs import value as V
src = "let n = 5; let f = 1; while (n > 0) { f = f * n; n = n - 1; } return f;"
r1 = api.run(src, backend="jtape", oracle=o)
r2 = api.run(src, backend="wasm", oracle=o)
assert r1.ok and r2.ok
assert V.to_py(r1.value) == V.to_py(r2.value) == 120
print("probes ok")
PY

echo "== ujs fold =="
echo 'return 1+2*3;' > /tmp/t.ujs
python3 -m ujs fold /tmp/t.ujs

echo "== ujs icfold =="
python3 -m ujs icfold /tmp/t.ujs
python3 - <<'PY'
from ujs import api
from ujs.oracle import Oracle
from ujs import value as V
o = Oracle(drive="gold")
probes = [
    "return 1 + 2 * 3;",
    "let xs=[1,2,3]; return xs[1] + len(xs);",
    'let d={a:1}; return d.a;',
    """
function add(a,b){ return a+b; }
return add(10,20);
""",
]
for src in probes:
    r0 = api.run(src, oracle=o, ic=False)
    r1 = api.run(src, oracle=o, ic=True)
    assert r0.ok and r1.ok, (src, r0.err, r1.err)
    assert V.to_py(r0.value) == V.to_py(r1.value), src
print("icfold probes ok")
PY

echo "== ujs difftest =="
python3 -m ujs difftest

echo "== ujs ship =="
python3 -m ujs ship --out /tmp/ujs-kit.zip
python3 - <<'PY'
import zipfile, json
z = zipfile.ZipFile("/tmp/ujs-kit.zip")
names = set(z.namelist())
assert "MANIFEST.json" in names
assert "weights/built.json" in names
assert "native/ujs_vm.c" in names
assert "samples/fact.wasm" in names
m = json.loads(z.read("MANIFEST.json"))
assert m["format"] == "UJS-1"
assert all(s["exact"] for s in m["stages"].values())
print("ship ok", sorted(names))
PY

echo "== ujs determinism =="
python3 - <<'PY'
from ujs.oracle import Oracle
from ujs.front.compile import compile_src
from ujs.bc_encode import encode_fn
from ujs.wat_vm import pack_program
o = Oracle(drive="gold")
src = "let n=5; let f=1; while(n>0){f=f*n; n=n-1;} return f;"
a = pack_program(encode_fn(compile_src(src, o)))
b = pack_program(encode_fn(compile_src(src, o)))
assert a == b and a[:4] != b"nope"
print("determinism OK", len(a), "bytes")
PY

echo "== ujs web-build =="
python3 -m ujs web-build
python3 -c 'assert open("ujs/web/ujs_rt.wasm","rb").read(4)==b"\0asm"; import os; print("wasm", os.path.getsize("ujs/web/ujs_rt.wasm"), "B")'
if command -v node >/dev/null; then
  node --input-type=module <<'JS'
import fs from 'fs';
const demos = JSON.parse(fs.readFileSync('ujs/web/demos.json','utf8'));
const buf = fs.readFileSync('ujs/web/ujs_rt.wasm');
const { instance } = await WebAssembly.instantiate(buf);
const { run, memory } = instance.exports;
for (const [id, d] of Object.entries(demos)) {
  const code = Uint8Array.from(d.code);
  new Uint8Array(memory.buffer).set(code, 16384);
  const r = Number(run(16384, code.length));
  if (r !== d.expect) throw new Error(id + ' ' + r);
}
console.log('node demos OK');
JS
  echo 'let n=5; let f=1; while(n>0){f=f*n; n=n-1;} return f;' > /tmp/ujs_fact.ujs
  python3 -m ujs js2wasm /tmp/ujs_fact.ujs -o /tmp/ujs_fact.wasm
  node --input-type=module -e 'import fs from "fs"; const {instance}=await WebAssembly.instantiate(fs.readFileSync("/tmp/ujs_fact.wasm")); const h=instance.exports.main_export(); if (Number(instance.exports.i64_of_export(h))!==120) throw new Error("js2wasm"); console.log("js2wasm OK");'
  # full demos from web-build
  node --input-type=module <<'JS'
import fs from 'fs';
const meta = JSON.parse(fs.readFileSync('ujs/web/full_demos.json','utf8'));
const expect = {arith:7, fact:120, str:'hello ujs', dict:3, sum:60, arrow:144, forof:15, ternary:100, nullish:42, blockarrow:42};
for (const [id, d] of Object.entries(meta)) {
  const { instance } = await WebAssembly.instantiate(fs.readFileSync('ujs/web/'+d.wasm));
  const h = instance.exports.main_export();
  const t = instance.exports.tag_of_export(h);
  let v;
  if (t===2) v = Number(instance.exports.i64_of_export(h));
  else if (t===4) {
    const p=instance.exports.str_ptr_export(h), n=instance.exports.str_len_export(h), b=instance.exports.mem_base();
    v = new TextDecoder().decode(new Uint8Array(instance.exports.memory.buffer, b+p, n));
  }
  if (v !== expect[id]) throw new Error(id+' '+v);
}
console.log('full demos OK');
JS
  echo "== in-page wasm_run =="
  node --input-type=module <<'JS'
import { bootRuntime, wasm_run } from './ujs/web/wasm_run.js';
await bootRuntime('ujs_full.wasm');
const probes = [
  ['return 1+2*3;', 7],
  ['const n=5; let f=1; while(n>0){f=f*n; n=n-1;} return f;', 120],
  ['return "hi"+"!";', 'hi!'],
  ['return 1===1;', true],
  ['return x+y;', 13, {x:10,y:3}, {}],
  ['return 1?2:3;', 2],
  ['let f=x=>x*2; return f(21);', 42],
  ['let s=0; for(let x of [1,2,3]){s+=x;} return s;', 6],
  ['return typeof 1;', 'i64'],
  ['let d={a:1}; return "a" in d;', true],
  ['return null ?? 9;', 9],
  ['let a=1, b=2; return a+b;', 3],
  ['let f=x=>{return x+1;}; return f(41);', 42],
];
for (const row of probes) {
  const [src, exp, G, L] = row;
  const r = await wasm_run(src, G||{}, L||{});
  if (r.err) throw new Error(JSON.stringify(r.err)+' '+src);
  if (JSON.stringify(r.ok) !== JSON.stringify(exp))
    throw new Error(src+' got '+r.ok+' want '+exp);
}
console.log('in-page wasm_run OK');
JS
fi

echo "ujs OK"
