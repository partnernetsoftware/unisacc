const HOST_IMPORTS = new Set(["host_time"]);
const ABI_VERSION = 1;

function importsFor(module, namespace, available, allowed) {
  const imports = Object.create(null);
  for (const entry of WebAssembly.Module.imports(module)) {
    if (entry.kind !== "function" || entry.module !== namespace ||
        !allowed(entry.name) || typeof available[entry.name] !== "function") {
      throw new WebAssembly.LinkError("unsupported ABI import");
    }
    imports[entry.name] = available[entry.name];
  }
  return { [namespace]: imports };
}

function verifyVersion(instance) {
  const version = instance.exports.uxe_abi_version;
  if (typeof version !== "function" || version() !== ABI_VERSION) {
    throw new WebAssembly.LinkError("UXE ABI version");
  }
}

export function linkUxe(baseModule, productModule, host) {
  if (!(baseModule instanceof WebAssembly.Module) || !(productModule instanceof WebAssembly.Module)) {
    throw new WebAssembly.LinkError("missing UXE module");
  }
  const base = new WebAssembly.Instance(baseModule,
    importsFor(baseModule, "env", host, name => HOST_IMPORTS.has(name)));
  verifyVersion(base);
  const product = new WebAssembly.Instance(productModule,
    importsFor(productModule, "uxe_v1", base.exports, name => name === "uxe_dispatch"));
  verifyVersion(product);
  if (!(base.exports.memory instanceof WebAssembly.Memory) ||
      !(product.exports.memory instanceof WebAssembly.Memory) ||
      base.exports.memory === product.exports.memory) {
    throw new WebAssembly.LinkError("UXE memory ownership");
  }
  return { base: base.exports, product: product.exports };
}
