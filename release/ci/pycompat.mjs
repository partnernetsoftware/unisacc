// release/ci/pycompat.mjs -- the small slice of Python semantics that the windows-signing
// workflow steps relied on, so their Node ports keep the same decisions and the same bytes.
//
// Value model (what loads() returns and what the helpers accept):
//   None -> null, bool -> true/false, int -> BigInt (a plain integral JS number is also an int),
//   float -> PyFloat, str -> string, list -> Array, dict -> PyDict (insertion ordered, Python
//   key equality: 1 == 1.0 == True).
// loads() follows the CPython 3.10 C scanner (strict strings, NaN/Infinity accepted, duplicate
// keys keep their first position and last value); dumps(v, 2) is json.dumps(v, indent=2) with
// ensure_ascii and float repr.  Errors are PyError(type, message) and run() prints them the way
// an uncaught Python exception ends ("Type: message", exit 1).
import fs from 'node:fs';
import os from 'node:os';

export class PyError extends Error {
  constructor(type, message) { super(message === undefined ? type : message); this.pyType = type; this.pyMessage = message; }
}
export class PyFloat {
  constructor(v) { this.v = v; }
}
const unhashable = v => Array.isArray(v) || v instanceof PyDict || v instanceof PySet;
function hashKey(k) {
  if (k === null) return 'N';
  if (typeof k === 'string') return 's:' + k;
  if (typeof k === 'boolean') return 'n:' + (k ? 1 : 0);
  if (typeof k === 'bigint') return 'n:' + k;
  const v = k instanceof PyFloat ? k.v : k;
  if (typeof v === 'number') {
    if (Number.isFinite(v) && Number.isInteger(v)) return 'n:' + BigInt(v);
    return 'f:' + String(v);
  }
  if (unhashable(k)) throw new PyError('TypeError', `unhashable type: '${typeName(k)}'`);
  throw new PyError('TypeError', `unhashable type: '${typeName(k)}'`);
}
export class PyDict {
  constructor(pairs) { this.m = new Map(); if (pairs) for (const [k, v] of pairs) this.set(k, v); }
  set(k, v) { const h = hashKey(k); const old = this.m.get(h); if (old) old[1] = v; else this.m.set(h, [k, v]); return this; }
  has(k) { return this.m.has(hashKey(k)); }
  get(k) { const e = this.m.get(hashKey(k)); if (!e) throw new PyError('KeyError', repr(k)); return e[1]; }
  keys() { return [...this.m.values()].map(e => e[0]); }
  values() { return [...this.m.values()].map(e => e[1]); }
  entries() { return [...this.m.values()]; }
  get size() { return this.m.size; }
}
export class PySet {
  constructor(items) { this.m = new Map(); if (items) for (const x of items) this.add(x); }
  add(x) { const h = hashKey(x); if (!this.m.has(h)) this.m.set(h, x); }
  has(x) { return this.m.has(hashKey(x)); }
  get size() { return this.m.size; }
  values() { return [...this.m.values()]; }
}

export function typeName(v) {
  if (v === null || v === undefined) return 'NoneType';
  if (typeof v === 'boolean') return 'bool';
  if (typeof v === 'bigint') return 'int';
  if (v instanceof PyFloat) return 'float';
  if (typeof v === 'number') return Number.isInteger(v) ? 'int' : 'float';
  if (typeof v === 'string') return 'str';
  if (Array.isArray(v)) return 'list';
  if (v instanceof PyDict) return 'dict';
  if (v instanceof PySet) return 'set';
  return typeof v;
}
// type(v) is int
export const isInt = v => typeof v === 'bigint' || (typeof v === 'number' && Number.isInteger(v));
const isNum = v => typeof v === 'boolean' || typeof v === 'bigint' || typeof v === 'number' || v instanceof PyFloat;
const numVal = v => typeof v === 'boolean' ? (v ? 1n : 0n) : v instanceof PyFloat ? v.v : v;
const isFloaty = v => v instanceof PyFloat || (typeof v === 'number' && !Number.isInteger(v));

