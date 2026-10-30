#!/usr/bin/env node
// release/apeinspect.mjs -- inspect bounded APE package/certificate extents; does not
// authenticate signatures.  Node port of the archived release/apeinspect.py (now
// archive/reference-py/release/apeinspect.py), same checks in the same order and the same
// output bytes:
//
//   node release/apeinspect.mjs FILE...
//
// prints one JSON list (json.dumps indent=2, keys in the Python order); a file that is not a
// well-formed APE prints "PATH: REASON" on stderr and exits 1 before anything is printed.
// import { inspect } from './apeinspect.mjs' returns the same fields for one buffer and throws
// ApeError(reason).  release/apeinspect.cx is the CX form of the same inspector.
import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { dumps, osError, PyError } from './ci/pycompat.mjs';

export class ApeError extends PyError {
  constructor(reason) { super('ValueError', reason); }
}
const sha256 = b => crypto.createHash('sha256').update(b).digest('hex');
const zero = b => b.every(x => x === 0);

export function inspect(data) {
  const size = data.length;
  const span = (off, n) => {
    if (off < 0 || n < 0 || off > size || n > size - off) throw new ApeError('extent outside file');
    return data.subarray(off, off + n);
  };
  const number = (off, n) => { let v = 0; const b = span(off, n); for (let i = n - 1; i >= 0; i--) v = v * 256 + b[i]; return v; };
  const big = (off, n) => { let v = 0n; const b = span(off, n); for (let i = n - 1; i >= 0; i--) v = v * 256n + BigInt(b[i]); return v; };
  const is = (b, text) => Buffer.from(b).equals(Buffer.from(text, 'latin1'));
  let cert = 0, certlen = 0, minimum = 0;
  if (size < 16) throw new ApeError('short file');
  if (is(span(0, 2), 'MZ')) {
    const pe = number(60, 4);
    if (pe < 64 || !is(span(pe, 4), 'PE\0\0')) throw new ApeError('invalid PE offset/header');
    const sections = number(pe + 6, 2), optsize = number(pe + 20, 2);
    const opt = pe + 24;
    span(opt, optsize);
    const magic = number(opt, 2);
    const dirs = { 0x10b: 96, 0x20b: 112 }[magic];
    if (!dirs || optsize < dirs) throw new ApeError('invalid optional header');
    const count = number(opt + dirs - 4, 4);
    if (count < 5 || count > Math.floor((optsize - dirs) / 8)) throw new ApeError('invalid data directories');
    cert = number(opt + dirs + 32, 4); certlen = number(opt + dirs + 36, 4);
    if (Boolean(cert) !== Boolean(certlen) || (cert && (cert % 8 || certlen < 8 || cert + certlen !== size))) {
      throw new ApeError('invalid security extent');
    }
    const sec = opt + optsize;
    if (!sections) throw new ApeError('no sections');
    span(sec, sections * 40);
    const headers = number(opt + 60, 4);
    if (headers < sec + sections * 40 || headers > size) throw new ApeError('invalid header extent');
    minimum = headers;
    for (let i = 0; i < sections; i++) {
      const rawlen = number(sec + i * 40 + 16, 4), raw = number(sec + i * 40 + 20, 4);
      if (rawlen) {
        if (raw < headers) throw new ApeError('section overlaps headers');
        span(raw, rawlen);
        minimum = Math.max(minimum, raw + rawlen);
      }
    }
    if (cert && cert < minimum) throw new ApeError('certificate overlaps sections');
  }
  if (cert) {
    let pos = cert;
    while (pos < size) {
      const length = number(pos, 4);
      if (length < 8 || !is(span(pos + 4, 4), '\0\x02\x02\0')) throw new ApeError('invalid WIN_CERTIFICATE');
      span(pos, length);
      const padding = ((-length % 8) + 8) % 8;
      if (!zero(span(pos + length, padding))) throw new ApeError('nonzero certificate padding');
      pos += length + padding;
    }
    if (pos !== size) throw new ApeError('invalid certificate table end');
  }
  const end = cert || size;
  const found = [];
  for (let padding = 0; padding < (cert ? 8 : 1); padding++) {
    const footer = end - padding - 16;
    if (footer < minimum || !zero(span(end - padding, padding))) continue;
    if (is(span(footer, 8), 'UNIPKG1\n')) {
      const length = big(footer + 8, 8);
      if (!length || length > BigInt(footer - minimum)) throw new ApeError('invalid package extent');
      found.push([footer, Number(length), padding]);
    }
  }
  if (found.length !== 1) throw new ApeError('missing or ambiguous package footer');
  const [footer, length, padding] = found[0];
  const pkg = span(footer - length, length);
  return {
    file_size: size, footer_offset: footer, package_offset: footer - length,
    package_length: length, certificate_offset: cert,
    certificate_length: certlen, padding,
    sha256: sha256(data),
    package_sha256: sha256(pkg),
    authentication: 'not_checked',
  };
}

