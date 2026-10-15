#!/usr/bin/env python3
"""Recompute the reviewed-tree stamps and family guards in tests/gatedeps.json from HEAD (R14-2 (4)).

gatequeue.py fingerprints each reviewed tree as sha256 over {path: [st_mode,
sha256]}.  Computing that in the shared checkout is wrong twice over: a
teammate's uncommitted file lands in the stamp, and so do the checkout's own
permission bits (0711/0600 from a teammate's umask, where git checks out
0755/0644) -- 0.0.13's gate-infra stayed red through two such "refreshes".
So the stamps are computed in a temporary extraction of HEAD (`git archive`),
which has exactly git's content and git's modes, and only the json is written
back.  Run it as the LAST commit before a release queue: any later exec/ or
src/ change makes the stamp stale again.   usage: make gatedeps
"""
import hashlib, io, json, os, pathlib, subprocess, sys, tarfile, tempfile
R = pathlib.Path(__file__).resolve().parents[1]

def digest(path):
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        for b in iter(lambda: f.read(1 << 20), b''): h.update(b)
    return [os.stat(path).st_mode, h.hexdigest()]

def head_shas(paths):
    """sha256 of each path's content at HEAD, for paths outside the extracted trees, in one
    `git cat-file --batch` (0.0.28 R5: one `git show` per guard was 5,893 processes, 50 s)."""
    paths = sorted(set(paths)); out = {}
    if not paths: return out
    r = subprocess.run(['git', 'cat-file', '--batch'], cwd=R, input=''.join('HEAD:%s\n' % q for q in paths).encode(),
                       capture_output=True, timeout=60, check=True).stdout
    i = 0
    for q in paths:
        j = r.index(b'\n', i); head = r[i:j].split(); i = j + 1
        if head[-1] == b'missing': out[q] = None; continue
        n = int(head[2]); out[q] = hashlib.sha256(r[i:i+n]).hexdigest(); i += n + 1
    return out

def stamp(v): return hashlib.sha256(json.dumps(v, sort_keys=True).encode()).hexdigest()

