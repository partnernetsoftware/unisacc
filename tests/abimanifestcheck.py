#!/usr/bin/env python3
"""abimanifestcheck.py --check | --selftest -- tests/abi.manifest is what the sources say today.

--check     derive the ABI contract from the sources (tests/abimanifest.py) and compare it with the
            committed tests/abi.manifest.  Any difference fails and is printed entry by entry as
            added / removed / changed.  An ABI change is then a reviewed, visible manifest diff
            (`sh ./unisacc.com -run tests/abimanifestcheck.cx -- --write tests/abi.manifest`), not a side effect of new product bytes.
--selftest  copy the generator's inputs to a scratch tree, break the contract one way at a time,
            and require every break to be reported.

Reads sources only; never builds or runs the product.  Exit 0 same, 1 different or underivable.
"""
import pathlib, shutil, sys, tempfile

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import abimanifest
from abimanifest import ROOT, MANIFEST, Underived, derive, parse, render


def diff(committed, derived):
    """Readable lines: what the sources now say against what the manifest records."""
    old = {(s, k): v for s, k, v in committed}
    new = {(s, k): v for s, k, v in derived}
    lines = []
    for key in sorted(set(old) | set(new)):
        label = '%s %s' % key
        if key not in old:
            lines.append('  added    %s = %s' % (label, new[key]))
        elif key not in new:
            lines.append('  removed  %s (was %s)' % (label, old[key]))
        elif old[key] != new[key]:
            lines.append('  changed  %s\n             manifest: %s\n             sources:  %s' % (label, old[key], new[key]))
    return lines


def check(root=ROOT):
    """(lines, bad) for the tree at root."""
    try:
        derived = derive(root)[0]
    except Underived as error:
        return ['  FAIL cannot derive the contract: %s' % error], True
    try:
        text = (pathlib.Path(root) / MANIFEST).read_bytes().decode('utf-8')
    except FileNotFoundError:
        return ['  FAIL no committed manifest: ' + MANIFEST], True
    try:
        lines = diff(parse(text), derived)
    except ValueError:
        return ['  FAIL %s is not section<tab>key<tab>value' % MANIFEST], True
    if not lines and text != render(derived):
        lines = ['  changed  formatting or order of %s (regenerate it)' % MANIFEST]
    if not derived:
        lines.append('  FAIL nothing derived')
    return lines, bool(lines)


def edit(root, rel, old, new, count=1):
    path = pathlib.Path(root) / rel
    text = path.read_text()
    assert text.count(old) >= 1 and (count is None or text.count(old) == count), '%s: fixture text %r occurs %d times' % (rel, old, text.count(old))
    path.write_text(text.replace(old, new))


