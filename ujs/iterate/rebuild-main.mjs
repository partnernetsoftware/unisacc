/**
 * Rebuild stage0-shaped module with a replacement main (func 1) body.
 * body = locals decls + ops + 0x0b  (no leading size uleb)
 *
 * Also patches host_set_global / host_get_global bounds to `ng` (default 64 =
 * MAXG). The RT stub was frozen with ng=1, which silently drops injects for
 * every global except index 0.
 */
export function rebuildWithMain(templateBytes, mainBody, opts = {}) {
  const ng = opts.ng != null ? opts.ng | 0 : 64;
  if (ng < 1 || ng > 255) throw new Error("rebuildWithMain: ng out of range");

  const data = templateBytes instanceof Uint8Array
    ? templateBytes
    : new Uint8Array(templateBytes);

  function readUleb(buf, i) {
    let v = 0, s = 0;
    for (;;) {
      const b = buf[i++];
      v |= (b & 0x7f) << s;
      if (!(b & 0x80)) return [v, i];
      s += 7;
    }
  }
  function writeUleb(v) {
    const out = [];
    for (;;) {
      let b = v & 0x7f;
      v >>>= 7;
      if (v) out.push(b | 0x80);
      else { out.push(b); return out; }
    }
  }

  // export name → func index
  const exportIdx = Object.create(null);
  {
    let i = 8;
    while (i < data.length) {
      const sid = data[i++];
      const [ln, i2] = readUleb(data, i);
      i = i2;
      const end = i + ln;
      if (sid === 7) {
        let o = i;
        const [n] = readUleb(data, o);
        o = readUleb(data, o)[1];
        for (let e = 0; e < n; e++) {
          const [nl, o2] = readUleb(data, o);
          o = o2;
          const name = new TextDecoder().decode(data.subarray(o, o + nl));
          o += nl;
          const kind = data[o++];
          const [idx, o3] = readUleb(data, o);
          o = o3;
          if (kind === 0) exportIdx[name] = idx;
        }
      }
      i = end;
    }
  }
  const patchFi = new Set(
    [exportIdx.host_set_global, exportIdx.host_get_global].filter((x) => x != null),
  );

  /** Patch i32.const bound in `local.get 0; i32.const N; i32.ge_u; if ...` */
  function patchNgBound(fbody) {
    const u = new Uint8Array(fbody);
    // skip local decls
    let i = 0;
    const [nloc, i1] = readUleb(u, i);
    i = i1;
    for (let g = 0; g < nloc; g++) {
      const [, i2] = readUleb(u, i);
      i = i2 + 1; // count + type
    }
    // expect: 20 00 41 <uleb ng> 4f
    if (u[i] !== 0x20 || u[i + 1] !== 0x00 || u[i + 2] !== 0x41) return fbody;
    let j = i + 3;
    const [, j2] = readUleb(u, j);
    if (u[j2] !== 0x4f) return fbody;
    const head = u.subarray(0, i + 3);
    const tail = u.subarray(j2);
    return concat([head, writeUleb(ng), tail]);
  }

  let i = 8;
  const before = [data.subarray(0, 8)];
  let codeSec = null;
  let after = [];
  while (i < data.length) {
    const sid = data[i];
    const start = i;
    i++;
    const [ln, i2] = readUleb(data, i);
    i = i2;
    const body = data.subarray(i, i + ln);
    i += ln;
    if (sid === 10) codeSec = { start, body: new Uint8Array(body) };
    else if (!codeSec) before.push(data.subarray(start, i));
    else after.push(data.subarray(start, i));
  }
  if (!codeSec) throw new Error("no code section in template");

  const code = codeSec.body;
  let [nfuncs, off] = readUleb(code, 0);
  const parts = [writeUleb(nfuncs)];
  for (let fi = 0; fi < nfuncs; fi++) {
    const [sz, off2] = readUleb(code, off);
    let fbody = code.subarray(off2, off2 + sz);
    off = off2 + sz;
    if (fi === 1) {
      const nb = mainBody instanceof Uint8Array ? mainBody : new Uint8Array(mainBody);
      parts.push(writeUleb(nb.length));
      parts.push(nb);
    } else {
      if (patchFi.has(fi)) fbody = patchNgBound(fbody);
      parts.push(writeUleb(fbody.length));
      parts.push(fbody);
    }
  }
  const newCode = concat(parts);
  const out = concat([
    ...before,
    [10],
    writeUleb(newCode.length),
    newCode,
    ...after,
  ]);
  return out;
}

function concat(chunks) {
  let n = 0;
  for (const c of chunks) n += c.length;
  const u = new Uint8Array(n);
  let o = 0;
  for (const c of chunks) {
    u.set(c, o);
    o += c.length;
  }
  return u;
}
