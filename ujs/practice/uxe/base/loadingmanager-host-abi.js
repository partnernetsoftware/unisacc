// Portable host capabilities for the LoadingManager Wasm module. The host
// owns JS identities, while Wasm owns callbacks, handler order, and counts.
const NAMESPACE = "uxe_lm_v1";
const MAX_TOKEN = 0xffffffff;
const HOST_NAMES = new Set(["host_lm_url", "host_lm_reset", "host_lm_test", "host_lm_cb"]);

function tokenNumber(value) {
  if (typeof value === "bigint" && value >= 0n && value <= 0xffffffffn) return Number(value);
  if (Number.isInteger(value) && value >= 0 && value <= MAX_TOKEN) return value;
  throw new RangeError("LoadingManager handle");
}

function identityTable() {
  const values = [];
  const tokens = new Map();
  return {
    register(value) {
      const previous = tokens.get(value);
      if (previous !== undefined) return previous;
      if (values.length === MAX_TOKEN) throw new RangeError("LoadingManager handle capacity");
      values.push(value);
      const token = values.length;
      tokens.set(value, token);
      return token;
    },
    read(raw) {
      const token = tokenNumber(raw);
      if (token === 0 || token > values.length) throw new RangeError("unknown LoadingManager handle");
      return values[token - 1];
    }
  };
}

export function createLoadingManagerHost() {
  const urls = identityTable();
  const regexes = identityTable();
  const loaders = identityTable();
  const callbacks = identityTable();
  const receivers = new Map();

  function registerURL(url) {
    if (typeof url !== "string") throw new TypeError("LoadingManager URL");
    return urls.register(url);
  }

  function registerCallback(callback) {
    if (callback === undefined || callback === null) return 0;
    if (typeof callback !== "function") throw new TypeError("LoadingManager callback");
    return callbacks.register(callback);
  }

  const imports = {
    host_lm_url(callback, url) {
      return BigInt(registerURL(callbacks.read(callback)(urls.read(url))));
    },
    host_lm_reset(regex) {
      regexes.read(regex).lastIndex = 0;
      return 0n;
    },
    host_lm_test(regex, url) {
      return regexes.read(regex).test(urls.read(url)) ? 1n : 0n;
    },
    host_lm_cb(callback, kind, receiver, url, loaded, total) {
      const fn = callbacks.read(callback);
      const owner = receivers.get(tokenNumber(receiver));
      if (owner === undefined) throw new RangeError("unbound LoadingManager receiver");
      switch (kind) {
        case 1n:
        case 2n:
          fn.call(owner, urls.read(url), tokenNumber(loaded), tokenNumber(total));
          break;
        case 3n:
          fn.call(owner, urls.read(url));
          break;
        case 4n:
          fn.call(owner);
          break;
        default:
          throw new RangeError("LoadingManager event");
      }
      return 0n;
    }
  };

  return {
    imports,
    registerURL,
    readURL: urls.read,
    registerCallback,
    registerRegExp(regex) {
      if (!regex || typeof regex.test !== "function") throw new TypeError("LoadingManager regex");
      return { token: regexes.register(regex), global: Number(Boolean(regex.global)) };
    },
    registerLoader: loaders.register,
    readLoader(token) {
      return tokenNumber(token) === 0 ? null : loaders.read(token);
    },
    bindManager(address, receiver) {
      const token = tokenNumber(address);
      if (!receiver || (typeof receiver !== "object" && typeof receiver !== "function")) {
        throw new TypeError("LoadingManager receiver");
      }
      if (receivers.has(token) && receivers.get(token) !== receiver) {
        throw new RangeError("LoadingManager receiver already bound");
      }
      receivers.set(token, receiver);
    }
  };
}

// Apply the dedicated capabilities to one module without any DOM or renderer
// dependency. The shared linker can add the same provider as a namespace.
export function loadingManagerImports(module, host) {
  if (!(module instanceof WebAssembly.Module) || !host?.imports) {
    throw new WebAssembly.LinkError("missing LoadingManager module or provider");
  }
  const imports = Object.create(null);
  for (const binding of WebAssembly.Module.imports(module)) {
    if (binding.kind !== "function" || binding.module !== NAMESPACE ||
        !HOST_NAMES.has(binding.name) || !Object.hasOwn(host.imports, binding.name) ||
        typeof host.imports[binding.name] !== "function") {
      throw new WebAssembly.LinkError("unsupported LoadingManager ABI import");
    }
    imports[binding.name] = host.imports[binding.name];
  }
  return { [NAMESPACE]: imports };
}

export function linkLoadingManagerModule(module, host) {
  const { exports } = new WebAssembly.Instance(module, loadingManagerImports(module, host));
  if (typeof exports.uxe_abi_version !== "function" || exports.uxe_abi_version() !== 1 ||
      !(exports.memory instanceof WebAssembly.Memory)) {
    throw new WebAssembly.LinkError("LoadingManager ABI version or memory");
  }
  return exports;
}
