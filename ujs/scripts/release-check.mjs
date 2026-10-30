/**
 * Release helpers for release-artifacts.sh; they replace its inline Python.
 *   node release-check.mjs version PACKAGE_JSON   print package.json "version"
 *   node release-check.mjs fingerprint CORE PKG   BUILD.json version == package version; every
 *                                                 artifact present with the recorded sha256 and bytes
 *   node release-check.mjs note STAGE             write STAGE/RELEASE.txt from STAGE/core/BUILD.json
 * Exit 1 with a reason on stderr when a check fails.
 */
import { createHash } from 'node:crypto';
import { readFileSync, statSync, writeFileSync } from 'node:fs';
import { join } from 'node:path';

const fail = (...m) => { console.error(...m); process.exit(1); };
const json = (p) => JSON.parse(readFileSync(p, 'utf8'));
const [mode, a, b] = process.argv.slice(2);

if (mode === 'version' && a) {
  console.log(json(a).version);
} else if (mode === 'fingerprint' && a && b) {
  const build = json(join(a, 'BUILD.json'));
  const pkg = json(b);
  if (build.version !== pkg.version) fail('version mismatch', build.version, pkg.version);
  for (const [name, meta] of Object.entries(build.artifacts)) {
    const p = join(a, name);
    let st;
    try { st = statSync(p); } catch { fail('missing', p); }
    if (!st.isFile()) fail('missing', p);
    const h = createHash('sha256').update(readFileSync(p)).digest('hex');
    if (h !== meta.sha256) fail('sha mismatch', name, h, meta.sha256);
    if (st.size !== meta.bytes) fail('size mismatch', name);
  }
  console.log('fingerprint OK', build.version, build.git);
} else if (mode === 'note' && a) {
  const build = json(join(a, 'core', 'BUILD.json'));
  const note = {
    how: 'Upload this zip as a GitHub Release asset for tag/version matching package.json.',
    verify: 'sha256 of core/ujs_full.wasm and core/compiler.gen.js must match core/BUILD.json',
    boot: "import { bootRuntime, wasm_run } from './core/wasm_run.js'; await bootRuntime(new URL('./core/ujs_full.wasm', import.meta.url));",
    build,
  };
  writeFileSync(join(a, 'RELEASE.txt'),
    `UJS ${build.version} artifacts\n` +
    `git=${build.git} created=${build.created}\n` +
    'Core: core/ujs_full.wasm + core/compiler.gen.js (+ hand-written ESM).\n' +
    'Do not commit this zip to git; attach to the GitHub Release.\n' +
    '\n' + JSON.stringify(note, null, 2) + '\n');
} else {
  fail('usage: release-check.mjs version PKG | fingerprint CORE PKG | note STAGE');
}
