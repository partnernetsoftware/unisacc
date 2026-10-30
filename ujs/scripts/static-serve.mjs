/**
 * Minimal local static server for uxe-gate.sh, which used `python3 -m http.server` before.
 *   node static-serve.mjs PORT DIR
 * Binds 127.0.0.1 only. GET/HEAD of files under DIR; a directory serves its index.html.
 * Paths that leave DIR are refused.
 */
import { createServer } from 'node:http';
import { readFile, stat } from 'node:fs/promises';
import { extname, join, resolve, sep } from 'node:path';

const [port, dir] = process.argv.slice(2);
if (!port || !dir) { console.error('usage: static-serve.mjs PORT DIR'); process.exit(2); }
const root = resolve(dir);
const types = {
  '.html': 'text/html; charset=utf-8', '.js': 'text/javascript; charset=utf-8',
  '.mjs': 'text/javascript; charset=utf-8', '.css': 'text/css; charset=utf-8',
  '.json': 'application/json', '.wasm': 'application/wasm', '.svg': 'image/svg+xml',
  '.png': 'image/png', '.txt': 'text/plain; charset=utf-8', '.md': 'text/markdown; charset=utf-8',
};

createServer(async (req, res) => {
  if (req.method !== 'GET' && req.method !== 'HEAD') { res.writeHead(405).end(); return; }
  let p = resolve(root, '.' + decodeURIComponent(new URL(req.url, 'http://x').pathname));
  if (p !== root && !p.startsWith(root + sep)) { res.writeHead(403).end(); return; }
  try {
    if ((await stat(p)).isDirectory()) p = join(p, 'index.html');
    const body = await readFile(p);
    res.writeHead(200, { 'content-type': types[extname(p)] || 'application/octet-stream', 'content-length': body.length });
    res.end(req.method === 'HEAD' ? undefined : body);
  } catch {
    res.writeHead(404).end();
  }
}).listen(Number(port), '127.0.0.1');
