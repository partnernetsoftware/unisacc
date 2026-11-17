/* host1-runtime.js -- instantiate host1_package wasm. Gold via main_export.
 * Reads custom section ujsmeta if present. step uses host_reset / host_set_global / run_step.
 */
(function (g) {
  var TAG = { null: 0, bool: 1, i64: 2, f64: 3, str: 4, list: 5, dict: 6 };

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

  function pyToHandle(ex, mem, v, scratch) {
    var bytes, base, h, i, keys;
    if (v === null || v === undefined) return ex.host_mk_null();
    if (typeof v === "boolean") return ex.host_mk_bool(v ? 1 : 0);
    if (typeof v === "number") {
      if (Number.isInteger(v) && Math.abs(v) <= Number.MAX_SAFE_INTEGER)
        return ex.host_mk_i64(BigInt(v));
      return ex.host_mk_f64(v);
    }
    if (typeof v === "bigint") return ex.host_mk_i64(v);
    if (typeof v === "string") {
      bytes = new TextEncoder().encode(v);
      base = Number(ex.mem_base());
      new Uint8Array(mem.buffer, base + scratch, bytes.length).set(bytes);
      return ex.host_mk_str(scratch, bytes.length);
    }
    if (Array.isArray(v)) {
      h = ex.host_mk_list(v.length);
      for (i = 0; i < v.length; i++)
        ex.host_list_set(h, i, pyToHandle(ex, mem, v[i], scratch));
      return h;
    }
    if (typeof v === "object") {
      keys = Object.keys(v);
      h = ex.host_mk_dict(keys.length);
      for (i = 0; i < keys.length; i++) {
        ex.host_dict_set(h, i,
          pyToHandle(ex, mem, keys[i], scratch),
          pyToHandle(ex, mem, v[keys[i]], scratch));
      }
      return h;
    }
    throw new Error("unsupported type " + typeof v);
  }

  function readHandle(ex, mem, h) {
    var t, p, n, out, i, k;
    if (!h) return null;
    t = ex.tag_of_export(h);
    if (t === TAG.null) return null;
    if (t === TAG.bool) return !!new Uint8Array(mem.buffer)[Number(ex.mem_base()) + h + 4];
    if (t === TAG.i64) return Number(ex.i64_of_export(h));
    if (t === TAG.f64) return Number(ex.f64_of_export(h));
    if (t === TAG.str) {
      p = Number(ex.str_ptr_export(h));
      n = Number(ex.str_len_export(h));
      return new TextDecoder().decode(
        new Uint8Array(mem.buffer, Number(ex.mem_base()) + p, n));
    }
    if (t === TAG.list) {
      n = Number(ex.host_len(h));
      out = [];
      for (i = 0; i < n; i++) out.push(readHandle(ex, mem, ex.host_list_get(h, i)));
      return out;
    }
    if (t === TAG.dict) {
      n = Number(ex.host_len(h));
      out = {};
      for (i = 0; i < n; i++) {
        k = readHandle(ex, mem, ex.host_dict_key(h, i));
        out[k] = readHandle(ex, mem, ex.host_dict_val(h, i));
      }
      return out;
    }
    throw new Error("unsupported tag " + t);
  }

  g.host1Boot = async function (src) {
    var buf = src;
    if (typeof src === "string") buf = u8fromB64(src);
    var meta = parseUjsmeta(buf);
    var inst = await WebAssembly.instantiate(buf);
    var e = inst.instance.exports;
    var mem = e.memory;
    var gnames = (meta && meta.globals) ? meta.globals : [];
    var scratch = typeof e.host_scratch === "function"
      ? Number(e.host_scratch()) : 950000;
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
      },
      step: function (globalsMap) {
        var k, v, ix, map = globalsMap || {};
        e.host_reset();
        for (k in map) {
          if (!Object.prototype.hasOwnProperty.call(map, k)) continue;
          v = map[k];
          ix = gnames.indexOf(k);
          if (ix >= 0) e.host_set_global(ix, pyToHandle(e, mem, v, scratch));
        }
        return readHandle(e, mem, e.run_step());
      }
    };
  };
})(typeof window !== "undefined" ? window : globalThis);
