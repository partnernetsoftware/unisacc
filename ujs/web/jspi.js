/** JSPI probe — Chrome/Firefox stack-suspend ↔ Promise. */
export function jspiSupported() {
  return typeof WebAssembly !== "undefined"
    && typeof WebAssembly.promising === "function"
    && typeof WebAssembly.Suspending === "function";
}

function u32leb(n) {
  const out = [];
  while (true) {
    let b = n & 0x7f;
    n >>>= 7;
    if (n) out.push(b | 0x80);
    else { out.push(b); break; }
  }
  return out;
}

function section(id, body) {
  return [id, ...u32leb(body.length), ...body];
}

/** Build: import env.delay(i32); export run() -> i32 { delay(ms); return 42 } */
function buildModule(ms) {
  const types = [2,
    0x60, 1, 0x7f, 0x00,       // (i32)->()
    0x60, 0, 1, 0x7f,          // ()->i32
  ];
  const imports = [1,
    3, 0x65, 0x6e, 0x76,       // "env"
    5, 0x64, 0x65, 0x6c, 0x61, 0x79, // "delay"
    0x00, 0x00,                // func type 0
  ];
  const funcs = [1, 1];        // one func, type 1
  const exports = [1,
    3, 0x72, 0x75, 0x6e,       // "run"
    0x00, 0x01,                // func index 1 (0=import)
  ];
  // local func: i32.const ms; call 0; i32.const 42; end
  const codeBody = [0x00, 0x41, ...u32leb(ms), 0x10, 0x00, 0x41, 0x2a, 0x0b];
  const code = [1, ...u32leb(codeBody.length), ...codeBody];
  const bytes = [
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00,
    ...section(1, types),
    ...section(2, imports),
    ...section(3, funcs),
    ...section(7, exports),
    ...section(10, code),
  ];
  return new Uint8Array(bytes);
}

export async function runJspiProbe(ms = 30) {
  if (!jspiSupported()) {
    return { supported: false, note: "WebAssembly.promising / Suspending missing" };
  }
  const delay = (n) => new Promise((r) => setTimeout(r, Number(n)));
  const imports = { env: { delay: new WebAssembly.Suspending(delay) } };
  const { instance } = await WebAssembly.instantiate(buildModule(ms), imports);
  const run = WebAssembly.promising(instance.exports.run);
  const t0 = performance.now();
  const v = await run();
  const dt = performance.now() - t0;
  return {
    supported: true,
    value: Number(v),
    ms: Math.round(dt),
    note: "wasm stack suspended while Promise pending",
  };
}
