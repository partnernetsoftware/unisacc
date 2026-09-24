/**
 * Rebuild stage0-shaped module with a replacement main (func 1) body.
 * body = locals decls + ops + 0x0b  (no leading size uleb)
 */
export function rebuildWithMain(templateBytes, mainBody) {
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
    const fbody = code.subarray(off2, off2 + sz);
    off = off2 + sz;
    if (fi === 1) {
      const nb = mainBody instanceof Uint8Array ? mainBody : new Uint8Array(mainBody);
      parts.push(writeUleb(nb.length));
      parts.push(nb);
    } else {
      parts.push(writeUleb(sz));
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