def main():
    p = R / 'tests/gatedeps.json'; d = json.loads(p.read_text())
    trees = sorted({t for e in d['families'].values() for t in (e.get('reviewed_trees') or {})})
    with tempfile.TemporaryDirectory(prefix='unisacc-gatedeps-') as tmp:
        blob = subprocess.run(['git', '-c', 'tar.umask=022', 'archive', '--format=tar', 'HEAD'] + trees,   # git's default archive umask is 002 (0664 files); a checkout has 0644/0755
                           cwd=R, check=True, capture_output=True, timeout=60).stdout
        with tarfile.open(fileobj=io.BytesIO(blob)) as tf: tf.extractall(tmp, filter='fully_trusted')
        changed = []
        guards = [n for e in list(d['families'].values()) + list(d.get('suites', {}).values()) for n in e.get('guards', {})]
        outside = head_shas(n for n in guards if not (pathlib.Path(tmp) / n).is_file())
        for fam, entry in d['families'].items():
            rt = entry.get('reviewed_trees')
            if not rt: continue
            for tree in rt:
                base = pathlib.Path(tmp)
                paths = sorted(q for q in (base / tree).rglob('*') if q.is_file()
                               and (tree in entry['all_files_trees'] or q.suffix in entry['inventory_suffixes'])
                               and not any((base / x) == q or (base / x) in q.parents for x in entry.get('excluded_dirs', [])))
                s = stamp({str(q.relative_to(base)): digest(q) for q in paths})
                if rt[tree] != s: changed.append((fam, tree)); rt[tree] = s
            for n, sha in list(entry.get('guards', {}).items()):
                f = pathlib.Path(tmp) / n
                cur = digest(f)[1] if f.is_file() else outside[n]
                if cur and cur != sha: changed.append((fam, 'guard:' + n)); entry['guards'][n] = cur
        # 0.0.23 E: per-suite guards (the audited lib-* declarations) follow HEAD too
        for name, entry in d.get('suites', {}).items():
            for n, sha in list(entry.get('guards', {}).items()):
                f = pathlib.Path(tmp) / n
                cur = digest(f)[1] if f.is_file() else outside[n]
                if cur and cur != sha: changed.append((name, 'guard:' + n)); entry['guards'][n] = cur
    # 0.0.28 R4: seed-construct parts come from seedconstructcheck.py --list; each is declared like
    # seed-construct-parse2 (same inputs), with its own command, and parts that left the list are dropped
    parts = subprocess.run([sys.executable, str(R / 'tests/seedconstructcheck.py'), '--list'], cwd=R,
                           capture_output=True, text=True, timeout=10, check=True).stdout.split()
    tmpl = d['suites']['seed-construct-parse2']
    want = {'seed-construct-' + q: q for q in parts}
    for name in [n for n in d['suites'] if n.startswith('seed-construct-') and n not in want]:
        del d['suites'][name]; changed.append((name, 'dropped'))
    for name, q in want.items():
        e = json.loads(json.dumps(tmpl)); e['command'] = ['python3', './tests/seedconstructcheck.py', '--part', q]
        if name != 'seed-construct-parse2' and d['suites'].get(name) != e:
            d['suites'][name] = e; changed.append((name, 'declared'))
    # C43: isolated finite Tape-domain model; dynamic C checks existing parse oracle.
    for k in range(1, 4):
        name = 'tapecheck-model-' + str(k)
        entry = {
            'command': ['./exec/tapecheck/check.sh', str(k)],
            'files': ['exec/tapecheck/check.sh', 'tests/tapecheckmodel.cx',
                      'tests/tapecheck-cases.tsv', 'tests/bound', 'tests/bound.c',
                      'tests/bound.py', 'unisacc.com', 'unisacc.com.build.json'],
            'trees': ['exec/tapecheck', 'exec/build', 'exec/facts', 'exec/c',
                      'exec/pp', 'seed', 'include', 'unisa'],
            'executable_inputs': True,
            'tools': ['cc', 'python3', 'sh', 'cmp', 'mktemp', 'rm', 'tail']}
        if d['suites'].get(name) != entry:
            d['suites'][name] = entry
            changed.append((name, 'declared'))
    # LB-1: isolated finite lower side-stream; reference is an oracle only.
    d['suites']['exec-hostsymbols-model'] = {
        'command': ['./exec/lower/hostsymbolscheck.sh'],
        'files': ['exec/lower/hostsymbolscheck.sh', 'tests/hostsymbolscheck.cx',
                  'tests/hostsymbols-cases.tsv', 'tests/hostsymbols-outside-domain.tsv', 'tests/seals/middleflow-step0/manifest.tsv',
                  'tests/seals/middleflow-step0/lower/model.net', 'tests/bound',
                  'tests/bound.c', 'tests/bound.py', 'unisacc.com', 'unisacc.com.build.json'],
        'trees': ['exec/lower', 'exec/tapecheck', 'exec/build', 'exec/facts', 'exec/c',
                  'exec/pp', 'seed', 'include', 'unisa'],
        'executable_inputs': True,
        'tools': ['cc', 'python3', 'sh', 'cmp', 'mktemp', 'rm']}
    # 2a isolated address-free declaration model; Python is reference only.
    d['suites']['exec-librarycontract-model'] = {'command': ['./exec/librarycontract/check.sh'], 'files': ['exec/librarycontract/check.sh', 'tests/librarycontractcheck.cx', 'tests/librarycontract-cases.tsv', 'tests/bound', 'tests/bound.c', 'tests/bound.py', 'unisacc.com', 'unisacc.com.build.json'], 'trees': ['exec/librarycontract', 'exec/c', 'exec/build', 'exec/facts', 'exec/pp', 'seed', 'include', 'unisa'], 'executable_inputs': True, 'tools': ['cc', 'python3', 'sh', 'cmp', 'mktemp', 'rm']}
    d['suites']['exec-librarypartition-model'] = {'command': ['./exec/librarypartition/check.sh'], 'files': ['exec/librarypartition/check.sh', 'tests/librarypartitioncheck.cx', 'tests/librarypartition-cases.tsv', 'tests/bound', 'tests/bound.c', 'tests/bound.py', 'unisacc.com', 'unisacc.com.build.json'], 'trees': ['exec/librarypartition', 'exec/c', 'exec/build', 'exec/facts', 'exec/pp', 'seed', 'include', 'unisa'], 'executable_inputs': True, 'tools': ['cc', 'python3', 'sh', 'cmp', 'mktemp', 'rm']}
    d['suites']['exec-libraryfunctionimports-model'] = {'command': ['./exec/parse2/libraryfunctionimportscheck.sh'], 'files': ['exec/parse2/libraryfunctionimportscheck.sh', 'exec/parse2/libraryfunctionimports-observer.c', 'tests/libraryfunctionimportscheck.cx', 'tests/libraryfunctionimports-cases.tsv', 'tests/bound', 'tests/bound.c', 'tests/bound.py', 'unisacc.com', 'unisacc.com.build.json'], 'trees': ['exec/parse2', 'exec/build', 'exec/facts', 'exec/c', 'exec/pp', 'seed', 'include', 'unisa', 'weights'], 'executable_inputs': True, 'tools': ['cc', 'python3', 'sh', 'cmp', 'mktemp', 'rm', 'rg', 'sort', 'xargs', 'sha256sum', 'cut', 'cp', 'mkdir', 'env']}
    # Existing ABI contract job lacked declared inputs, blocking scoped commitgate.
    d['suites']['abi-manifest'] = {'command': ['tests/bound', '58', 'sh', './unisacc.com', '-run', 'tests/abimanifestcheck.cx', '--', '--check'], 'files': ['tests/abimanifestcheck.cx', 'tests/abi.manifest', 'unisacc.com', 'unisacc.com.build.json', 'tests/bound', 'tests/bound.c', 'tests/bound.py'], 'trees': ['exec', 'src', 'include'], 'executable_inputs': True, 'tools': ['sh']}
    d['suites']['exec-libraryfn-model'] = {'command': ['./exec/lower/libraryfncheck.sh'], 'files': ['exec/lower/libraryfncheck.sh', 'exec/lower/libraryfn-observer.c', 'tests/libraryfncheck.cx', 'tests/libraryfn-cases.tsv', 'tests/bound', 'tests/bound.c', 'tests/bound.py', 'unisacc.com', 'unisacc.com.build.json'], 'trees': ['exec/lower', 'exec/tapecheck', 'exec/build', 'exec/facts', 'exec/c', 'exec/pp', 'seed', 'include', 'unisa', 'weights'], 'executable_inputs': True, 'tools': ['cc', 'python3', 'sh', 'cmp', 'mktemp', 'rm', 'rg', 'sort', 'xargs', 'sha256sum', 'cut', 'cp', 'mkdir', 'env']}
    d['suites']['exec-librarydata-model'] = {'command': ['./exec/lower/librarydatacheck.sh'], 'files': ['exec/lower/librarydatacheck.sh', 'exec/lower/librarydata-observer.c', 'tests/librarydatacheck.cx', 'tests/librarydata-cases.tsv', 'tests/bound', 'tests/bound.c', 'tests/bound.py', 'unisacc.com', 'unisacc.com.build.json'], 'trees': ['exec/lower', 'exec/tapecheck', 'exec/build', 'exec/facts', 'exec/c', 'exec/pp', 'seed', 'include', 'unisa', 'weights'], 'executable_inputs': True, 'tools': ['cc', 'python3', 'sh', 'cmp', 'mktemp', 'rm', 'rg', 'sort', 'xargs', 'sha256sum', 'cut', 'cp', 'mkdir', 'env']}
    # Forwarding registration deliberately retains broad trees; no audited reuse claim.
    d['suites']['forward-multi'] = {
        'command': ['./tests/forwardmulti.sh'],
        'files': ['tests/forwardmulti.sh', 'tests/forwardpaths.cx', 'tests/lib.sh',
                  'tests/build_ref.sh', 'tests/export_ref.sh', 'tests/refshim.h',
                  'tests/reffoot.h', 'tests/bound', 'tests/bound.c', 'tests/bound.py',
                  'unisacc.c', 'unisacc.com', 'unisacc.com.build.json'],
        'trees': ['exec', 'include', 'iterate', 'kernel', 'seed', 'src', 'unisa', 'weights'],
        'executable_inputs': True,
        'tools': ['cc', 'python3', 'sh', 'chmod', 'mktemp', 'rm', 'uname', 'head', 'sed', 'env']}
    # 0.0.43 T-1: com-tapebin is split.  The reference is prepared once by com-tapebin-ref (keyed by
    # content inside tests/tapebinref.py); six file slices, the fixed-cost part and a global cover read it.
    # The guards are the HEAD stamps the old declaration already pinned, minus the product-facing script,
    # which the slices now own; nothing here is a new stamp.
    tb_guards = {k: v for k, v in d['suites'].get('com-tapebin-1', {}).get('guards', {}).items()
                 if k != 'tests/tapebinproduct.py'}
    tb_trees = ['examples', 'exec', 'include', 'iterate', 'kernel', 'seed', 'src', 'tests/c', 'unisa', 'weights']
    tb_files = ['Makefile', 'examples/hello.c', 'exec/c/chain.knownfail', 'exec/c/compiler.c', 'exec/c/tapebin.h',
                'exec/c/tapebin_emit.h', 'src/tapebin_encode.inc', 'src/tapebin_shape.inc', 'tests/bound', 'tests/bound.c',
                'tests/bound.py', 'tests/build_ref.sh', 'tests/export_ref.sh', 'tests/knownfail.py', 'tests/knownfail.sh',
                'tests/lib.sh', 'tests/par.sh', 'tests/product-refusals.knownfail', 'tests/reffoot.h', 'tests/refshim.h',
                'tests/tapebinproduct.py', 'tests/tapebinref.py', 'unisa/__init__.py', 'unisa/bits.py', 'unisa/fp.py',
                'unisa/tape.py', 'unisa/tapebin.py', 'unisacc.c', 'unisacc.com', 'unisacc.com.build.json']
    for name in [n for n in d['suites'] if n.startswith('com-tapebin-')]:
        del d['suites'][name]; changed.append((name, 'dropped'))
    d['suites']['com-tapebin-ref'] = {
        'command': ['python3', './tests/tapebinref.py', 'prepare'],
        'files': ['tests/tapebinref.py', 'tests/export_ref.sh', 'tests/build_ref.sh', 'tests/refshim.h', 'tests/reffoot.h', 'unisacc.c'],
        'guards': {k: v for k, v in tb_guards.items() if k in ('tests/bound', 'tests/bound.c', 'tests/bound.py',
                                                             'tests/build_ref.sh', 'tests/export_ref.sh', 'tests/reffoot.h', 'tests/refshim.h', 'unisacc.c')},
        'trees': ['exec', 'include', 'kernel', 'seed', 'src', 'unisa'],
        'executable_inputs': False}
    shard_entry = {'command': ['python3', './tests/tapebinproduct.py'], 'files': tb_files, 'guards': tb_guards,
                   'match': 'contains', 'trees': tb_trees, 'executable_inputs': True}
    for name in ['com-tapebin-six', 'com-tapebin-cover'] + ['com-tapebin-%d' % k for k in range(1, 7)]:
        d['suites'][name] = json.loads(json.dumps(shard_entry))
    changed.append(('com-tapebin', 'declared 1-6, six, cover, ref'))
    # 0.0.43 step 6, fifth cut: the outside-the-name-list reads the strace measured (research/c43-flow-reuse-step4.md).
    # Only what was opened: ledgercheck reads three plan files (its version.h keys which ones), docs reads four files,
    # subtract-safety walks every tracked directory except archive/ and ujs/ (its own check is "every tracked file").
    # ledgercheck reads the plan of the version that src/version.h names, and the one before it; a version bump
    # makes it read a plan file that did not exist when it was measured.  Declared as the whole plans/ tree so
    # that editing a new plan file (plans/v0.0.43.md) reruns it, instead of reusing the old result (0.0.43 step 6).
    d['suites']['ledgercheck'] = {
        'command': ['python3', './tests/ledgercheck.py'],
        'files': ['tests/ledgercheck.py', 'src/version.h'],
        'trees': ['plans']}
    d['suites']['docs'] = {
        'command': ['./tests/docs.sh'],
        # docs.sh opens these by fixed name (no directory listing); the English paper was missing from the first measure
        'files': ['tests/docs.sh', 'prd.md', 'spec.md', 'research/model-bytes.json', 'research/unisacc-paper.md', 'research/unisacc-paper.en.md']}
    d['suites']['subtract-safety'] = {
        'command': ['python3', './tests/subtractsafety.py'],
        'files': ['tests/subtractsafety.py', 'prd.md', 'spec.md', '.gitignore', '.gitattributes'],
        'trees': ['apps', 'bench', 'docs', 'plans', 'research', 'release', 'scripts', '.github']}
    changed.append(('step4-readsets', 'declared ledgercheck, docs, subtract-safety'))
    # 0.0.43 step 6, batch 1 of the outside-the-name-list audit (research/c43-flow-reuse-step6.md).
    # decision-ledger opens research/decision-ledger.json (measured).
    # c43 declaration gap (measured): decision-ledger walks exec/**/*.py (rglob) and the exec manifests (glob), and
    # reads tests/decisionledger.allow 2638 times, so exec/ is declared as a tree.
    d['suites']['decision-ledger'] = {
        'command': ['python3', './tests/decisionledger.py'],
        'files': ['tests/decisionledger.py', 'research/decision-ledger.json', 'tests/decisionledger.allow'],
        'trees': ['exec']}
    # exittable opens plans/v0.0.42.md (the version in src/version.h) and archive/plans/v0.0.38.md (measured);
    # the plan of a later version is read after a bump, so plans/ is declared as a whole tree (as ledgercheck).
    # c43 declaration gap (measured, batch B): exittable also reads the release ledgers and tools, the version
    # header and the gate-order/gate-layer/queue sources it prints from.
    d['suites']['exittable'] = {
        'command': ['python3', './tests/exittablecheck.py'],
        'files': ['tests/exittablecheck.py', 'archive/plans/v0.0.38.md', 'release/preauth.tsv', 'release/rulings.tsv',
                  'release/tools/exittable.py', 'release/tools/h1source.py', 'src/version.h', 'tests/gatelayers.py',
                  'tests/gateorder.json', 'tests/gatequeue.py'],
        'trees': ['plans']}
    # k2b walks every tracked file (tests/k2bcheck.py:10 git ls-files -co) and the partial strace (timed out at 55 s)
    # opened every tracked top-level directory; declared as those trees outside the name list.
    # c43 measured (batch A): the script itself is declared.  k2b reads every tracked file by design (whole-tree
    # identity), so this declaration cannot narrow it; see research/c43-k2b-enable-assessment.md.
    d['suites']['k2b'] = {
        'command': ['python3', './tests/k2bcheck.py'],
        'files': ['tests/k2bcheck.py'],
        'trees': ['apps', 'archive', 'bench', 'docs', 'plans', 'research', 'ujs', '.github']}
    changed.append(('step6-batch1', 'declared decision-ledger, exittable, k2b'))
    # 0.0.43 step 6, batch 2.  gate-infra, com-tapedomain and com-latebind run the ROOT product (unisacc.com, which
    # is git-ignored, so it is in no global name list) when MODEL_COM is unset; their measured reads include it.
    # Declared with executable_inputs so the root product's bytes enter the fingerprint (tests/gatequeue.py:410).
    rootfiles = ['unisacc.com', 'unisacc.com.build.json']
    gi = d['suites']['gate-infra']
    gi['files'] = sorted(set(gi.get('files', [])) | set(rootfiles))
    # c43 measured (batch A): the gate runs tests/bound around the product, and bound's sources are read too.
    d['suites']['com-tapedomain'] = {
        'command': ['sh', './unisacc.com', '-run', 'tests/tapedomaincheck.cx', '--', './unisacc.com'],
        'files': ['tests/tapedomaincheck.cx', 'tests/bound', 'tests/bound.c'] + rootfiles, 'executable_inputs': True}
    d['suites']['com-latebind'] = {
        'command': ['sh', './unisacc.com', '-run', 'tests/latebindcheck.cx', '--', './unisacc.com'],
        'files': ['tests/latebindcheck.cx', 'tests/bound', 'tests/bound.c'] + rootfiles, 'executable_inputs': True}
    changed.append(('step6-batch2', 'root product declared for gate-infra, com-tapedomain, com-latebind'))
    # 0.0.43 derived-consistency step 1 (research/c43-derived-consistency.md): the two checks the commit gate
    # will append.  Declared from the measured reads (strace, read-only): facts-export opens exec/, include/ and
    # the unisa/, weights/gold/, src/front_pp.c files of its TABLES; gate-layers opens the exec/facts tables and
    # the tests/ helpers below.  Declared trees are the directories it lists or reads whole.
    d['suites']['facts-export'] = {
        'command': ['python3', './exec/facts/export.py', '--check'],
        'files': ['exec/facts/export.py', 'src/front_pp.c', 'iterate/kernel/typekw.tsv'],
        'trees': ['exec', 'include', 'unisa', 'weights/gold']}
    d['suites']['gate-layers'] = {
        'command': ['python3', './tests/gatelayers.py', '--check'],
        'files': ['tests/gatelayers.py', 'tests/bound', 'tests/bound.c', 'tests/bound.py', 'tests/lib.sh',
                  'tests/seedconstructcheck.py', 'exec/facts/parse-constants.tsv', 'exec/facts/parse-tokens.tsv',
                  'exec/facts/parse-words.tsv', 'exec/facts/parse2-extra.tsv', 'exec/facts/valueranks.tsv'],
        'trees': ['exec/build']}
    changed.append(('derived-step1', 'declared facts-export and gate-layers (measured reads)'))
    # 0.0.43 seed C references (fix: seed C ports follow unisa/tape.py).  Declared from the measured reads: the three
    # checks read tests/bound, unisa/ (the Python side), and for the shape check docs/tapebin-v1.{shape,records}.tsv,
    # exec/c/tapebin.h and src/tapebin_shape.inc; the C reference each compiles is seed/unisa_*.c.
    d['suites']['unisa-tape'] = {
        'command': ['sh', './tests/unisatapecheck.sh'],
        'files': ['tests/unisatapecheck.sh', 'tests/bound', 'tests/bound.c', 'seed/unisa_tape.c'], 'trees': ['unisa']}
    d['suites']['unisa-tapebin'] = {
        'command': ['sh', './tests/unisatapebincheck.sh'],
        'files': ['tests/unisatapebincheck.sh', 'tests/bound', 'tests/bound.c', 'seed/unisa_tapebin.c'], 'trees': ['unisa']}
    d['suites']['unisa-tapebin-shape'] = {
        'command': ['sh', './tests/unisatapebinshapecheck.sh'],
        'files': ['tests/unisatapebinshapecheck.sh', 'tests/bound', 'tests/bound.c', 'seed/unisa_tapebin_shape.c',
                  'docs/tapebin-v1.shape.tsv', 'docs/tapebin-v1.records.tsv', 'exec/c/tapebin.h', 'src/tapebin_shape.inc'],
        'trees': ['unisa']}
    changed.append(('seed-c-ports', 'declared unisa-tape, unisa-tapebin, unisa-tapebin-shape'))
    # 0.0.43 (reviewed_trees as a derived file): the snapshot check, routed from exec/, src/ and unisa/ by
    # release/derived-map.tsv, so a commit that changes those trees carries the refreshed snapshot.  It reads the three
    # trees and the declaration only.
    d['suites']['reviewed-trees'] = {
        'command': ['python3', './tests/reviewedtreescheck.py'],
        'files': ['tests/reviewedtreescheck.py', 'tests/gatedeps.json'], 'trees': ['exec', 'src', 'unisa']}
    changed.append(('reviewed-trees', 'declared the snapshot check'))
    # Persist the registered inventory matcher/global-reachability self-test.
    d['suites']['script-inventory-self'] = {
        'command': ['python3', './tests/inventorycheck.py'],
        'files': ['tests/inventorycheck.py', 'tests/inventory.py'], 'tools': ['python3']}
    p.write_text(json.dumps(d, indent=2) + '\n')
    dirty = subprocess.run(['git', 'status', '--porcelain', '--'] + trees, cwd=R, capture_output=True, text=True).stdout.strip()
    print('gatedeps: reviewed stamps from HEAD; changed %s' % (changed or 'none'))
    if dirty: print('gatedeps: note -- uncommitted changes under reviewed trees are NOT in the stamp:\n' + dirty)
    return 0

if __name__ == '__main__':
    sys.exit(main())
