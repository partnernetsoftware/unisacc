#!/usr/bin/env python3
"""abimanifest.py [--root DIR] [--write | --print] -- the library/host ABI contract as data.

Derives tests/abi.manifest from the sources of truth.  Every line is produced by a rule in this
file; nothing is typed in by hand.  A rule that stops matching its source is an error, never an
empty section: a manifest that silently loses an entry would read "green but unchecked".

Line format: SECTION <tab> KEY <tab> VALUE, sorted by (SECTION, KEY).  No timestamps, no host
paths.  Keys carry a zero-padded ordinal wherever order is part of the contract.

Sections and their rules (ROOT-relative inputs):
  export        exec/c/libunisacc.h: each `US_API ... us_NAME(...);`, as result and parameter types
                with parameter names removed.  Cross-checked against the `API ... us_NAME(...) {`
                definitions in exec/c/libunisacc.c: same names, same types, or the generator fails.
  struct        exec/c/libunisacc.h: fields, in order, of each `typedef struct NAME { ... } NAME;`.
  visibility    exec/c/buildlibrary.py: whether the build passes -fvisibility=hidden (without it
                every non-static function would be an export this manifest does not list).
  callback      exec/c/libunisacc.c us_relocate: the parallel arrays names[] / vals[] / lengths[]:
                index, resource name, the host value handed to the guest, declared name length.
                Fails unless the three arrays have equal length and each length matches its name.
  callback-fn   exec/c/libunisacc.c: for every vals[] entry `(uintptr_t)FUNCTION`, the C signature
                of FUNCTION (defined there or in exec/c/library*.h) and the sha256 of its whitespace-normalised body (a behaviour proxy).
  guard         exec/c/libunisacc.c: the first statement of the POSIX (`#else`) and Windows
                (`#ifdef _WIN32`) arm of library_mmap / library_munmap: the argument-rejection rule.
  resource      every string literal beginning with \\0 in exec/c/*.c, exec/c/*.h, exec/c/asm/*.c,
                by file and order of appearance: the resource names the hosts spell.
  dlslot        src/host_dl.h: each `if (i == N) return X;` with the preprocessor conditions around it.
  gate          src/front_parse.c: each name in an `if (isname(t, "__x", n) ...) {` whose block emits
                an assembler directive (`es("  .word`): today .sys .sys6 .hostcall .hostaddr .argc .argv.
  image-path    src/back_image.c: every string literal that is an absolute path (ELF interpreter,
                dyld, libSystem).
  parse2-call   exec/parse2/body-manifest.tsv: each `call` row whose stem starts with `library`,
                in order, with its facts and options.
  table         sha256 of each *.tsv directly in TABLE_DIRS whose file name contains a TABLE_WORDS
                word (library, ccinterop, codeprivate, hostbridge, abi, syscall, signature, ...).
                Every exec/nativeabi/*.tsv is included whatever its name.
  table-dep     names (NOT hashes) of the stems and facts those manifests reference outside that
                set: shared rule tables whose change can alter ABI behaviour without changing a
                `table` line.
  bridge        sha256 of exec/c/library*.h, exec/c/librarycall*.S, exec/c/forwardsignature.h and
                src/host_dl.h: the call bridge and signature protocol implementation.
"""
import hashlib, pathlib, re, sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
MANIFEST = 'tests/abi.manifest'
TABLE_DIRS = ('exec', 'exec/lower', 'exec/parse2', 'exec/facts', 'exec/enc', 'exec/nativeabi')
TABLE_WORDS = ('library', 'ccinterop', 'codeprivate', 'hostbridge', 'abi', 'syscall', 'sysprep', 'signature',
               'modelinput', 'modelbindings', 'modelcandidates', 'modelgraphequality', 'layoutfacts',
               'layoutprovenance', 'memorylayout')


class Underived(Exception):
    """A rule no longer matches its source."""


def need(condition, message):
    if not condition:
        raise Underived(message)


def squash(text):
    return re.sub(r'\s+', ' ', text).strip()


def uncomment(text):
    return re.sub(r'/\*.*?\*/', ' ', text, flags=re.S)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def types_only(result, params):
    """`int f(const char *name, size_t n)` -> `int(const char*,size_t)`: names are not ABI."""
    out = []
    for p in params.split(','):
        p = squash(p)
        bare = re.sub(r'\b[A-Za-z_]\w*$', '', p).strip()
        if bare and bare not in ('const', 'unsigned', 'struct'):
            p = bare
        out.append(re.sub(r'\s*\*\s*', '*', p))
    return '%s(%s)' % (re.sub(r'\s*\*\s*', '*', squash(result)), ','.join(out))


def function(text, name):
    """(signature, body) of the C function definition `name` in text; body by brace matching."""
    found = list(re.finditer(r'^(static [^\n;{}]*\b%s\s*\([^)]*\))\s*\{' % re.escape(name), text, re.M))
    need(len(found) == 1, '%d definitions of %s' % (len(found), name))
    m = found[0]
    depth, at = 0, m.end() - 1
    for at in range(m.end() - 1, len(text)):
        depth += {'{': 1, '}': -1}.get(text[at], 0)
        if depth == 0:
            break
    need(depth == 0, 'unbalanced body of %s' % name)
    return squash(m.group(1)), text[m.end():at]


