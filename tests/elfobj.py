#!/usr/bin/env python3
"""elfobj gate (R16-7): `-c -b lnx/ARCH` writes a relocatable ELF that system linkers accept.

    python3 tests/elfobj.py UA [--link] [--run-arm64-lima VM]

For each probe and both Linux architectures:
  1. the object parses as ET_REL with the eight sections of docs/toolchain.md §4,
     every relocation type is in the allowed set, `_start` is the only global;
  2. invariant: the object's .text equals the image's text outside the relocated
     fields, and .data equals the image's stored data (the .o is pinned to the
     closure-checked image bytes);
  3. --link: ld.lld (host) or ld (Linux host) links the object alone into a program;
  4. --run-arm64-lima VM: the lld- and GNU-ld-linked arm64 programs run inside
     the Lima VM and print exactly what the `-b lnx/arm64` image prints.
Skips are named; STRICT=1 turns them into failures.  Every subprocess is bounded.
"""
import os
import pathlib
import shutil
import struct
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
PROBES = ['examples/hello.c', 'examples/fact.c', 'examples/fib.c', 'examples/ptr.c',
          'examples/struct.c', 'examples/switch.c', 'examples/indirect6.c']
ALLOWED = {'x86_64': {2}, 'arm64': {275, 277}}          # PC32; ADR_PREL_PG_HI21, ADD_ABS_LO12_NC
MACHINE = {'x86_64': 62, 'arm64': 183}
LLD = ['/opt/homebrew/opt/lld@21/bin/ld.lld', '/opt/homebrew/opt/lld/bin/ld.lld', 'ld.lld']
EMUL = {'x86_64': 'elf_x86_64', 'arm64': 'aarch64elf'}
# The kernel32 names a COFF object imports: the one list the image writer owns
# (a private copy went stale when 1828ea47 grew it 14 -> 22 names).
sys.path.insert(0, str(ROOT))
from unisa.image.pe import IMPORTS as WINIMPORTS


def run(cmd, timeout=30, **kw):
    return subprocess.run([str(c) for c in cmd], capture_output=True, timeout=timeout, **kw)


def parse(blob):
    assert blob[:4] == b'\x7fELF' and blob[4] == 2 and blob[5] == 1, 'not ELF64 LE'
    e_type, e_machine = struct.unpack_from('<HH', blob, 16)
    e_shoff, = struct.unpack_from('<Q', blob, 40)
    e_shentsize, e_shnum, e_shstrndx = struct.unpack_from('<HHH', blob, 58)
    secs = []
    for i in range(e_shnum):
        f = struct.unpack_from('<IIQQQQIIQQ', blob, e_shoff + i * e_shentsize)
        secs.append(dict(name=f[0], type=f[1], flags=f[2], off=f[4], size=f[5], link=f[6], info=f[7], align=f[8], entsize=f[9]))
    sh = secs[e_shstrndx]
    names = blob[sh['off']:sh['off'] + sh['size']]
    for s in secs:
        s['sname'] = names[s['name']:names.index(b'\0', s['name'])].decode()
    by = {s['sname']: s for s in secs}
    return e_type, e_machine, secs, by


