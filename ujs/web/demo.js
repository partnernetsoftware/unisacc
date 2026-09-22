/** UJS web — maximize inference-compiler showcase. */
import { bootRuntime, wasm_run } from "./wasm_run.js";
import { jspiSupported, runJspiProbe } from "./jspi.js";

const TAG = { null: 0, bool: 1, i64: 2, f64: 3, str: 4, list: 5, dict: 6, fn: 7 };

const SAMPLES = [
  { id: "fact", title: "factorial",
    src: `const n = 5;
let f = 1;
while (n > 0) {
  f = f * n;
  n = n - 1;
}
return f;` },
  { id: "ternary", title: "ternary",
    src: `let x = 7;
return x > 5 ? "big" : "small";` },
  { id: "arrow", title: "arrow",
    src: `let twice = x => x * 2;
let add = (a, b) => a + b;
return add(twice(10), 1);` },
  { id: "forof", title: "for-of",
    src: `let s = 0;
for (let x of [10, 20, 30]) {
  s += x;
}
return s;` },
  { id: "typeof", title: "typeof / in",
    src: `let d = {a: 1, b: 2};
return typeof d === "dict" && "a" in d;` },
  { id: "rest", title: "rest",
    src: `function sum(a, ...rest) {
  let s = a; let i = 0;
  while (i < len(rest)) { s = s + rest[i]; i = i + 1; }
  return s;
}
return sum(1, 2, 3, 4);` },
  { id: "switch", title: "switch",
    src: `let x = 2;
switch (x) {
  case 1: return 10;
  case 2: return 20;
  default: return 0;
}` },
  { id: "str", title: "strings",
    src: `return "hello" + " " + "ujs";` },
  { id: "nullish", title: "??",
    src: `let a = null;
let b = a ?? 99;
return b ?? 1;` },
  { id: "multilet", title: "multi-let",
    src: `let a = 1, b = 2, c = 3;
return a + b + c;` },
  { id: "blockarrow", title: "block =>",
    src: `let f = x => {
  let y = x * 2;
  return y + 1;
};
return f(10);` },
  { id: "methodish", title: "dot+call",
    src: `let d = {a: 10};
function get(o) { return o.a; }
return get(d) + 5;` },
  { id: "globals", title: "globals",
    src: `return rate * amount;`,
    G: { rate: 3, amount: 100 } },
];

let askWasm, manifest, demos, fullDemos;

async function boot() {
  const status = document.getElementById("status");
  try {
    const [man, dem, fullMeta, askBuf] = await Promise.all([
      fetch("manifest.json").then((r) => r.json()),
      fetch("demos.json").then((r) => r.json()),
      fetch("full_demos.json").then((r) => r.json()).catch(() => ({})),
      fetch("ujs_rt.wasm").then((r) => r.arrayBuffer()),
    ]);
    manifest = man;
    demos = dem;
    fullDemos = fullMeta;
    askWasm = (await WebAssembly.instantiate(askBuf)).instance.exports;
    await bootRuntime("ujs_full.wasm");
    const stages = Object.keys(man.stages).length;
    let jspiLine = "JSPI: checking…";
    status.innerHTML =
      `<p>就绪 · <strong>${stages} ask stages</strong> · full demos ${Object.keys(fullMeta).length} · ` +
      `<code>wasm_run</code> live · <span id="jspi-status">${jspiLine}</span></p>`;
    fillChips();
    fillStages();
    fillDemos();
    fillFullDemos();
    loadSample(SAMPLES[0]);
    document.getElementById("run-btn").onclick = doWasmRun;
    const jspiBtn = document.getElementById("jspi-btn");
    if (jspiBtn) jspiBtn.onclick = doJspi;
    // async status
    if (!jspiSupported()) {
      document.getElementById("jspi-status").textContent = "JSPI: 本浏览器未开";
    } else {
      document.getElementById("jspi-status").textContent = "JSPI: 可用（可点探针）";
    }
  } catch (e) {
    status.innerHTML =
      `<p>加载失败：${e.message}。先跑 <code>python3 -m ujs web-build</code>。</p>`;
    console.error(e);
  }
}

async function doJspi() {
  const out = document.getElementById("jspi-out");
  out.textContent = "running…";
  try {
    const r = await runJspiProbe(40);
    out.textContent = JSON.stringify(r, null, 2);
  } catch (e) {
    out.textContent = "ERR " + e.message;
  }
}

function fillChips() {
  const box = document.getElementById("chips");
  box.innerHTML = "";
  for (const s of SAMPLES) {
    const b = document.createElement("button");
    b.type = "button";
    b.className = "chip";
    b.textContent = s.title;
    b.onclick = () => loadSample(s);
    box.appendChild(b);
  }
}

function loadSample(s) {
  document.getElementById("src").value = s.src;
  document.getElementById("globals").value = JSON.stringify(s.G || {});
  document.getElementById("locals").value = JSON.stringify(s.L || {});
}

function renderHeat(heat) {
  const box = document.getElementById("heat");
  const stages = Object.keys(heat).sort();
  if (!stages.length) {
    box.innerHTML = "<p class='hint'>跑一次 wasm_run 查看 ask 热力</p>";
    return;
  }
  const max = Math.max(...Object.values(heat));
  box.innerHTML = stages.map((st) => {
    const n = heat[st];
    const pct = Math.round((n / max) * 100);
    return `<div class="heat-row"><span>${st}</span>` +
      `<i style="width:${pct}%"></i><b>${n}</b></div>`;
  }).join("");
}