def initializer(text, declaration):
    m = re.search(re.escape(declaration) + r'\s*=\s*\{(.*?)\};', text, re.S)
    need(m, 'no initializer ' + declaration)
    return [squash(x) for x in re.split(r',(?![^()]*\))', m.group(1))]


def derive(root=ROOT):
    """(entries, inputs): the sorted manifest entries and every file the rules read."""
    root, entries, inputs = pathlib.Path(root), [], set()

    def read(rel, binary=False):
        inputs.add(rel)
        data = (root / rel).read_bytes()
        return data if binary else data.decode('utf-8')

    def add(section, key, value):
        entries.append((section, key, squash(value)))

    def files(*patterns):
        return sorted(p.relative_to(root).as_posix() for pat in patterns for p in root.glob(pat) if p.is_file())

    # export, struct
    header = re.sub(r'^\s*#.*$', '', uncomment(read('exec/c/libunisacc.h')), flags=re.M)
    library = read('exec/c/libunisacc.c')
    declared = {m.group(2): types_only(m.group(1), m.group(3))
                for m in re.finditer(r'\bUS_API\s+([^;(]+?)\b(us_\w+)\s*\(([^)]*)\)\s*;', header)}
    defined = {m.group(2): types_only(m.group(1), m.group(3))
               for m in re.finditer(r'^API\s+([^;(\n]+?)\b(us_\w+)\s*\(([^)]*)\)\s*\{', library, re.M)}
    need(declared, 'no US_API declaration in exec/c/libunisacc.h')
    need(header.count('US_API') == len(declared), 'a US_API line in libunisacc.h is not a function declaration this rule reads')
    need(len(re.findall(r'^API\b', library, re.M)) == len(defined), 'an API definition in libunisacc.c is not read by this rule')
    need(declared == defined, 'header and definition disagree: ' + ', '.join(
        sorted(k for k in set(declared) | set(defined) if declared.get(k) != defined.get(k))))
    for name, signature in declared.items():
        add('export', name, signature)
    structs = re.findall(r'typedef\s+struct\s+(\w+)\s*\{(.*?)\}\s*(\w+)\s*;', header, re.S)
    need(structs, 'no public struct in libunisacc.h')
    for tag, body, name in structs:
        add('struct', name, '; '.join(squash(f) for f in body.split(';') if f.strip()))
    add('visibility', 'exec/c/buildlibrary.py',
        'hidden' if "'-fvisibility=hidden'" in read('exec/c/buildlibrary.py') else 'DEFAULT')

    # callback, callback-fn, guard
    names = initializer(library, 'const char *names[]')
    values = initializer(library, 'uint64_t vals[]')
    lengths = initializer(library, 'int lengths[]')
    need(len(names) == len(values) == len(lengths), 'names/vals/lengths differ in length')
    need(re.search(r'for \(int i=0;i<%d;i\+\+\) \{resource_u64\(scalar\[i\],vals\[i\]\)' % len(names), library),
         'the resource loop bound is not the array length')
    for index, (name, value, length) in enumerate(zip(names, values, lengths)):
        need(re.fullmatch(r'"\\0[^"\\]+"', name), 'resource name not a \\0 literal: ' + name)
        need(int(length) == len(name) - 4 + 1, 'declared length of %s is %s' % (name, length))
        add('callback', '%02d' % index, '%s = %s (n=%s)' % (name[1:-1], value, length))
        m = re.fullmatch(r'\(uintptr_t\)([A-Za-z_]\w*)', value)
        if m:
            signature, body = function(library + '\n' + ''.join(
                read(rel) for rel in files('exec/c/library*.h')), m.group(1))
            add('callback-fn', m.group(1), '%s body-sha256=%s' % (signature, sha(squash(body).encode())))
    for name in ('library_mmap', 'library_munmap'):
        body = uncomment(function(library, name)[1])
        m = re.fullmatch(r'\s*#ifdef _WIN32\n(.*?)\n#else\n(.*?)\n#endif\s*', body, re.S)
        need(m, name + ' is not one #ifdef _WIN32 / #else / #endif')
        for arm, code in (('windows', m.group(1)), ('posix', m.group(2))):
            first = re.match(r'\s*(if\s*\(.*?\)\s*(?:\{.*?\}|return[^;]*;))', code, re.S)
            need(first, 'no leading guard in %s (%s)' % (name, arm))
            add('guard', '%s/%s' % (name, arm), first.group(1))

    # resource
    for rel in files('exec/c/*.c', 'exec/c/*.h', 'exec/c/asm/*.c'):
        for ordinal, literal in enumerate(re.findall(r'"\\0([^"\\]+)"', read(rel))):
            add('resource', '%s#%03d' % (rel, ordinal), literal)

    # dlslot
    stack, found = [], 0
    for line in read('src/host_dl.h').splitlines():
        s = line.strip()
        if re.match(r'#\s*if', s):
            stack.append(squash(s))
        elif re.match(r'#\s*else', s):
            need(stack, 'unbalanced #else in host_dl.h')
            stack[-1] = 'else of ' + stack[-1]
        elif re.match(r'#\s*endif', s):
            need(stack, 'unbalanced #endif in host_dl.h')
            stack.pop()
        m = re.match(r'if \(i == (\d+)\) return (.*);$', s)
        if m:
            add('dlslot', '%s | %s' % (' && '.join(stack), m.group(1)), m.group(2)); found += 1
    need(found and not stack, 'no loader slot rule found in src/host_dl.h')

    # gate
    parser, gates = read('src/front_parse.c'), 0
    blocks = list(re.finditer(r'^    if \((isname\(t, "__.*?)\) \{', parser, re.M | re.S))
    for block in blocks:
        end = parser.find('\n    }\n', block.end())
        emitted = sorted(set(re.findall(r'es\("  (\.[a-z0-9]+)', parser[block.end():end])))
        if not emitted:
            continue
        for name, length in re.findall(r'isname\(t, "(__\w+)", (\d+)\)', block.group(1)):
            need(len(name) == int(length), 'isname length of %s is %s' % (name, length))
            add('gate', name, ' '.join(emitted)); gates += 1
    need(gates, 'no syscall gate found in src/front_parse.c')

    # image-path
    paths = re.findall(r'"(/[^"\\]+)"', read('src/back_image.c'))
    need(paths, 'no absolute path literal in src/back_image.c')
    for ordinal, path in enumerate(paths):
        add('image-path', '%02d' % ordinal, path)

    # parse2-call
    calls = [l.split('\t') for l in read('exec/parse2/body-manifest.tsv').splitlines()
             if l.startswith('call\tlibrary')]
    need(calls, 'no library call in exec/parse2/body-manifest.tsv')
    for ordinal, row in enumerate(calls):
        add('parse2-call', '%02d' % ordinal, ' '.join(row[1:]))

    # table, table-dep
    tables = [rel for rel in files(*(d + '/*.tsv' for d in TABLE_DIRS))
              if rel.startswith('exec/nativeabi/') or any(w in pathlib.PurePosixPath(rel).name for w in TABLE_WORDS)]
    need(tables, 'no ABI table found')
    hashed = set(tables)
    for rel in tables:
        data = read(rel, binary=True)
        add('table', rel, 'sha256=%s bytes=%d' % (sha(data), len(data)))
        if not rel.endswith('-manifest.tsv'):
            continue
        directory = pathlib.PurePosixPath(rel).parent
        for line in data.decode('utf-8').splitlines():
            row = line.split('\t')
            if line.startswith('#') or len(row) < 5:
                continue
            stem = row[1]
            if stem != '-' and not any(w in pathlib.PurePosixPath(stem).name for w in TABLE_WORDS):
                add('table-dep', '%s stem %s' % (rel, stem), 'unhashed')
            for fact in row[4].split('+'):
                fact = fact[5:] if fact.startswith('fact:') else fact
                if fact not in ('-', '') and 'exec/facts/%s.tsv' % fact not in hashed:
                    add('table-dep', '%s facts %s' % (rel, fact), 'unhashed')

    # bridge
    for rel in files('exec/c/library*.h', 'exec/c/librarycall*.S') + ['exec/c/forwardsignature.h', 'src/host_dl.h']:
        add('bridge', rel, 'sha256=' + sha(read(rel, binary=True)))

    entries = sorted(set(entries))
    keys = [(s, k) for s, k, _ in entries]
    need(len(keys) == len(set(keys)), 'duplicate manifest key')
    return entries, sorted(inputs)


def render(entries):
    head = ('# ABI contract manifest.  Generated by `python3 tests/abimanifest.py --write`; do not edit.\n'
            '# Checked by tests/abimanifestcheck.py --check.  Rules: the docstring of tests/abimanifest.py.\n'
            '# section\tkey\tvalue\n')
    return head + ''.join('%s\t%s\t%s\n' % e for e in entries)


def parse(text):
    out = []
    for line in text.splitlines():
        if line and not line.startswith('#'):
            section, key, value = line.split('\t', 2)
            out.append((section, key, value))
    return out


def main(argv):
    root = ROOT
    if argv[:1] == ['--root']:
        root, argv = pathlib.Path(argv[1]), argv[2:]
    if argv not in (['--write'], ['--print']):
        print(__doc__.split('\n')[0], file=sys.stderr)
        return 2
    try:
        text = render(derive(root)[0])
    except Underived as error:
        print('abimanifest: cannot derive: %s' % error, file=sys.stderr)
        return 1
    if argv == ['--write']:
        (root / MANIFEST).write_bytes(text.encode('utf-8'))
        print('abimanifest: wrote %s, %d entries' % (MANIFEST, text.count('\n') - 3))
    else:
        sys.stdout.write(text)
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
