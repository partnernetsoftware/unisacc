#!/usr/bin/env python3
"""subtract-safety gate (R16-9): removing or archiving a file must not leave a dangling user.

Two 0.0.15 incidents, each with no gate that could have caught it:
  * 900e7f1 archived tests/modeltypedcandidatescheck.py while seven gated lib-*
    checks still imported it -> 8 queue reds, found a day later (30be224);
  * bc86247's cleanup turned AGENTS.md and CLAUDE.md into a link cycle, so the
    instruction file resolved to nothing (the rules silently vanished).

Checks, all over the tracked tree (git ls-files; ujs/ is another project and skipped):
  1. no file outside archive/ imports a Python module whose only definition is
     under archive/;
  2. no file outside archive/ names a path (`dir/name.ext`, shell `source`,
     markdown link) whose basename is an archived file and which resolves to
     nothing outside archive/ -- links that say `archive/...` are fine, that is
     the point of archiving;
  3. AGENTS.md and CLAUDE.md each resolve (at most 8 symlink hops, no cycle) to
     a non-empty regular file with a heading.
`--selftest` builds one counterexample per incident in a temp tree and requires
the gate to catch both (the acceptance in archive/plans/v0.0.16.md).
"""
import os
import pathlib
import re
import subprocess
import sys
import tempfile

ARCHIVE = 'archive'
SKIP_PREFIXES = ('ujs/', '.git/')
IMPORT_RE = re.compile(r'^\s*(?:from\s+([\w.]+)\s+import\b|import\s+([\w.]+(?:\s*,\s*[\w.]+)*))', re.M)
PATH_RE = re.compile(r'(?<![\w$@{])[\w][\w.-]*(?:/[\w.-]+)+')
TEXT_SUFFIXES = {'.py', '.sh', '.md', '.c', '.h', '.tsv', '.txt', '.json', '.yml', '.yaml',
                 '.toml', '.cfg', '.ini', '.mk', '.S', '.s', '.html', '.css', '.js', '.in', '.knownfail', ''}


def tracked(root):
    """Repo-relative paths of the files the gate reads (git when available, else a walk)."""
    try:
        out = subprocess.run(['git', '-C', str(root), 'ls-files', '-z', '--cached'],
                             capture_output=True, check=True, timeout=30).stdout
        names = [n for n in out.decode().split('\0') if n]
    except (subprocess.CalledProcessError, FileNotFoundError):
        names = []
        for dirpath, dirs, files in os.walk(root):
            dirs[:] = [d for d in dirs if d not in ('.git', '__pycache__')]
            for f in files:
                names.append(os.path.relpath(os.path.join(dirpath, f), root))
    keep = []
    for n in names:
        if n.startswith(SKIP_PREFIXES):
            continue
        try:
            st = os.lstat(root / n)
        except FileNotFoundError:
            continue  # deleted in the working tree but still in the index: nothing to read
        if os.path.stat.S_ISREG(st.st_mode) or os.path.stat.S_ISLNK(st.st_mode):
            keep.append(n)
    return sorted(keep)


def read_text(path):
    try:
        with open(path, 'rb') as f:
            data = f.read(4 << 20)
    except (FileNotFoundError, IsADirectoryError, PermissionError):
        return None
    if b'\0' in data[:4096]:
        return None
    try:
        return data.decode('utf-8')
    except UnicodeDecodeError:
        return None


def check_tree(root):
    root = pathlib.Path(root)
    names = tracked(root)
    outside = [n for n in names if not n.startswith(ARCHIVE + '/')]
    archived = [n for n in names if n.startswith(ARCHIVE + '/')]
    outside_set = set(outside)
    outside_stems = {pathlib.PurePosixPath(n).stem for n in outside if n.endswith('.py')}
    outside_stems |= {pathlib.PurePosixPath(n).parent.name for n in outside if n.endswith('/__init__.py')}
    archived_stems = {pathlib.PurePosixPath(n).stem for n in archived if n.endswith('.py')} - {'__init__', '__main__'}
    archived_names = {pathlib.PurePosixPath(n).name for n in archived}
    outside_by_name = {}
    for n in outside:
        outside_by_name.setdefault(pathlib.PurePosixPath(n).name, []).append(n)

    def resolves(token, referrer):
        """Does `token` name a tracked file outside archive/ from where it is written?"""
        t = token
        while t.startswith('./'):
            t = t[2:]
        if t.startswith(SKIP_PREFIXES):
            return True  # another project's tree: not scanned, not judged
        parts = t.split('/')
        for i in range(len(parts)):  # `$R/tests/lib.sh` arrives as `R/tests/lib.sh`: try every suffix
            suffix = '/'.join(parts[i:])
            if suffix in outside_set:
                return True
            rel = os.path.normpath(os.path.join(os.path.dirname(referrer), suffix))
            if rel in outside_set:
                return True
        return False

    findings = []
    for n in outside:
        suffix = pathlib.PurePosixPath(n).suffix
        if suffix not in TEXT_SUFFIXES and not n.endswith('Makefile'):
            continue
        text = read_text(root / n)
        if text is None:
            continue
        if n.endswith('.py'):
            for m in IMPORT_RE.finditer(text):
                mods = [m.group(1)] if m.group(1) else [x.strip() for x in m.group(2).split(',')]
                for mod in mods:
                    head = mod.split('.')[0]
                    if head in archived_stems and head not in outside_stems:
                        line = text.count('\n', 0, m.start()) + 1
                        findings.append('%s:%d imports %s, which exists only under %s/' % (n, line, head, ARCHIVE))
        for m in PATH_RE.finditer(text):
            token = m.group(0).rstrip('.')
            if token.startswith(ARCHIVE + '/') or '/' + ARCHIVE + '/' in token:
                continue
            base = token.rsplit('/', 1)[-1]
            if base not in archived_names or '.' not in base:
                continue
            if resolves(token, n):
                continue
            line = text.count('\n', 0, m.start()) + 1
            findings.append('%s:%d names %s, but the only %s is under %s/' % (n, line, token, base, ARCHIVE))
    return findings