async function doWasmRun() {
  const src = document.getElementById("src").value;
  const out = document.getElementById("run-out");
  const trace = document.getElementById("trace-out");
  let G = {}, L = {};
  try {
    G = JSON.parse(document.getElementById("globals").value || "{}");
    L = JSON.parse(document.getElementById("locals").value || "{}");
  } catch (e) {
    out.textContent = "JSON error: " + e.message;
    return;
  }
  const r = await wasm_run(src, G, L);
  if (r.err) {
    out.textContent = "ERR " + JSON.stringify(r.err, null, 2);
    trace.textContent = "";
    return;
  }
  out.textContent =
    `→ ${JSON.stringify(r.ok)}\n` +
    `image ${r.imageBytes} B · ic_stub ${r.ic_stub}\n` +
    `locals ${JSON.stringify(r.locals)}\n` +
    `globals ${JSON.stringify(r.globals)}`;
  renderHeat(r.askHeat || {});
  const lines = (r.askTrace || []).slice(0, 40).map(
    (e, i) => `${String(i).padStart(3)}  ask(${e.stage}, ${JSON.stringify(e.key)}) → ${e.y}`
  );
  const more = (r.askTrace || []).length > 40
    ? `\n… +${r.askTrace.length - 40} more asks`
    : "";
  trace.textContent =
    `ask trace (${(r.askTrace || []).length} decisions)\n` + lines.join("\n") + more;
}

function fillStages() {
  const sel = document.getElementById("stage");
  sel.innerHTML = "";
  for (const name of Object.keys(manifest.stages)) {
    const o = document.createElement("option");
    o.value = name;
    o.textContent = name;
    sel.appendChild(o);
  }
  sel.onchange = renderFields;
  renderFields();
  document.getElementById("ask-btn").onclick = doAsk;
}

function renderFields() {
  const st = manifest.stages[document.getElementById("stage").value];
  const box = document.getElementById("fields");
  box.innerHTML = "";
  st.fields.forEach((f, i) => {
    const lab = document.createElement("label");
    lab.textContent = f.name;
    const s = document.createElement("select");
    s.id = "f" + i;
    f.vocab.forEach((v, vi) => {
      const o = document.createElement("option");
      o.value = String(vi);
      o.textContent = v;
      s.appendChild(o);
    });
    lab.appendChild(s);
    box.appendChild(lab);
  });
}

function doAsk() {
  const name = document.getElementById("stage").value;
  const st = manifest.stages[name];
  const idxs = st.fields.map((_, i) => Number(document.getElementById("f" + i).value) | 0);
  while (idxs.length < 3) idxs.push(0);
  const cls = askWasm.ask(st.id, idxs[0], idxs[1], idxs[2]);
  document.getElementById("ask-out").textContent =
    `ask(${name}, [${idxs}]) → class ${cls}  "${st.classes[cls]}"`;
}

function fillDemos() {
  const box = document.getElementById("demos");
  box.innerHTML = "";
  for (const [id, d] of Object.entries(demos)) {
    const b = document.createElement("button");
    b.type = "button";
    b.className = "demo-btn";
    b.textContent = "i64:" + d.title;
    b.onclick = () => runI64Demo(id);
    box.appendChild(b);
  }
}

function runI64Demo(id) {
  const d = demos[id];
  const code = Uint8Array.from(d.code);
  const base = Number(askWasm.mem_base?.() || 0);
  new Uint8Array(askWasm.memory.buffer).set(code, base + 16384);
  const result = Number(askWasm.run(16384, code.length));
  document.getElementById("demo-out").textContent =
    `${d.src}\n\n→ ${result}` + (result === d.expect ? "  OK" : `  expected ${d.expect}`);
}

function fillFullDemos() {
  const box = document.getElementById("full-demos");
  box.innerHTML = "";
  for (const [id, d] of Object.entries(fullDemos)) {
    const b = document.createElement("button");
    b.type = "button";
    b.className = "demo-btn";
    b.textContent = "wasm:" + id;
    b.onclick = () => runFullDemo(id);
    box.appendChild(b);
  }
}

async function runFullDemo(id) {
  const d = fullDemos[id];
  const buf = await fetch(d.wasm).then((r) => r.arrayBuffer());
  const { instance } = await WebAssembly.instantiate(buf);
  const ex = instance.exports;
  const h = ex.main_export();
  document.getElementById("full-out").textContent =
    `${d.src}\n\n→ ${JSON.stringify(readHandle(ex, h))}`;
}

function readHandle(ex, h) {
  const t = ex.tag_of_export(h);
  if (t === TAG.i64) return Number(ex.i64_of_export(h));
  if (t === TAG.str) {
    const p = ex.str_ptr_export(h), n = ex.str_len_export(h), b = ex.mem_base();
    return new TextDecoder().decode(new Uint8Array(ex.memory.buffer, b + p, n));
  }
  if (t === TAG.bool) return !!new Uint8Array(ex.memory.buffer)[ex.mem_base() + h + 4];
  if (t === TAG.null) return null;
  return { tag: t, handle: h };
}

boot();