// a == b
export function eq(a, b) {
  if (isNum(a) && isNum(b)) {
    const x = numVal(a), y = numVal(b);
    // eslint-disable-next-line eqeqeq
    return typeof x === typeof y ? x === y : x == y;
  }
  if (typeof a === 'string' || typeof b === 'string') return a === b;
  if (a === null || b === null) return a === b;
  if (Array.isArray(a) && Array.isArray(b)) return a.length === b.length && a.every((x, i) => eq(x, b[i]));
  if (a instanceof PyDict && b instanceof PyDict) {
    if (a.size !== b.size) return false;
    for (const [k, v] of a.entries()) { if (!b.has(k) || !eq(v, b.get(k))) return false; }
    return true;
  }
  if (a instanceof PySet && b instanceof PySet) return a.size === b.size && a.values().every(x => b.has(x));
  return false;
}
export function truthy(v) {
  if (v === null || v === undefined || v === false) return false;
  if (v === true) return true;
  if (typeof v === 'bigint') return v !== 0n;
  if (v instanceof PyFloat) return v.v !== 0;
  if (typeof v === 'number') return v !== 0;
  if (typeof v === 'string' || Array.isArray(v)) return v.length > 0;
  if (v instanceof PyDict || v instanceof PySet) return v.size > 0;
  return true;
}
function cmpOperands(op, a, b) {
  if (isNum(a) && isNum(b)) return [numVal(a), numVal(b)];
  if (typeof a === 'string' && typeof b === 'string') return [[...a].map(c => c.codePointAt(0)), [...b].map(c => c.codePointAt(0))];
  throw new PyError('TypeError', `'${op}' not supported between instances of '${typeName(a)}' and '${typeName(b)}'`);
}
function cmpSeq(x, y) {
  for (let i = 0; i < Math.min(x.length, y.length); i++) if (x[i] !== y[i]) return x[i] < y[i] ? -1 : 1;
  return x.length - y.length;
}
export function lt(a, b) { const [x, y] = cmpOperands('<', a, b); return Array.isArray(x) ? cmpSeq(x, y) < 0 : x < y; }
export function le(a, b) { const [x, y] = cmpOperands('<=', a, b); return Array.isArray(x) ? cmpSeq(x, y) <= 0 : x <= y; }
export function gt(a, b) { const [x, y] = cmpOperands('>', a, b); return Array.isArray(x) ? cmpSeq(x, y) > 0 : x > y; }
// a + b for numbers (sum())
export function add(a, b) {
  if (!isNum(a) || !isNum(b)) throw new PyError('TypeError', `unsupported operand type(s) for +: '${typeName(a)}' and '${typeName(b)}'`);
  if (isFloaty(a) || isFloaty(b)) return new PyFloat(Number(numVal(a)) + Number(numVal(b)));
  return BigInt(numVal(a)) + BigInt(numVal(b));
}
// obj[key]
export function getitem(obj, key) {
  if (obj instanceof PyDict) return obj.get(key);
  if (Array.isArray(obj) || typeof obj === 'string') {
    if (!(isInt(key) || typeof key === 'boolean')) {
      throw new PyError('TypeError', Array.isArray(obj) ? `list indices must be integers or slices, not ${typeName(key)}` : 'string indices must be integers');
    }
    const items = typeof obj === 'string' ? [...obj] : obj;
    let i = Number(numVal(key)); if (i < 0) i += items.length;
    if (i < 0 || i >= items.length) throw new PyError('IndexError', `${typeName(obj)} index out of range`);
    return items[i];
  }
  throw new PyError('TypeError', `'${typeName(obj)}' object is not subscriptable`);
}
// for x in obj
export function iterate(obj) {
  if (Array.isArray(obj)) return obj;
  if (obj instanceof PyDict) return obj.keys();
  if (obj instanceof PySet) return obj.values();
  if (typeof obj === 'string') return [...obj];
  throw new PyError('TypeError', `'${typeName(obj)}' object is not iterable`);
}
export function len(obj) {
  if (Array.isArray(obj)) return obj.length;
  if (typeof obj === 'string') return [...obj].length;
  if (obj instanceof PyDict || obj instanceof PySet) return obj.size;
  throw new PyError('TypeError', `object of type '${typeName(obj)}' has no len()`);
}
export const pyset = obj => new PySet(iterate(obj));
export function check(cond, message) { if (!cond) throw new PyError('AssertionError', message); }
// re.fullmatch(pattern, s) for patterns without alternation at the top level
export function fullmatch(re, s) {
  if (typeof s !== 'string') throw new PyError('TypeError', `expected string or bytes-like object`);
  return re.test(s);
}
// str.strip() / str.lstrip(chars)
const PY_SPACE = '\t\n\x0b\x0c\r\x1c\x1d\x1e\x1f \x85\xa0                　';
export function strip(s) {
  let a = 0, b = s.length;
  while (a < b && PY_SPACE.includes(s[a])) a++;
  while (b > a && PY_SPACE.includes(s[b - 1])) b--;
  return s.slice(a, b);
}
export function lstrip(s, chars) { let a = 0; while (a < s.length && chars.includes(s[a])) a++; return s.slice(a); }