def selftest():
    assert abimanifest.initializer('int a[] = { f(a,(b)), (int[]){1,2}[0], "x,y", 4 };', 'int a[]') == ['f(a,(b))', '(int[]){1,2}[0]', '"x,y"', '4']
    code = 'static int probe(void) { /* } */ const char *s="}"; // }\nreturn 0; }\n'
    assert abimanifest.function(code, 'probe')[1].endswith('return 0; ')
    for malformed in ('{ f(a,(b) }', '{ /* unterminated'):
        try:
            abimanifest.balanced_end(abimanifest.lexical_mask(malformed, True), 0)
        except Underived:
            pass
        else:
            raise AssertionError('malformed scanner input must fail')
    arms = abimanifest.platform_arms('#ifdef _WIN32\nif (x) return 0;\n#if A\nx();\n#else\ny();\n#endif\n#else\nif (y) return 1;\n#endif\n', 'probe')
    assert arms[1][1].startswith('if (y)'), 'nested else must not become POSIX arm'
    entries, inputs = derive(ROOT)
    with tempfile.TemporaryDirectory(prefix='abimanifest-') as tmp:
        clean = pathlib.Path(tmp) / 'clean'
        for rel in inputs:
            (clean / rel).parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(ROOT / rel, clean / rel)
        (clean / MANIFEST).parent.mkdir(parents=True, exist_ok=True)
        (clean / MANIFEST).write_text(render(entries))
        assert derive(clean)[0] == entries, 'the scratch copy of the inputs derives a different contract'
        assert check(clean) == ([], False), 'an unchanged tree must pass'
        table = 'exec/lower/codeprivate-result.tsv'
        names = '"\\0process/argc","\\0process/argv"'
        cases = (
            ('renamed export', 'removed  export us_sym_typed',
             lambda r: (edit(r, 'exec/c/libunisacc.h', 'us_sym_typed(', 'us_symbol_typed('),
                        edit(r, 'exec/c/libunisacc.c', 'API void *us_sym_typed(', 'API void *us_symbol_typed('))),
            ('export renamed in the header only', 'cannot derive',
             lambda r: edit(r, 'exec/c/libunisacc.h', 'us_sym_typed(', 'us_symbol_typed(')),
            ('changed export parameter type', 'changed  export us_compile',
             lambda r: (edit(r, 'exec/c/libunisacc.h', 'const char *target, int optimisation', 'const char *target, long optimisation'),
                        edit(r, 'exec/c/libunisacc.c', 'us_compile(us_context *c,const char *target,int level)', 'us_compile(us_context *c,const char *target,long level)'))),
            ('export definition without a declaration', 'cannot derive',
             lambda r: edit(r, 'exec/c/libunisacc.c', 'API int us_relocate(', 'API int us_extra(us_context *c) { return 0; }\nAPI int us_relocate(')),
            ('reordered struct field', 'changed  struct us_signature',
             lambda r: edit(r, 'exec/c/libunisacc.h', 'size_t count;\n    unsigned variadic;', 'unsigned variadic;\n    size_t count;')),
            ('reordered resource name', 'changed  callback 00',
             lambda r: edit(r, 'exec/c/libunisacc.c', names, '"\\0process/argv","\\0process/argc"')),
            ('reordered resource names of different length', 'cannot derive',
             lambda r: edit(r, 'exec/c/libunisacc.c', '"\\0library/mmap","\\0library/munmap"', '"\\0library/munmap","\\0library/mmap"')),
            ('resource length not matching its name', 'cannot derive',
             lambda r: edit(r, 'exec/c/libunisacc.c', 'int lengths[]={13,13,12,', 'int lengths[]={13,13,13,')),
            ('changed table hash', 'changed  table ' + table,
             lambda r: (pathlib.Path(r) / table).write_bytes((pathlib.Path(r) / table).read_bytes() + b'#\n')),
            ('removed table', 'removed  table ' + table,
             lambda r: (pathlib.Path(r) / table).unlink()),
            ('MAP_FIXED no longer rejected', 'changed  guard library_mmap/posix',
             lambda r: edit(r, 'exec/c/libunisacc.c', 'if (!active || length<=0 || (flags&MAP_FIXED)) return -EINVAL;', 'if (!active || length<=0) return -EINVAL;')),
            ('munmap range test changed', 'changed  callback-fn library_munmap',
             lambda r: edit(r, 'exec/c/libunisacc.c', 'if (begin>=lo && end<=hi) break;', 'if (begin>=lo) break;')),
            ('loader import in a unisacc-built Linux compiler', 'added    dlslot #if defined(__linux__) | 0',
             lambda r: edit(r, 'src/host_dl.h', '#if defined(__linux__) && !defined(__UNISA__)\n    if (i == 0)', '#if defined(__linux__)\n    if (i == 0)')),
            ('new syscall gate name', 'added    gate __openat',
             lambda r: edit(r, 'src/front_parse.c', 'isname(t, "__getdents64", 12) || isname(t, "__open", 6) ||', 'isname(t, "__getdents64", 12) || isname(t, "__openat", 8) || isname(t, "__open", 6) ||')),
            ('changed ELF interpreter path', 'changed  image-path 00',
             lambda r: edit(r, 'src/back_image.c', '"/lib/ld-linux-aarch64.so.1"', '"/lib/ld-musl-aarch64.so.1"')),
            ('hand-edited manifest', 'changed  export us_new',
             lambda r: edit(r, MANIFEST, 'export\tus_new\tus_context*(const char*)', 'export\tus_new\tus_context*(void)')),
            ('missing manifest', 'no committed manifest',
             lambda r: (pathlib.Path(r) / MANIFEST).unlink()),
            ('elif loader branch', 'added    dlslot #elif ALT after',
             lambda r: edit(r, 'src/host_dl.h', '#if defined(__linux__) && !defined(__UNISA__)',
                            '#if FIRST\n#elif ALT\n    if (i == 9) return alternate;\n#elif LAST')),
            ('unsupported outer elif guard', 'cannot derive',
             lambda r: edit(r, 'exec/c/libunisacc.c', '#ifdef _WIN32\n    /* Windows gates',
                            '#ifdef _WIN32\n#elif ALT\n    /* Windows gates')),
        )
        for number, (name, expect, mutate) in enumerate(cases):
            broken = pathlib.Path(tmp) / ('case%d' % number)
            shutil.copytree(clean, broken)
            mutate(broken)
            lines, bad = check(broken)
            assert bad, name + ' must fail'
            assert any(expect in line for line in lines), '%s: expected "%s" in\n%s' % (name, expect, '\n'.join(lines))
    print('abimanifestcheck: unchanged tree passes; %d contract breaks each fail: %s' % (len(cases), '; '.join(c[0] for c in cases)))
    return 0


def main(argv):
    if argv == ['--selftest']:
        return selftest()
    if argv != ['--check']:
        print(__doc__.split('\n')[0], file=sys.stderr)
        return 2
    lines, bad = check()
    if lines:
        print('\n'.join(lines))
    if bad:
        print('abimanifestcheck  FAILED: %s differs from the sources. If the ABI change is intended, run\n'
              '  sh ./unisacc.com -run tests/abimanifestcheck.cx -- --write tests/abi.manifest\nand commit the manifest with the change.' % MANIFEST)
        return 1
    print('abimanifestcheck  %s matches the sources' % MANIFEST)
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
