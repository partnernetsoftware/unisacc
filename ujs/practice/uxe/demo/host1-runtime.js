/* host1-runtime.js -- instantiate host1_package wasm. Gold via main_export.
 * Not a game step kernel: run_step re-runs main and re-inits let.
 */
(function (g) {
  function u8fromB64(b64) {
    var bin = atob(b64), u = new Uint8Array(bin.length), i;
    for (i = 0; i < bin.length; i++) u[i] = bin.charCodeAt(i);
    return u.buffer;
  }
  g.host1Boot = async function (src) {
    var buf = src;
    if (typeof src === "string") buf = u8fromB64(src);
    var inst = await WebAssembly.instantiate(buf);
    var e = inst.instance.exports;
    return {
      exports: e,
      goldI64: function () {
        return Number(e.i64_of_export(e.main_export()));
      }
    };
  };
})(typeof window !== "undefined" ? window : globalThis);