// repr() of a str / list of str / number
export function repr(v) {
  if (typeof v === 'string') {
    const q = v.includes("'") && !v.includes('"') ? '"' : "'";
    let out = q;
    for (const ch of v) {
      const c = ch.codePointAt(0);
      if (ch === q || ch === '\\') out += '\\' + ch;
      else if (ch === '\t') out += '\\t';
      else if (ch === '\n') out += '\\n';
      else if (ch === '\r') out += '\\r';
      else if (c < 0x20 || c === 0x7f) out += '\\x' + c.toString(16).padStart(2, '0');
      else if (c > 0x7f && /[\p{C}\p{Z}]/u.test(ch)) {
        out += c <= 0xff ? '\\x' + c.toString(16).padStart(2, '0') : c <= 0xffff ? '\\u' + c.toString(16).padStart(4, '0') : '\\U' + c.toString(16).padStart(8, '0');
      } else out += ch;
    }
    return out + q;
  }
  if (Array.isArray(v)) return '[' + v.map(repr).join(', ') + ']';
  if (v === null) return 'None';
  if (v === true) return 'True';
  if (v === false) return 'False';
  if (typeof v === 'bigint' || typeof v === 'number' || v instanceof PyFloat) return isFloaty(v) ? floatRepr(numVal(v)) : String(numVal(v));
  return String(v);
}

// float.__repr__
export function floatRepr(x) {
  if (Number.isNaN(x)) return 'nan';
  if (x === Infinity) return 'inf';
  if (x === -Infinity) return '-inf';
  if (x === 0) return Object.is(x, -0) ? '-0.0' : '0.0';
  const sign = x < 0 ? '-' : '';
  const [mant, e] = Math.abs(x).toExponential().split('e');
  const digits = mant.replace('.', ''), exp = Number(e), decpt = exp + 1;
  if (decpt > -4 && decpt <= 16) {
    if (decpt <= 0) return sign + '0.' + '0'.repeat(-decpt) + digits;
    if (decpt >= digits.length) return sign + digits + '0'.repeat(decpt - digits.length) + '.0';
    return sign + digits.slice(0, decpt) + '.' + digits.slice(decpt);
  }
  const m = digits.length > 1 ? digits[0] + '.' + digits.slice(1) : digits;
  return sign + m + 'e' + (exp < 0 ? '-' : '+') + String(Math.abs(exp)).padStart(2, '0');
}

// json.dumps(v, indent=indent) (ensure_ascii=True, allow_nan=True)
function encodeString(s) {
  let out = '"';
  for (let i = 0; i < s.length; i++) {
    const ch = s[i], c = s.charCodeAt(i);
    if (ch === '"') out += '\\"';
    else if (ch === '\\') out += '\\\\';
    else if (ch === '\n') out += '\\n';
    else if (ch === '\r') out += '\\r';
    else if (ch === '\t') out += '\\t';
    else if (ch === '\b') out += '\\b';
    else if (ch === '\f') out += '\\f';
    else if (c < 0x20 || c > 0x7e) out += '\\u' + c.toString(16).padStart(4, '0');
    else out += ch;
  }
  return out + '"';
}
function encodeFloat(x) {
  if (Number.isNaN(x)) return 'NaN';
  if (x === Infinity) return 'Infinity';
  if (x === -Infinity) return '-Infinity';
  return floatRepr(x);
}
function encodeKey(k) {
  if (typeof k === 'string') return k;
  if (isFloaty(k)) return encodeFloat(numVal(k));
  if (k === true) return 'true';
  if (k === false) return 'false';
  if (k === null) return 'null';
  if (isInt(k)) return String(k);
  throw new PyError('TypeError', `keys must be str, int, float, bool or None, not ${typeName(k)}`);
}
export function dumps(v, indent) {
  const enc = (o, level) => {
    if (o === null || o === undefined) return 'null';
    if (o === true) return 'true';
    if (o === false) return 'false';
    if (typeof o === 'string') return encodeString(o);
    if (typeof o === 'bigint') return String(o);
    if (o instanceof PyFloat) return encodeFloat(o.v);
    if (typeof o === 'number') return Number.isInteger(o) ? String(o) : encodeFloat(o);
    const nl = '\n' + ' '.repeat(indent * (level + 1)), close = '\n' + ' '.repeat(indent * level);
    if (Array.isArray(o)) {
      if (!o.length) return '[]';
      return '[' + nl + o.map(x => enc(x, level + 1)).join(',' + nl) + close + ']';
    }
    let pairs;
    if (o instanceof PyDict) pairs = o.entries();
    else if (typeof o === 'object') pairs = Object.entries(o);
    else throw new PyError('TypeError', `Object of type ${typeName(o)} is not JSON serializable`);
    if (!pairs.length) return '{}';
    return '{' + nl + pairs.map(([k, x]) => encodeString(encodeKey(k)) + ': ' + enc(x, level + 1)).join(',' + nl) + close + '}';
  };
  return enc(v, 0);
}

