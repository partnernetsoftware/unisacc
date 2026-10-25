#!/usr/bin/env python3
"""Named regressions for inventory matching and global reachability."""
import ast
import pathlib
import re
import subprocess
import sys
import tempfile
SOURCE = pathlib.Path(sys.argv[1]) if len(sys.argv) == 2 else pathlib.Path(__file__).with_name('inventory.py')
source = SOURCE.read_text()
module = ast.parse(source)
# Execute only the actual matcher initialization/function; no copied optimized algorithm.
nodes = [n for n in module.body if (isinstance(n, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'ref_patterns' for t in n.targets)) or (isinstance(n, ast.FunctionDef) and n.name == 'refs')]
ns = {'pathlib': pathlib, 're': re, 'names': {'tests/probe.py': None, 'exec/check/probe.py': None}}
exec(compile(ast.Module(body=nodes, type_ignores=[]), str(SOURCE), 'exec'), ns)
checks = 0
def check(condition, name):
    global checks
    if not condition:
        raise AssertionError(name)
    checks += 1
    print('ok', name)
check(ns['refs']('xprobe.pyZ') == set(), 'invalid-boundary-no-extra-match')
for text, name in [('tests/probe.py', 'full-path'), ('/probe.py', 'slash-basename'), ('"probe.py"', 'bare-basename')]:
    found = ns['refs'](text)
    check(found == set(ns['names']), name + '-duplicate-basename-retained')
with tempfile.TemporaryDirectory(prefix='inventory-self-') as d:
    root = pathlib.Path(d)
    tests = root / 'tests'; tests.mkdir()
    (tests / 'inventory.py').write_text(source)
    files = {'gate.sh': 'job demo python3 ./tests/a.py\n', 'all.sh': '', 'release.sh': '',
             'a.py': 'import helper\n# ./tests/b.py\n', 'b.py': '# ./tests/a.py\n',
             'helper.py': '# library\n', 'listed.py': '', 'orphan.py': ''}
    for name, text in files.items(): (tests / name).write_text(text)
    disposition = ''.join('tests/' + name + '\tfixture\n' for name in ['inventory.py', 'gate.sh', 'all.sh', 'release.sh', 'listed.py'])
    (tests / 'ungated-checks.tsv').write_text(disposition)
    def scan():
        p = subprocess.run([sys.executable, str(tests / 'inventory.py'), '--write', '--check'], capture_output=True, text=True, timeout=5)
        if p.returncode not in (0, 1): raise AssertionError(('mechanism', p.returncode, p.stderr))
        return p.returncode, p.stdout, (tests / 'INVENTORY.md').read_text()
    rc, out, rows = scan()
    check(rc == 1 and 'ORPHAN tests/orphan.py' in out, 'unlisted-orphan-fails')
    check('| `tests/a.py` | gate: demo |' in rows and '| `tests/b.py` | reached from a gate/CI script |' in rows, 'cycle-reachability-terminates')
    check('| `tests/helper.py` | library (imported by a gated script) |' in rows, 'imported-library-classified')
    check('| `tests/listed.py` | listed in tests/ungated-checks.tsv |' in rows, 'listed-orphan-disposition')
    (tests / 'ungated-checks.tsv').write_text(disposition + 'tests/orphan.py\tfixture\n')
    rc, out, before = scan()
    check(rc == 0 and 'orphan 0' in out, 'listing-orphan-clears-failure')
    (tests / 'gate.sh').write_text('')
    rc, out, after = scan()
    check(rc == 1 and before != after and all('ORPHAN tests/' + n in out for n in ('a.py','b.py','helper.py')), 'delete-root-reference-changes-global-closure')
    (tests / 'gate.sh').write_text(files['gate.sh'])
    (tests / 'ungated-checks.tsv').write_text(disposition)
    rc, out, restored = scan()
    check(rc == 1 and restored == rows, 'restore-input-restores-exact-rows')
print('inventory-self:', checks, 'named assertions passed')
