#!/usr/bin/env python3
"""Independent generic PE exports resource probe; no compiler semantic rules.
Host executes synthetic export enumeration; SDK cross builds are compile evidence.
"""
import argparse, json, pathlib, shutil, subprocess, tempfile
ROOT = pathlib.Path(__file__).resolve().parents[1]
def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--cross', action='store_true')
    args = ap.parse_args()
    records = []
    with tempfile.TemporaryDirectory(prefix='library-winimports-') as t:
        for sanitizer in ('', 'address,undefined'):
            out = str(pathlib.Path(t) / ('probe' + str(len(records))))
            cmd = ['cc', '-std=c11', '-Wall', '-Wextra', '-Werror']
            if sanitizer:
                cmd += ['-fsanitize=' + sanitizer]
            cmd += [str(ROOT / 'tests/librarywinimportscheck.c'), '-o', out]
            subprocess.run(cmd, check=True, timeout=20)
            run = subprocess.run([out], check=True, timeout=20, capture_output=True, text=True)
            records.append({'mode': sanitizer or 'native', 'result': run.stdout.strip()})
        if args.cross:
            zig = shutil.which('zig')
            if not zig:
                raise RuntimeError('SDK cross check requires zig')
            wrapper = pathlib.Path(t) / 'sdk.c'
            wrapper.write_text('#include <stdint.h>\n#include <windows.h>\ntypedef struct { const unsigned char *name; int n; const unsigned char *data; int len; } ResourceInput;\n#include "' + str(ROOT / 'exec/c/librarywinimports.h') + '"\n_Static_assert(sizeof(long)==4,"LLP64");\n_Static_assert(sizeof(uintptr_t)==8,"address width");\nint probe(void){LibraryWinImports x={0,0};int rc=library_winimports_init(&x);library_winimports_free(&x);return rc;}\n')
            for target in ('x86_64-windows-gnu', 'aarch64-windows-gnu'):
                subprocess.run([zig, 'cc', '-target', target, '-std=c11', '-Wall', '-Wextra', '-Werror', '-c', str(wrapper), '-o', str(pathlib.Path(t) / (target + '.o'))], check=True, timeout=45)
                records.append({'mode': target, 'result': 'real SDK COFF compile; not executed'})
    print(json.dumps({'records': records}, indent=2))
if __name__ == '__main__':
    main()