// str(pathlib.Path(arg)) for the platform's flavour
function pathStr(arg) {
  if (process.platform === 'win32') {
    const s = arg.replace(/\//g, '\\');
    const drive = /^[A-Za-z]:/.test(s) ? s.slice(0, 2) : '';
    const rest = s.slice(drive.length);
    const root = rest.startsWith('\\') ? '\\' : '';
    const parts = rest.split('\\').filter(p => p && p !== '.');
    return drive + root + parts.join('\\') || '.';
  }
  const stripped = arg.replace(/^\/+/, '');
  const lead = arg.length - stripped.length;
  const root = lead === 0 ? '' : lead === 2 ? '//' : '/';
  const parts = stripped.split('/').filter(p => p && p !== '.');
  return root + parts.join('/') || '.';
}

function main() {
  const prog = path.basename(process.argv[1]);
  const eol = process.platform === 'win32' ? '\r\n' : '\n';
  const usage = `usage: ${prog} [-h] files [files ...]`;
  const files = [], unknown = [];
  let dashdash = false;
  for (const a of process.argv.slice(2)) {
    if (!dashdash && a === '--') { dashdash = true; continue; }
    if (!dashdash && (a === '-h' || (a.length > 2 && '--help'.startsWith(a)))) {
      // argparse wraps the description at the terminal width minus 2 (COLUMNS, else the tty, else 80)
      const width = (Number.parseInt(process.env.COLUMNS, 10) > 0 ? Number.parseInt(process.env.COLUMNS, 10) : process.stdout.columns || 80) - 2;
      const lines = [];
      for (const word of 'Inspect bounded APE package/certificate extents; does not authenticate signatures.'.split(' ')) {
        if (lines.length && lines[lines.length - 1].length + 1 + word.length <= width) lines[lines.length - 1] += ' ' + word;
        else lines.push(word);
      }
      process.stdout.write([usage, '', ...lines, '',
        'positional arguments:', '  files', '', 'options:', '  -h, --help  show this help message and exit', ''].join(eol));
      return 0;
    }
    if (!dashdash && a.length > 1 && a[0] === '-' && !/^-\d+$|^-\d*\.\d+$/.test(a)) unknown.push(a); else files.push(a);
  }
  const usageError = msg => { process.stderr.write(`${usage}${eol}${prog}: error: ${msg}${eol}`); return 2; };
  if (!files.length) return usageError('the following arguments are required: files');
  if (unknown.length) return usageError(`unrecognized arguments: ${unknown.join(' ')}`);
  const results = [];
  for (const arg of files) {
    const name = pathStr(arg);
    let result;
    try {
      let data;
      try { data = fs.readFileSync(name); } catch (e) { throw osError(e, name); }
      result = inspect(data);
    } catch (e) {
      if (!(e instanceof PyError)) throw e;
      process.stderr.write(`${name}: ${e.pyMessage}${eol}`);
      return 1;
    }
    results.push({ path: name, ...result });
  }
  process.stdout.write(dumps(results, 2).replace(/\n/g, eol) + eol);
  return 0;
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) process.exitCode = main();