def check_instructions(root):
    root = pathlib.Path(root)
    findings = []
    for name in ('AGENTS.md', 'CLAUDE.md'):
        cur = root / name
        seen = []
        for _ in range(8):
            try:
                st = os.lstat(cur)
            except FileNotFoundError:
                findings.append('%s: missing (%s)' % (name, ' -> '.join(seen + [str(cur)])))
                cur = None
                break
            if not os.path.stat.S_ISLNK(st.st_mode):
                break
            seen.append(str(cur))
            target = os.readlink(cur)
            cur = (cur.parent / target) if not os.path.isabs(target) else pathlib.Path(target)
            cur = pathlib.Path(os.path.normpath(cur))
            if str(cur) in seen:
                findings.append('%s: symlink cycle %s' % (name, ' -> '.join(seen + [str(cur)])))
                cur = None
                break
        else:
            findings.append('%s: more than 8 symlink hops' % name)
            cur = None
        if cur is None:
            continue
        text = read_text(cur)
        if not text or not text.strip():
            findings.append('%s: resolves to an empty file (%s)' % (name, cur))
        elif not re.search(r'^#', text, re.M):
            findings.append('%s: resolves to a file with no heading (%s)' % (name, cur))
    return findings


def selftest():
    with tempfile.TemporaryDirectory(prefix='subtract-safety-') as d:
        root = pathlib.Path(d)
        (root / 'tests').mkdir()
        (root / 'archive/tests').mkdir(parents=True)
        (root / 'AGENTS.md').write_text('# rules\n')
        (root / 'CLAUDE.md').symlink_to('AGENTS.md')
        (root / 'tests/libcheck.py').write_text('import os\nfrom modeltypedcandidatescheck import main\n')
        (root / 'tests/run.sh').write_text('. "$R/tests/lib.sh"\npython3 tests/old_check.py\n')
        (root / 'tests/lib.sh').write_text('x=1\n')
        (root / 'docs.md').write_text('see [old](archive/tests/old_check.py) and tests/lib.sh\n')
        (root / 'archive/tests/modeltypedcandidatescheck.py').write_text('def main(): pass\n')
        (root / 'archive/tests/old_check.py').write_text('print(1)\n')
        f = check_tree(root)
        assert any('tests/libcheck.py:2 imports modeltypedcandidatescheck' in x for x in f), f
        assert any('tests/run.sh:2 names tests/old_check.py' in x for x in f), f
        assert not any('docs.md' in x or 'lib.sh' in x for x in f), f
        assert check_instructions(root) == [], check_instructions(root)
        # incident 1 repaired: the module is back outside archive/
        (root / 'tests/modeltypedcandidatescheck.py').write_text('def main(): pass\n')
        (root / 'tests/old_check.py').write_text('print(1)\n')
        assert check_tree(root) == [], check_tree(root)
        # incident 2: AGENTS.md -> CLAUDE.md -> AGENTS.md
        (root / 'AGENTS.md').unlink()
        (root / 'AGENTS.md').symlink_to('CLAUDE.md')
        f = check_instructions(root)
        assert len(f) == 2 and all('symlink cycle' in x for x in f), f
        (root / 'AGENTS.md').unlink()
        (root / 'AGENTS.md').write_text('')
        f = check_instructions(root)
        assert len(f) == 2 and all('empty file' in x for x in f), f
    print('subtract-safety selftest: both 0.0.15 counterexamples caught')


def main(argv):
    if '--selftest' in argv:
        selftest()
        return 0
    root = pathlib.Path(__file__).resolve().parents[1]
    if len(argv) > 1 and not argv[1].startswith('--'):
        root = pathlib.Path(argv[1])
    findings = check_tree(root) + check_instructions(root)
    for f in findings:
        print('subtract-safety: ' + f)
    archived = sum(1 for n in tracked(root) if n.startswith(ARCHIVE + '/'))
    print('subtract-safety  archived files %d   dangling users %d   instruction files ok %s'
          % (archived, len(findings), 'no' if any(x.startswith(('AGENTS.md', 'CLAUDE.md')) for x in findings) else 'yes'))
    return 1 if findings else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
