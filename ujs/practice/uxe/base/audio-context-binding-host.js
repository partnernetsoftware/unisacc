export function createAudioContextRegistry() {
  const contexts = new Map();
  const handles = new WeakMap();
  function register(context) {
    if (!context || typeof context.createAnalyser !== "function") return -1;
    const known = handles.get(context);
    if (known !== undefined) return known;
    if (contexts.size >= 64) return -1;
    const handle = contexts.size + 1;
    contexts.set(handle, context);
    handles.set(context, handle);
    return handle;
  }
  function host_context_identity(action, handle) {
    if (action === 1n) return contexts.has(Number(handle)) ? 1n : 0n;
    if (action !== 0n || contexts.size >= 64) return -1n;
    try {
      const Constructor = window.AudioContext || window.webkitAudioContext;
      return BigInt(register(new Constructor()));
    } catch {
      return -1n;
    }
  }
  return {
    imports: { host_context_identity },
    register,
    resolve: handle => contexts.get(Number(handle)),
  };
}
