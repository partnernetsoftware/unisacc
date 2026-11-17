/* host1-runtime.js -- instantiate host1_package wasm. Gold via main_export.
 * Reads custom section ujsmeta if present. run_step still re-runs main.
 */
(function (g) {
  function u8fromB64(b64) {
    var bin = atob(b64), u = new Uint8Array(bin.length), i;
    for (i = 0; i < bin.length; i++) u[i] = bin.charCodeAt(i);
    return u.buffer;
  }
  function uleb(u, i) {
    var n = 0, sh = 0, b;
    while (i < u.length) {
      b = u[i++];
      n = n + ((b & 127) << sh);
      if (b < 128) return { n: n, i: i };
      sh = sh + 7;
    }
    return { n: 0, i: i };
  }
  function parseUjsmeta(buf) {
    var u = buf instanceof Uint8Array ? buf : new Uint8Array(buf);
    var i = 8, sid, sz, name, body, j;
    if (u.length < 8 || u[0] || u[1] !== 97) return null;
    while (i < u.length) {
      sid = u[i++];
      sz = uleb(u, i);
      i = sz.i;
      if (sid === 0) {
        j = i;
        name = uleb(u, j);
        body = u.subarray(name.i + name.n, i + sz.n);
        name = String.fromCharCode.apply(null, u.subarray(name.i, name.i + name.n));
        if (name === "ujsmeta") {
          try { return JSON.parse(String.fromCharCode.apply(null, body)); }
          catch (e) { return null; }
        }
      }
      i = i + sz.n;
    }
    return null;
  }
  g.host1Boot = async function (src) {
    var buf = src;
    if (typeof src === "string") buf = u8fromB64(src);
    var meta = parseUjsmeta(buf);
    var memory = null;
    function gpuSubmit(ptr, len) {
      if (typeof g.onHostGpuSubmit !== "function" || !memory) return;
      var u = new Uint8Array(memory.buffer, ptr >>> 0, len >>> 0);
      var ab = u.buffer.slice(u.byteOffset, u.byteOffset + u.byteLength);
      g.onHostGpuSubmit(ab);
    }
    var inst = await WebAssembly.instantiate(buf, { env: { host_gpu_submit: gpuSubmit } });
    var e = inst.instance.exports;
    memory = e.memory;
    return {
      exports: e,
      meta: meta,
      goldI64: function () {
        return Number(e.i64_of_export(e.main_export()));
      },
      mainHandle: function () { return e.main_export(); },
      tag: function (h) { return e.tag_of_export(h); },
      i64: function (h) { return Number(e.i64_of_export(h)); },
      readI64List: function (h) {
        var n = e.host_len(h);
        var i, el, out;
        if (typeof n !== "number") n = Number(e.i64_of_export(n));
        out = [];
        for (i = 0; i < n; i++) {
          el = e.host_list_get(h, i);
          out.push(Number(e.i64_of_export(el)));
        }
        return out;
      }
    };
  };
})(typeof window !== "undefined" ? window : globalThis);

