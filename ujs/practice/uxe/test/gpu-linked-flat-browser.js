try {
  if (REVISION !== "160") throw new Error("three.js revision mismatch");
  const { gl } = context();
  function module(name) {
    const request = new XMLHttpRequest();
    request.open("GET", name + ".wasm", false);
    request.overrideMimeType("text/plain; charset=x-user-defined");
    request.send();
    return new WebAssembly.Module(Uint8Array.from(request.responseText, value => value.charCodeAt(0) & 255));
  }
  function onlyImport(wasm, namespace, name) {
    const imports = WebAssembly.Module.imports(wasm);
    return imports.length === 1 && imports[0].kind === "function" &&
      imports[0].module === namespace && imports[0].name === name;
  }
  const baseModule = module("base"), probeModule = module("probe");
  if (!onlyImport(baseModule, "uxe_gpu_v1", "host_gpu_submit") ||
      !onlyImport(probeModule, "uxe_gpu_link_v1", "uxe_gpu_dispatch")) {
    throw new Error("linked GPU import boundary mismatch");
  }
  let base;
  const submit = createGpuSubmit(gl, () => base.exports.memory, () => 1);
  base = new WebAssembly.Instance(baseModule, { uxe_gpu_v1: { host_gpu_submit: submit } });
  const probe = new WebAssembly.Instance(probeModule, {
    uxe_gpu_link_v1: { uxe_gpu_dispatch: base.exports.uxe_gpu_dispatch },
  });
  if (base.exports.memory === probe.exports.memory) throw new Error("linked GPU memories alias");
  const visits = [];
  const actual = [], status = [];
  const mutate = new URLSearchParams(location.search).has("mutate");
  function step(mode) {
    visits.push(Number(mode));
    const rc = probe.exports.uxe_gpu_probe(mode);
    const caller = new DataView(probe.exports.memory.buffer);
    const callee = new DataView(base.exports.memory.buffer);
    if (caller.getUint32(2097152, true) !== 1515870810 ||
        callee.getUint32(2097152, true) !== 1346721877) {
      throw new Error("linked dispatch crossed memory owners");
    }
    return rc;
  }