// json.loads(s) -- CPython 3.10 C scanner
export function loads(s) {
  const fail = (msg, pos) => {
    const cp = [...s.slice(0, pos)].length;
    const before = s.slice(0, pos);
    const line = before.split('\n').length;
    const nl = before.lastIndexOf('\n');
    const col = nl < 0 ? cp + 1 : [...before.slice(nl + 1)].length + 1;
    throw new PyError('json.decoder.JSONDecodeError', `${msg}: line ${line} column ${col} (char ${cp})`);
  };
  const STOP = Symbol('stop');
  let stopAt = 0;
  const stop = i => { stopAt = i; throw STOP; };
  const n = s.length;
  const ws = i => { while (i < n && (s[i] === ' ' || s[i] === '\t' || s[i] === '\n' || s[i] === '\r')) i++; return i; };
  const hex4 = i => { const t = s.slice(i, i + 4); return /^[0-9a-fA-F]{4}$/.test(t) ? parseInt(t, 16) : -1; };
  const str = start => { // start: index after the opening quote
    const begin = start - 1; let out = '', end = start;
    for (;;) {
      let next = end;
      while (next < n) {
        const c = s.charCodeAt(next);
        if (c === 0x22 || c === 0x5c) break;
        if (c <= 0x1f) fail('Invalid control character at', next);
        next++;
      }
      if (next >= n) fail('Unterminated string starting at', begin);
      out += s.slice(end, next);
      if (s[next] === '"') return [out, next + 1];
      next++;
      if (next === n) fail('Unterminated string starting at', begin);
      const e = s[next];
      if (e !== 'u') {
        const map = { '"': '"', '\\': '\\', '/': '/', b: '\b', f: '\f', n: '\n', r: '\r', t: '\t' };
        if (!(e in map)) fail('Invalid \\escape', next - 1);
        out += map[e]; end = next + 1; continue;
      }
      next++; end = next + 4;
      if (end >= n) fail('Invalid \\uXXXX escape', next - 1);
      let c = hex4(next);
      if (c < 0) fail('Invalid \\uXXXX escape', end - 5);
      if (c >= 0xd800 && c <= 0xdbff && end + 6 < n && s[end] === '\\' && s[end + 1] === 'u') {
        const c2 = hex4(end + 2);
        if (c2 < 0) fail('Invalid \\uXXXX escape', end + 1);
        if (c2 >= 0xdc00 && c2 <= 0xdfff) { out += String.fromCharCode(c, c2); end += 6; continue; }
      }
      out += String.fromCharCode(c);
    }
  };
  const NUM = /-?(?:0|[1-9][0-9]*)(\.[0-9]+)?([eE][-+]?[0-9]+)?/y;
  const value = i => {
    if (i >= n) stop(i);
    const c = s[i];
    if (c === '"') return str(i + 1);
    if (c === '{') {
      const obj = new PyDict(); i = ws(i + 1);
      if (i >= n || s[i] !== '}') {
        for (;;) {
          if (i >= n || s[i] !== '"') fail('Expecting property name enclosed in double quotes', i);
          const [key, after] = str(i + 1);
          i = ws(after);
          if (i >= n || s[i] !== ':') fail("Expecting ':' delimiter", i);
          i = ws(i + 1);
          const [val, next] = value(i);
          obj.set(key, val); i = ws(next);
          if (i < n && s[i] === '}') break;
          if (i >= n || s[i] !== ',') fail("Expecting ',' delimiter", i);
          i = ws(i + 1);
        }
      }
      return [obj, i + 1];
    }
    if (c === '[') {
      const arr = []; i = ws(i + 1);
      if (i >= n || s[i] !== ']') {
        for (;;) {
          const [val, next] = value(i);
          arr.push(val); i = ws(next);
          if (i < n && s[i] === ']') break;
          if (i >= n || s[i] !== ',') fail("Expecting ',' delimiter", i);
          i = ws(i + 1);
        }
      }
      return [arr, i + 1];
    }
    if (c === 'n' && i + 3 < n && s.startsWith('null', i)) return [null, i + 4];
    if (c === 't' && i + 3 < n && s.startsWith('true', i)) return [true, i + 4];
    if (c === 'f' && i + 4 < n && s.startsWith('false', i)) return [false, i + 5];
    if (c === 'N' && i + 2 < n && s.startsWith('NaN', i)) return [new PyFloat(NaN), i + 3];
    if (c === 'I' && i + 7 < n && s.startsWith('Infinity', i)) return [new PyFloat(Infinity), i + 8];
    if (c === '-' && i + 8 < n && s.startsWith('-Infinity', i)) return [new PyFloat(-Infinity), i + 9];
    NUM.lastIndex = i;
    const m = NUM.exec(s);
    if (!m) stop(i);
    const end = i + m[0].length;
    if (m[1] !== undefined || m[2] !== undefined) return [new PyFloat(Number(m[0])), end];
    return [BigInt(m[0]), end];
  };
  if (s.charCodeAt(0) === 0xfeff) fail('Unexpected UTF-8 BOM (decode using utf-8-sig)', 0);
  let start = ws(0), result, end;
  try { [result, end] = value(start); } catch (e) { if (e === STOP) fail('Expecting value', stopAt); throw e; }
  end = ws(end);
  if (end !== n) fail('Extra data', end);
  return result;
}