def check_object(blob, arch, image):
    e_type, e_machine, secs, by = parse(blob)
    assert e_type == 1, e_type
    assert e_machine == MACHINE[arch], e_machine
    assert [s['sname'] for s in secs] == ['', '.text', '.data', '.bss', '.rela.text', '.symtab', '.strtab', '.shstrtab'], [s['sname'] for s in secs]
    assert by['.text']['type'] == 1 and by['.text']['flags'] == 6
    assert by['.data']['type'] == 1 and by['.data']['flags'] == 3
    assert by['.bss']['type'] == 8 and by['.bss']['flags'] == 3
    r = by['.rela.text']
    assert r['type'] == 4 and r['link'] == 5 and r['info'] == 1 and r['entsize'] == 24 and r['size'] % 24 == 0
    relocs = [struct.unpack_from('<QQq', blob, r['off'] + k * 24) for k in range(r['size'] // 24)]
    assert relocs, 'no relocations'
    text = bytearray(blob[by['.text']['off']:by['.text']['off'] + by['.text']['size']])
    for off, info, addend in relocs:
        typ, sym = info & 0xffffffff, info >> 32
        assert typ in ALLOWED[arch], (arch, typ)
        assert sym in (1, 2, 3), sym                     # a section symbol
        assert 0 <= off <= len(text) - 4, off
        if sym == 1: assert arch == 'arm64' and 0 <= addend < len(text), addend
        if sym == 2: assert -4 <= addend < by['.data']['size'], addend
        if sym == 3: assert -4 <= addend < by['.bss']['size'], addend
    st = by['.symtab']
    syms = [struct.unpack_from('<IBBHQQ', blob, st['off'] + k * 24) for k in range(st['size'] // 24)]
    strtab = blob[by['.strtab']['off']:by['.strtab']['off'] + by['.strtab']['size']]
    name = lambda n: strtab[n:strtab.index(b'\0', n)].decode()
    globs = [s for s in syms if s[1] >> 4 == 1]
    assert len(globs) == 1 and name(globs[0][0]) == '_start' and globs[0][1] & 15 == 2 and globs[0][3] == 1, globs
    assert st['info'] == len(syms) - 1, (st['info'], len(syms))
    assert [s[3] for s in syms[1:4]] == [1, 2, 3] and all(s[1] & 15 == 3 for s in syms[1:4])
    assert all(name(s[0]) for s in syms[4:]), 'unnamed local symbol'
    # invariant against the image: text outside relocated fields, stored data
    itext_off = 176
    itext = bytearray(image[itext_off:itext_off + len(text)])
    assert len(itext) == len(text)
    for off, info, addend in relocs:
        text[off:off + 4] = b'\0\0\0\0'; itext[off:off + 4] = b'\0\0\0\0'
    assert text == itext, 'object .text differs from the image text outside relocations'
    # the image's second program header gives the data file offset and filesz
    p_offset, = struct.unpack_from('<Q', image, 64 + 56 + 8)
    p_filesz, = struct.unpack_from('<Q', image, 64 + 56 + 32)
    p_memsz, = struct.unpack_from('<Q', image, 64 + 56 + 40)
    # .data ends at a blob boundary (>= the image's last nonzero byte): its
    # prefix is the image's stored data and the rest is zeros
    d = by['.data']
    assert d['size'] >= p_filesz, (d['size'], p_filesz)
    obj_data = blob[d['off']:d['off'] + d['size']]
    assert obj_data[:p_filesz] == image[p_offset:p_offset + p_filesz], 'object .data differs from the image data'
    assert not any(obj_data[p_filesz:]), 'object .data has nonzero bytes past the image data'
    assert d['size'] + by['.bss']['size'] == p_memsz, (d['size'], by['.bss']['size'], p_memsz)
    return len(relocs), len(syms)


def main(argv):
    if len(argv) < 2:
        print(__doc__); return 2
    ua = argv[1]
    want_link = '--link' in argv
    vm = argv[argv.index('--run-arm64-lima') + 1] if '--run-arm64-lima' in argv else None
    winvm = argv[argv.index('--run-windows') + 1] if '--run-windows' in argv else None
    strict = os.environ.get('STRICT') == '1'
    tmp = pathlib.Path(os.environ.get('TMPDIR', '/tmp')) / ('elfobj-%d' % os.getpid())
    tmp.mkdir()
    skips, nrel, nobj = [], 0, 0
    lld = next((p for p in LLD if shutil.which(p) or pathlib.Path(p).is_file()), None)
    linked = {}
    try:
        for probe in PROBES:
            stem = pathlib.Path(probe).stem
            for arch in ('x86_64', 'arm64'):
                obj = tmp / ('%s.%s.o' % (stem, arch)); img = tmp / ('%s.%s.img' % (stem, arch))
                r = run([ua, ROOT / probe, '-c', '-b', 'lnx/' + arch, '-o', obj], cwd=ROOT)
                assert r.returncode == 0 and obj.is_file(), (probe, arch, r.stderr[-300:])
                r = run([ua, ROOT / probe, '-b', 'lnx/' + arch, '-o', img], cwd=ROOT)
                assert r.returncode == 0, (probe, arch, r.stderr[-300:])
                k, n = check_object(obj.read_bytes(), arch, img.read_bytes())
                nrel += k; nobj += 1
                if want_link:
                    if lld:
                        exe = tmp / ('%s.%s.lld' % (stem, arch))
                        r = run([lld, '-m', EMUL[arch], '-o', exe, obj])
                        assert r.returncode == 0, ('ld.lld', probe, arch, r.stderr[-300:])
                        linked[(stem, arch)] = exe
                    elif sys.platform.startswith('linux') and shutil.which('ld'):
                        exe = tmp / ('%s.%s.gnu' % (stem, arch))
                        r = run(['ld', '-m', EMUL[arch], '-o', exe, obj])
                        assert r.returncode == 0, ('ld', probe, arch, r.stderr[-300:])
                        linked[(stem, arch)] = exe
                    else:
                        skips.append('link %s %s: no ld.lld and no GNU ld' % (stem, arch))
        # Mach-O objects (R17-1): MH_OBJECT, ld64 links them, the program prints what the image prints
        if sys.platform == 'darwin' and shutil.which('ld') and shutil.which('xcrun'):
            sdk = run(['xcrun', '--show-sdk-path']).stdout.decode().strip()
            for probe in PROBES:
                stem = pathlib.Path(probe).stem
                for arch in ('arm64', 'x86_64'):
                    obj = tmp / ('%s.osx.%s.o' % (stem, arch)); img = tmp / ('%s.osx.%s.img' % (stem, arch)); exe = tmp / ('%s.osx.%s.exe' % (stem, arch))
                    r = run([ua, ROOT / probe, '-c', '-b', 'osx/' + arch, '-o', obj], cwd=ROOT)
                    assert r.returncode == 0, ('macho -c', probe, arch, r.stderr[-300:])
                    blob = obj.read_bytes()
                    assert blob[:4] == b'\xcf\xfa\xed\xfe' and struct.unpack_from('<I', blob, 12)[0] == 1, 'not MH_OBJECT'
                    r = run(['ld', '-arch', arch, '-o', exe, obj, '-e', '_start', '-lSystem', '-syslibroot', sdk])
                    assert r.returncode == 0, ('ld64', probe, arch, r.stderr[-300:])
                    r = run([ua, ROOT / probe, '-b', 'osx/' + arch, '-o', img], cwd=ROOT); assert r.returncode == 0
                    img.chmod(0o755); run(['codesign', '-f', '-s', '-', img])
                    a = run([exe], timeout=10); b = run([img], timeout=10)
                    assert (a.returncode, a.stdout) == (b.returncode, b.stdout), ('macho run', probe, arch, a.stdout[-200:], b.stdout[-200:])
                    nobj += 1; linked[(stem, 'osx-' + arch)] = exe
            print('elfobj  Mach-O objects for osx/arm64 and osx/x86_64 link with ld64 and print what the images print')
        else:
            skips.append('Mach-O: not on macOS (needs ld64 and the SDK)')
        # COFF objects (R17-1): lld-link with a kernel32 import library made by
        # llvm-dlltool; the programs run in the Windows VM when one is named
        lldlink = next((x for x in ('/opt/homebrew/opt/lld@21/bin/lld-link', '/opt/homebrew/opt/lld/bin/lld-link') if pathlib.Path(x).is_file()), shutil.which('lld-link'))
        dlltool = next((x for x in ('/opt/homebrew/opt/llvm/bin/llvm-dlltool',) if pathlib.Path(x).is_file()), shutil.which('llvm-dlltool'))
        winexe = {}
        if lldlink and dlltool:
            (tmp / 'k32.def').write_text('LIBRARY kernel32.dll\nEXPORTS\n' + ''.join('  %s\n' % n for n in WINIMPORTS))
            for arch, m, mach in (('x86_64', 'i386:x86-64', 'x64'), ('arm64', 'arm64', 'arm64')):
                lib = tmp / ('k32_%s.lib' % arch)
                r = run([dlltool, '-m', m, '-d', tmp / 'k32.def', '-l', lib]); assert r.returncode == 0, r.stderr
                for probe in PROBES:
                    stem = pathlib.Path(probe).stem
                    obj = tmp / ('%s.win.%s.obj' % (stem, arch)); exe = tmp / ('%s.win.%s.exe' % (stem, arch)); img = tmp / ('%s.win.%s.img.exe' % (stem, arch))
                    r = run([ua, ROOT / probe, '-c', '-b', 'win/' + arch, '-o', obj], cwd=ROOT)
                    assert r.returncode == 0, ('coff -c', probe, arch, r.stderr[-300:])
                    blob = obj.read_bytes()
                    assert struct.unpack_from('<HH', blob, 0) == ((0x8664 if arch == 'x86_64' else 0xAA64), 3), 'not a 3-section COFF object'
                    r = run([lldlink, '/entry:_start', '/subsystem:console', '/nodefaultlib', '/machine:' + mach, '/out:%s' % exe, obj, lib])
                    assert r.returncode == 0, ('lld-link', probe, arch, r.stdout[-300:], r.stderr[-300:])
                    r = run([ua, ROOT / probe, '-b', 'win/' + arch, '-o', img], cwd=ROOT); assert r.returncode == 0
                    winexe[(stem, arch)] = (exe, img); nobj += 1; linked[(stem, 'win-' + arch)] = exe
            print('elfobj  COFF objects for win/x86_64 and win/arm64 link with lld-link against a kernel32 import library')
            if winvm:
                utm = '/Applications/UTM.app/Contents/MacOS/utmctl'
                st = run([utm, 'status', winvm], timeout=20)
                # utmctl exec returns 0 even when the guest agent is not up (it prints
                # OSStatus -2700); an IP address from the agent is the readiness signal
                if b'started' not in st.stdout or not run([utm, 'ip-address', winvm], timeout=20).stdout.strip():
                    skips.append('run COFF programs: Windows VM %s not answering' % winvm)
                else:
                    t = str(os.getpid())
                    bat = ['@echo off']
                    runset = {k: v for k, v in winexe.items() if k[0] in ('hello', 'fact', 'ptr')}   # bounded: pushes cost seconds each
                    for (stem, arch), (exe, img) in sorted(runset.items()):
                        for kind, f in (('obj', exe), ('img', img)):
                            g = 'C:\\u\\o%s_%s_%s_%s.exe' % (t, stem, arch, kind)
                            run([utm, 'file', 'push', winvm, g], input=f.read_bytes(), timeout=30)
                            bat.append('echo === %s %s %s >> C:\\u\\o%s.txt' % (stem, arch, kind, t))
                            bat.append('%s >> C:\\u\\o%s.txt 2>&1' % (g, t))
                            bat.append('echo RC=%%errorlevel%% >> C:\\u\\o%s.txt' % t)
                    bat.append('echo done > C:\\u\\o%s.done' % t)
                    run([utm, 'file', 'push', winvm, 'C:\\u\\o%s.bat' % t], input=('\r\n'.join(bat) + '\r\n').encode(), timeout=30)
                    r = run([utm, 'exec', winvm, '--cmd', 'cmd.exe', '--', '/c', 'C:\\u\\o%s.bat' % t], timeout=40)
                    assert r.returncode == 0, 'guest exec refused'
                    import time
                    for _ in range(20):                     # the batch runs on after exec returns
                        if b'done' in run([utm, 'file', 'pull', winvm, 'C:\\u\\o%s.done' % t], timeout=15).stdout: break
                        time.sleep(1.5)
                    out = run([utm, 'file', 'pull', winvm, 'C:\\u\\o%s.txt' % t], timeout=30).stdout.decode(errors='replace').replace('\r', '')
                    blocks = {}
                    for chunk in out.split('=== ')[1:]:
                        head, _, body = chunk.partition('\n'); blocks[tuple(head.split()[:3])] = body.strip()
                    for (stem, arch) in runset:
                        a = blocks.get((stem, arch, 'obj')); b = blocks.get((stem, arch, 'img'))
                        assert a is not None and a == b, ('windows run', stem, arch, (a or '')[-200:], (b or '')[-200:])
                    print('elfobj  COFF programs run in %s (win/x86_64 emulated, win/arm64 native) and print what the images print' % winvm)
        else:
            skips.append('COFF: no lld-link or llvm-dlltool')
        # R17-9 interop (a): our whole-program object reads data symbols a C compiler defined
        bsrc = tmp / 'banner.c'; bsrc.write_text('const char banner[] = "banner from cc";\nint answer = 42;\n')
        usrc = tmp / 'usebanner.c'; usrc.write_text('#include <stdio.h>\nextern const char banner[];\nextern int answer;\nint main(void){ printf("%s %d\\n", banner, answer); return 0; }\n')
        want = b'banner from cc 42\n'
        if sys.platform == 'darwin' and shutil.which('ld'):
            sdk = run(['xcrun', '--show-sdk-path']).stdout.decode().strip()
            for arch in ('arm64', 'x86_64'):
                bo = tmp / ('banner_%s.o' % arch); uo = tmp / ('use_%s.o' % arch); ex = tmp / ('use_%s' % arch)
                assert run(['cc', '-arch', arch, '-c', '-o', bo, bsrc]).returncode == 0
                r = run([ua, usrc, '-c', '-b', 'osx/' + arch, '-o', uo]); assert r.returncode == 0, r.stderr
                r = run(['ld', '-arch', arch, '-o', ex, uo, bo, '-e', '_start', '-lSystem', '-syslibroot', sdk]); assert r.returncode == 0, r.stderr
                assert run([ex]).stdout == want, ('interop macho', arch)
            print('elfobj  interop (a): our Mach-O objects read cc-defined data symbols (osx/arm64, osx/x86_64)')
        if vm and shutil.which('limactl'):
            uo = tmp / 'use_lnx.o'
            r = run([ua, usrc, '-c', '-b', 'lnx/arm64', '-o', uo]); assert r.returncode == 0, r.stderr
            tar = run(['tar', '-C', tmp, '-cf', '-', 'banner.c', 'use_lnx.o']).stdout
            r = run(['limactl', 'shell', vm, '--', 'sh', '-c', 'rm -rf /tmp/ia && mkdir /tmp/ia && cd /tmp/ia && tar xf - && gcc -c -o banner.o banner.c && ld -o prog use_lnx.o banner.o && ./prog'], input=tar, timeout=40)
            assert r.stdout == want, ('interop elf', r.stdout, r.stderr[-300:])
            print('elfobj  interop (a): our ELF object reads gcc-defined data symbols, linked by GNU ld in %s' % vm)
        # the product's Linux object route (cdx 7f72206) writes the reference's bytes
        prod = os.environ.get('OBJ_PRODUCT')
        if prod:
            same = 0
            for probe in PROBES:
                for arch in ('x86_64', 'arm64'):
                    for unit in ([], ['-funit']):
                        a = tmp / 'pr.o'; b = tmp / 'rf.o'
                        r1 = run(['sh', prod, ROOT / probe, '-c', '-b', 'lnx/' + arch] + unit + ['-o', a], cwd=ROOT)
                        r2 = run([ua, ROOT / probe, '-c', '-b', 'lnx/' + arch] + unit + ['-o', b], cwd=ROOT)
                        assert r1.returncode == 0 and r2.returncode == 0, ('product object', probe, arch, unit, r1.stderr[-200:])
                        assert a.read_bytes() == b.read_bytes(), ('product object bytes differ', probe, arch, unit)
                        same += 1
            r = run(['sh', prod, ROOT / 'examples/hello.c', '-c', '-b', 'osx/arm64', '-o', tmp / 'x.o'], cwd=ROOT)
            assert r.returncode != 0 and b'not on the product route' in r.stderr, 'product osx object should be refused by name'
            print('elfobj  product: %d Linux objects (whole and unit) byte-identical to the reference; osx/win refused by name' % same)
        # bare -c is still the tape
        r = run([ua, ROOT / 'examples/hello.c', '-c', '-o', tmp / 'bare.tape'], cwd=ROOT)
        assert r.returncode == 0 and (tmp / 'bare.tape').read_bytes().startswith(b'_start:'), 'bare -c no longer writes the tape'

        if vm:
            if not shutil.which('limactl'):
                skips.append('run arm64: no limactl')
            else:
                st = run(['limactl', 'list', vm, '--format', '{{.Status}}'], timeout=20)
                if st.returncode != 0 or st.stdout.strip() != b'Running':
                    skips.append('run arm64: Lima VM %s not running' % vm)
                else:
                    files = ['hello.arm64.o', 'hello.arm64.img', 'fact.arm64.o', 'fact.arm64.img']
                    if ('hello', 'arm64') in linked: files += [linked[('hello', 'arm64')].name]
                    tar = run(['tar', '-C', tmp, '-cf', '-'] + files).stdout
                    script = ('rm -rf /tmp/elfobj && mkdir /tmp/elfobj && cd /tmp/elfobj && tar xf - && '
                              'for s in hello fact; do ld -o $s.gnu $s.arm64.o && chmod +x $s.arm64.img && '
                              './$s.gnu > $s.gnu.out; echo "$s gnu rc=$?"; ./$s.arm64.img > $s.img.out; echo "$s img rc=$?"; '
                              'cmp $s.gnu.out $s.img.out && echo "$s SAME"; done; '
                              'if [ -f hello.arm64.lld ]; then chmod +x hello.arm64.lld; ./hello.arm64.lld > lld.out; cmp lld.out hello.img.out && echo "lld SAME"; fi')
                    r = run(['limactl', 'shell', vm, '--', 'sh', '-c', script], input=tar, timeout=50)
                    out = r.stdout.decode(errors='replace')
                    assert 'hello SAME' in out and 'fact SAME' in out and 'rc=1' not in out, out + r.stderr.decode(errors='replace')[-300:]
                    if ('hello', 'arm64') in linked: assert 'lld SAME' in out, out
                    print('elfobj  arm64 programs linked by GNU ld%s run in %s and print what the image prints' % (' and ld.lld' if ('hello', 'arm64') in linked else '', vm))
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    for s in skips: print('elfobj  SKIP ' + s)
    print('elfobj  objects %d   relocations %d   linked %d   skipped %d' % (nobj, nrel, len(linked), len(skips)))
    return 1 if (strict and skips) else 0


if __name__ == '__main__':
    try:
        sys.exit(main(sys.argv))
    except AssertionError as exc:
        print('elfobj: FAIL', exc, file=sys.stderr)
        sys.exit(1)