// OSError the way Python words it: "[Errno 2] No such file or directory: 'x'"
const STRERROR = { ENOENT: 'No such file or directory', EACCES: 'Permission denied', EPERM: 'Operation not permitted',
  EISDIR: 'Is a directory', ENOTDIR: 'Not a directory', EEXIST: 'File exists', ELOOP: 'Too many levels of symbolic links',
  ENAMETOOLONG: 'File name too long', EINVAL: 'Invalid argument', EIO: 'Input/output error' };
const OSCLASS = { ENOENT: 'FileNotFoundError', EEXIST: 'FileExistsError', EACCES: 'PermissionError', EPERM: 'PermissionError',
  EISDIR: 'IsADirectoryError', ENOTDIR: 'NotADirectoryError' };
export function osError(err, name) {
  if (!err || typeof err.code !== 'string' || typeof err.errno !== 'number') return err;
  const errno = os.constants.errno[err.code] ?? Math.abs(err.errno);
  const text = `[Errno ${errno}] ${STRERROR[err.code] ?? err.message}` + (name === undefined ? '' : `: ${repr(name)}`);
  return new PyError(OSCLASS[err.code] ?? 'OSError', text);
}
export function withOs(name, fn) { try { return fn(); } catch (e) { throw osError(e, name); } }
export const readBytes = file => withOs(file, () => fs.readFileSync(file));
// bytes.decode('utf-8') (strict, BOM kept)
export function decodeUtf8(buf) {
  try { return new TextDecoder('utf-8', { fatal: true, ignoreBOM: true }).decode(buf); }
  catch { throw new PyError('UnicodeDecodeError', "'utf-8' codec can't decode bytes: invalid utf-8"); }
}
// Path.read_text() under PYTHONUTF8=1: strict UTF-8 with universal newlines
export const readText = file => decodeUtf8(readBytes(file)).replace(/\r\n?/g, '\n');
// Path.write_text(s): text mode writes os.linesep for "\n"
export const writeText = (file, text) => withOs(file, () => fs.writeFileSync(file, process.platform === 'win32' ? text.replace(/\n/g, '\r\n') : text));
export const mkdir = dir => withOs(dir, () => fs.mkdirSync(dir));
export const stat = file => withOs(file, () => fs.statSync(file));

// Run main(); end like an uncaught Python exception would (last traceback line, exit 1).
export function run(main) {
  try { main(); }
  catch (e) {
    const line = e instanceof PyError ? (e.pyMessage === undefined || e.pyMessage === '' ? e.pyType : `${e.pyType}: ${e.pyMessage}`) : `${e && e.name}: ${e && e.message}`;
    process.stderr.write(line + (process.platform === 'win32' ? '\r\n' : '\n'));
    process.exitCode = 1;
  }
}
