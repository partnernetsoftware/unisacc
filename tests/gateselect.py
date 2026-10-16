#!/usr/bin/env python3
"""Offline tier planner using the shared gate declarations and stage map.
Dev/merge accept additive scopes; candidate/release always plan the full list.
No gate execution, queue state access, environment pin, or result cache.
"""
import argparse
import hashlib
import json
import re
from pathlib import Path

FIXED = {
    ("tier", "dev.expand"): "A+B+H",
    ("tier", "merge.expand"): "A+B+L(A)+H+M",
    ("tier", "candidate.expand"): "U",
    ("tier", "release.expand"): "U",
    ("tier", "merge.hot_seconds"): "UNCOMMITTED",
    ("policy", "unknown-input"): "global",
    ("policy", "unclaimed-change"): "global",
    ("policy", "filter-mode"): "additive",
    ("policy", "budget-mode"): "report-only",
}
BUDGETS = {("tier", "dev.hot_seconds"), ("tier", "dev.cold_seconds")}
SMOKE_ROLES = frozenset(("infrastructure", "sc1-control", "product-control"))


class SelectionError(ValueError):
    pass


def require(ok, reason):
    if not ok:
        raise SelectionError(reason)


def digest(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(",", ":")).encode()).hexdigest()


def parse_policy(text):
    values, smokes, aliases, asks = {}, {}, {}, {}
    vocabulary = None
    header = False
    for lineno, line in enumerate(text.splitlines(), 1):
        if not line.strip() or line.startswith("#"):
            continue
        parts = line.split("\t")
        require(len(parts) == 3, "policy-shape line %d" % lineno)
        if not header:
            require(parts == ["record", "key", "value"], "policy-header")
            header = True
            continue
        record, key, value = parts
        require(all(parts) and all(s == s.strip() for s in parts), "policy-empty-or-space line %d" % lineno)
        pair = record, key
        require(pair not in values, "policy-duplicate %s/%s" % pair)
        if pair in FIXED:
            require(value == FIXED[pair], "policy-value %s/%s" % pair)
        elif pair in BUDGETS:
            require(value.isascii() and value.isdigit() and 0 < int(value) <= 86400, "policy-budget")
        elif pair == ("tier", "merge.include"):
            names = value.split(",")
            require(all(names) and len(set(names)) == len(names), "merge-include-empty-or-duplicate")
        elif record == "stage.alias":
            tags = value.split(",")
            require(tags == sorted(set(tags)) and all(tags) and not any(c.isspace() for c in value), "alias-values")
            aliases[key] = tags
        elif record == "ask":
            tags = value.split(",")
            require(all(re.fullmatch(r"[a-z][a-z0-9_-]*", t, flags=re.ASCII) for t in tags) and len(tags) == len(set(tags)), "ask-values")
            if key == "vocabulary":
                require(len(tags) == 18, "ask-vocabulary-count")
                vocabulary = tags
            else:
                require(key.startswith("suite.") and len(key) > 6, "ask-key")
                asks[key[6:]] = tags
        elif record == "smoke" and key in SMOKE_ROLES:
            names = value.split(",")
            require(all(names) and len(set(names)) == len(names), "smoke-empty-or-duplicate")
            smokes[key] = names
        else:
            raise SelectionError("policy-unknown %s/%s" % pair)
        values[pair] = value
    require(header and set(FIXED) | BUDGETS <= set(values), "policy-missing-required")
    require(set(smokes) == SMOKE_ROLES, "policy-missing-smoke-role")
    require(("tier", "merge.include") in values, "policy-missing-merge-include")
    all_smokes = [n for group in smokes.values() for n in group]
    require(len(all_smokes) == len(set(all_smokes)), "smoke-duplicate-role-owner")
    if aliases:
        expected = dict(pp=["E2"], lex=["E1"], parse=["E3"], opt=["E4"], prune=["E4"], lowering=["lower"], enc=["E5", "E6"])
        require(all(aliases.get(k) == v for k, v in expected.items()), "alias-required-mapping")
        require(set(aliases) <= set(expected) | {"image"} and ("image" not in aliases or aliases["image"] == ["E6"]), "alias-unknown")
    require(not asks or vocabulary is not None, "ask-axis-disabled")
    for tags in asks.values():
        require(set(tags) <= set(vocabulary) and tags == sorted(tags, key=vocabulary.index), "ask-label-order-or-unknown")
    return {"aliases": aliases, "ask_vocabulary": vocabulary, "ask_suites": asks, "values": [[r, k, v] for (r, k), v in sorted(values.items())],
            "smokes": all_smokes,
            "merge_suites": values[("tier", "merge.include")].split(","),
            "hot_seconds": int(values[("tier", "dev.hot_seconds")]),
            "cold_seconds": int(values[("tier", "dev.cold_seconds")])}


def validate_fixture(f):
    require(f.get("mode") == "FIXTURE_ONLY", "real-declaration-adapter-not-ready")
    jobs = f.get("jobs")
    require(isinstance(jobs, list) and jobs, "universe-empty-or-malformed")
    require(all(isinstance(j, dict) for j in jobs), "job-record-shape")
    require(isinstance(f.get("metadata_changed", False), bool), "metadata-flag-shape")
    names = [j.get("name") for j in jobs]
    require(all(isinstance(n, str) and n for n in names), "universe-name")
    require(len(set(names)) == len(names), "universe-duplicate")
    universe = set(names)
    catalog = f.get("catalog", {})
    for axis in ("layers", "stages"):
        tags = catalog.get(axis)
        require(isinstance(tags, list) and tags and all(isinstance(t, str) and t for t in tags)
                and len(tags) == len(set(tags)), "catalog-shape " + axis)
    layers, stages = set(catalog["layers"]), set(catalog["stages"])
    for j in jobs:
        require(j.get("layer") in layers, "unclassified-layer " + j["name"])
        require(j.get("primary") in stages, "unclassified-primary " + j["name"])
        require(isinstance(j.get("also", []), list) and set(j.get("also", [])) <= stages, "unclassified-also " + j["name"])
        require(isinstance(j.get("after", []), list), "predecessor-shape")
        require(set(j.get("after", [])) <= universe, "dangling-predecessor " + j["name"])
        require(isinstance(j.get("command"), list) and j["command"] and all(isinstance(a, str) for a in j["command"]), "command-shape")
        require(j.get("declaration") in ("known", "unknown"), "declaration-not-normalized")
    require(set(f.get("affected", [])) <= universe, "affected-not-in-universe")
    changes = f.get("changes", [])
    require(isinstance(changes, list), "changes-shape")
    for change in changes:
        require(isinstance(change, dict) and change.get("status") in ("M", "A", "D", "R", "?"), "change-status")
        paths = change.get("paths")
        require(isinstance(paths, list) and len(paths) == (2 if change["status"] == "R" else 1), "change-paths")
        require(all(isinstance(p, str) and p and "\0" not in p for p in paths), "change-path")
    changed = {p for change in changes for p in change["paths"]}
    require(set(f.get("unclaimed", [])) <= changed, "unclaimed-not-changed")
    # Validate all topology, not only the current selection; no hidden cycles.
    by_name = {j["name"]: j for j in jobs}
    visiting, done = set(), set()
    def visit(n):
        require(n not in visiting, "predecessor-cycle " + n)
        if n in done:
            return
        visiting.add(n)
        for prior in by_name[n].get("after", []):
            visit(prior)
        visiting.remove(n)
        done.add(n)
    for n in names:
        visit(n)
    return jobs, by_name, changed


def bind_ask_catalog(policy, raw):
    rows = re.findall(rb"^#define S_([A-Z][A-Z0-9_]*) ([0-9]+)$", raw, re.M)
    require(len(rows) == 18 and [int(i) for n, i in rows] == list(range(18)), "ask-generated-catalog-shape")
    names = [n.decode().lower() for n, i in rows]
    require(len(set(names)) == 18 and policy["ask_vocabulary"] == names, "ask-generated-catalog-mismatch")
    return hashlib.sha256(raw).hexdigest()


def select(fixture, policy, tier="dev", layers=(), stages=(), asks=()):
    jobs, by_name, changed = validate_fixture(fixture)
    require(tier in ("dev", "merge", "candidate", "release", "inspect"), "unknown-tier")
    require(set(layers) <= set(fixture["catalog"]["layers"]), "unknown-layer-filter")
    paper_stages = sorted(set(stages))
    stage_catalog = set(fixture["catalog"]["stages"])
    aliases = policy.get("aliases", {})
    require(not (set(aliases) & stage_catalog), "alias-engineering-conflict")
    require(all(set(v) <= stage_catalog for v in aliases.values()), "alias-unknown-target")
    stages = sorted({t for q in stages for t in aliases.get(q, [q])})
    require(set(stages) <= stage_catalog, "unknown-stage-filter")
    asks = sorted(set(asks))
    vocabulary = policy.get("ask_vocabulary")
    require(not asks or vocabulary is not None, "ask-axis-disabled")
    require(not asks or "ask_catalog_sha256" in fixture, "ask-generated-catalog-unbound")
    require(set(asks) <= set(vocabulary or []), "unknown-ask-filter")
    require(tier not in ("candidate", "release") or not (layers or stages or asks), "full-tier-filter-forbidden")
    names = [j["name"] for j in jobs]
    universe = set(names)
    smoke = set(policy["smokes"])
    require(not any(n.startswith("PENDING_") for n in smoke), "smoke-role-unresolved")
    require(smoke <= universe, "smoke-not-in-universe " + ",".join(sorted(smoke - universe)))
    ask_suites = policy.get("ask_suites", {})
    require(set(ask_suites) <= universe, "ask-suite-not-in-universe")
    require(all(any(q in tags for tags in ask_suites.values()) for q in asks), "ask-unmapped")
    affected = set(fixture.get("affected", []))
    reason = {n: [] for n in names}
    for n in affected:
        reason[n].append("changed-input:shared-adapter")
    for n in smoke:
        reason[n].append("smoke")
    if changed:
        for j in jobs:
            if j["declaration"] == "unknown":
                affected.add(j["name"])
                reason[j["name"]].append("unknown-declaration:global")
    if fixture.get("unclaimed") or fixture.get("metadata_changed"):
        affected = set(universe)
        for n in names:
            reason[n].append("unclaimed-or-metadata:global")
    extra = set()
    if layers or stages:
        for j in jobs:
            layer_hit = not layers or j["layer"] in layers
            stage_hit = not stages or j["primary"] in stages or bool(set(j.get("also", [])) & set(stages))
            if layer_hit and stage_hit:
                extra.add(j["name"])
                if layers:
                    reason[j["name"]].append("layer-filter:additive")
                if stages:
                    reason[j["name"]].append("stage-primary" if j["primary"] in stages else "stage-also")
    for n, tags in ask_suites.items():
        for q in asks:
            if q in tags:
                extra.add(n)
                reason[n].append("ask-label:" + q)
    if tier in ("candidate", "release"):
        chosen = set(universe)
        for n in names:
            reason[n].append("full-universe")
    elif tier == "inspect":
        chosen = extra if layers or stages or asks else set(universe)
    else:
        chosen = affected | smoke | extra
        if tier == "merge":
            require(set(policy["merge_suites"]) <= universe, "merge-control-not-in-universe")
            chosen.update(policy["merge_suites"])
            for n in policy["merge_suites"]:
                reason[n].append("merge-control")
            affected_layers = {by_name[n]["layer"] for n in affected}
            for j in jobs:
                if j["layer"] in affected_layers:
                    chosen.add(j["name"])
                    reason[j["name"]].append("affected-layer:" + j["layer"])
    if tier != "inspect":
        work = list(chosen)
        while work:
            n = work.pop()
            for prior in by_name[n].get("after", []):
                reason[prior].append("predecessor:" + n)
                if prior not in chosen:
                    chosen.add(prior)
                    work.append(prior)
    ordered = [n for n in names if n in chosen]
    broad = bool(universe) and len(chosen) * 2 >= len(universe)
    return {"format": "GATESELECT_FIXTURE1", "mode": "FIXTURE_ONLY", "tier": tier,
            "input_sha256": digest(fixture), "policy_sha256": digest(policy),
            "filters": {"layers": sorted(set(layers)), "stages": paper_stages, "expanded_stages": stages, "asks": asks},
            "ask_catalog_sha256": fixture.get("ask_catalog_sha256"),
            "suites": ordered, "affected": [n for n in names if n in affected],
            "reasons": {n: sorted(set(reason[n])) for n in ordered},
            "status": "INSPECT_ONLY" if tier == "inspect" else "PLAN_ONLY",
            "budget": {"hot_seconds": policy["hot_seconds"] if tier == "dev" else None,
                       "cold_seconds": policy["cold_seconds"] if tier == "dev" else None,
                       "enforcement": "REPORT_ONLY_NEVER_TRUNCATE"},
            "breadth": "ESCALATE_BREADTH" if broad and tier in ("dev", "merge") else "NORMAL",
            "coverage_verdict": "NOT_ASSESSED",
            "adapter": "NORMALIZED_FIXTURE_ONLY"}


def verify(plan, fixture, policy):
    jobs, by_name, changed = validate_fixture(fixture)
    require(plan.get("format") == "GATESELECT_FIXTURE1", "plan-format")
    require(plan.get("input_sha256") == digest(fixture), "plan-input-changed")
    require(plan.get("policy_sha256") == digest(policy), "plan-policy-changed")
    selected = plan.get("suites")
    require(isinstance(selected, list) and len(set(selected)) == len(selected), "plan-duplicate-or-shape")
    universe = set(by_name)
    require(set(selected) <= universe, "plan-outside-universe")
    filters = plan.get("filters", {})
    expected = select(fixture, policy, plan.get("tier"), filters.get("layers", []), filters.get("stages", []), filters.get("asks", []))
    if plan["tier"] != "inspect":
        # Independent invariants use the trusted fixture/step1 result, never plan's A/B.
        require(set(expected["affected"]) | set(policy["smokes"]) <= set(selected), "plan-missing-A-or-B")
        for n in selected:
            require(set(by_name[n].get("after", [])) <= set(selected), "plan-missing-predecessor " + n)
        if plan["tier"] in ("candidate", "release"):
            require(set(selected) == universe, "plan-full-universe-required")
    require(selected == expected["suites"], "plan-selection-or-order-mismatch")
    require(plan == expected, "plan-metadata-or-reason-mismatch")
    return {"status": "VALID_PLAN_ONLY", "coverage_verdict": "NOT_ASSESSED"}


# Real sources below only plan. They never import gatequeue or execute a gate job.
def load_module(path, name):
    import importlib.util
    import sys
    sys.dont_write_bytecode = True
    spec = importlib.util.spec_from_file_location(name, path)
    require(spec is not None and spec.loader is not None, 'module-unavailable ' + str(path))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def bounded(root, argv, input=None):
    import subprocess
    import sys
    p = subprocess.run([sys.executable, '-B', str(root / 'tests/bound.py'), '55', *map(str, argv)],
                       cwd=root, input=input, capture_output=True)
    require(p.returncode == 0, 'planning-tool rc=%s argv=%s stderr=%s' %
            (p.returncode, argv, p.stderr.decode('utf-8', 'replace')[-4000:]))
    return p.stdout


def parse_gate_plan(raw):
    import os
    require(raw.endswith(b'\0'), 'gate-plan-not-nul-terminated')
    fields = raw.split(b'\0'); i = 0; jobs = []
    while i < len(fields) - 1:
        require(fields[i] and i + 1 < len(fields), 'gate-plan-record')
        try:
            n = int(fields[i + 1])
        except ValueError:
            raise SelectionError('gate-plan-argc')
        require(n > 0 and i + 2 + n <= len(fields) - 1, 'gate-plan-argv-extent')
        jobs.append({'name': os.fsdecode(fields[i]), 'command': [os.fsdecode(v) for v in fields[i+2:i+2+n]]})
        i += n + 2
    require(i == len(fields) - 1 and jobs, 'gate-plan-tail-or-empty')
    names = [j['name'] for j in jobs]
    require(len(set(names)) == len(names), 'gate-plan-duplicate')
    return jobs


def parse_stage_rows(raw, names):
    mapped = {}
    for line in raw.decode().splitlines():
        cols = line.split("\t")
        require(len(cols) == 3 and cols[1] in names and cols[1] not in mapped, "stage-map-record")
        require(cols[0] and not any(c.isspace() for c in cols[0]), "stage-map-primary-shape")
        also = [] if cols[2] == "-" else cols[2].split(",")
        require(all(also) if also else cols[2] == "-", "stage-map-empty-also")
        require(not any(c.isspace() for tag in also for c in tag), "stage-map-also-space")
        require(len(set(also)) == len(also), "stage-map-duplicate-also")
        require(also == sorted(also), "stage-map-unsorted-also")
        require(cols[0] not in also, "stage-map-primary-also-duplicate")
        mapped[cols[1]] = cols[0], also
    require(list(mapped) == names, "stage-map-incomplete-or-order")
    return mapped


def parse_diff(raw):
    import os
    if not raw:
        return []
    require(raw.endswith(b'\0'), 'git-diff-not-nul-terminated')
    tokens = raw.split(b'\0')[:-1]; i = 0; rows = []
    while i < len(tokens):
        status = tokens[i].decode('ascii'); i += 1
        require(status[:1] in ('M', 'A', 'D', 'R', 'C', 'T'), 'git-diff-status ' + status)
        n = 2 if status[:1] in ('R', 'C') else 1
        require(i + n <= len(tokens), 'git-diff-path-count')
        paths = [os.fsdecode(x) for x in tokens[i:i+n]]; i += n
        rows.append({'status': 'R' if n == 2 else 'M' if status[:1] == 'T' else status[:1], 'paths': paths})
    return rows


def changed_paths(root, request):
    import os
    paths = request.get('paths', [])
    require(all(p and not p.startswith(('-', ':', '/')) and '..' not in Path(p).parts for p in paths), 'unsafe-scope-path')
    def commit(value):
        return bounded(root, ['git', 'rev-parse', '--verify', '--end-of-options', value + '^{commit}']).decode().strip()
    base, head = commit(request['base']), commit(request['head'])
    args = ['git', 'diff', '--name-status', '-z', '--find-renames', base, head, '--', *paths]
    rows = parse_diff(bounded(root, args))
    if request.get('worktree'):
        rows += parse_diff(bounded(root, ['git', 'diff', '--name-status', '-z', '--find-renames', head, '--', *paths]))
        extra = bounded(root, ['git', 'ls-files', '--others', '--exclude-standard', '-z', '--', *paths])
        rows += [{'status': '?', 'paths': [os.fsdecode(p)]} for p in extra.split(b'\0') if p]
    return base, head, rows


def path_identity(path):
    import os
    import stat
    try:
        st = path.lstat()
    except FileNotFoundError:
        return {'state': 'ABSENT'}
    if stat.S_ISLNK(st.st_mode):
        raw = os.fsencode(os.readlink(path)); kind = 'SYMLINK_TARGET'
    else:
        require(stat.S_ISREG(st.st_mode), 'nonregular-plan-input ' + str(path))
        raw = path.read_bytes(); kind = 'FILE'
    return {'state': kind, 'mode': st.st_mode & 0o777, 'sha256': hashlib.sha256(raw).hexdigest()}


def real_inputs(root, request, policy_path):
    import shutil
    import tempfile
    source_names = ['tests/gate.sh', 'tests/gatedeps.json', 'tests/gatedecl.py', 'tests/gatelayers.py',
                    'tests/stagemap.c', 'tests/gateorder.json', 'release/derived-map.tsv',
                    'release/tools/derivedcheck.py', 'tests/bound.py', 'tests/bound', 'tests/bound.c']
    if parse_policy(policy_path.read_text()).get('ask_vocabulary') is not None:
        source_names.append('kernel/unisa_model.inc')
    before = {p: path_identity(root / p) for p in source_names}
    before['SELECTOR'] = path_identity(Path(__file__).resolve())
    before['POLICY'] = path_identity(policy_path)
    checkout_head = bounded(root, ['git', 'rev-parse', '--verify', 'HEAD']).decode().strip()
    base, head, changes = changed_paths(root, request)
    changed = sorted({p for c in changes for p in c['paths']})
    contents = {p: path_identity(root / p) for p in changed} if request.get('worktree') else {}
    gate = ['sh', 'tests/gate.sh', '--plan'] + (['--com'] if request.get('com') else [])
    raw = bounded(root, gate); jobs = parse_gate_plan(raw)
    names = [j['name'] for j in jobs]
    # Classifications come only from their existing owners, never from suite-name heuristics here.
    layers = load_module(root / 'tests/gatelayers.py', '_selector_layers')
    decl = load_module(root / 'tests/gatedecl.py', '_selector_declarations')
    for j in jobs:
        j['layer'] = layers.layer(j['name'])
    cc = shutil.which('cc')
    require(cc is not None, 'stage-mapper-compiler-missing')
    with tempfile.TemporaryDirectory(prefix='gateselect-stagemap-') as tmp:
        mapper = Path(tmp) / 'stagemap'
        bounded(root, [cc, '-std=c99', 'tests/stagemap.c', '-o', mapper])
        stage_bytes = bounded(root, [mapper, '--list'], input=('\n'.join(names)+'\n').encode())
    mapped = parse_stage_rows(stage_bytes, names)
    deps = json.loads((root / 'tests/gatedeps.json').read_text())
    after = json.loads((root / 'tests/gateorder.json').read_text()).get('after', {})
    require(isinstance(deps.get('suites'), dict) and isinstance(deps.get('families'), dict), 'declarations-shape')
    hit_reasons = {}; unclaimed = set(changed); affected = set()
    for j in jobs:
        n = j['name']; entry = deps['suites'].get(n)
        if entry is not None:
            require(isinstance(entry, dict), 'declaration-entry-shape ' + n)
            if entry.get('family'):
                require(entry['family'] in deps['families'], 'declaration-family-missing ' + n)
            # Raw suite entry, not effective_entry: selection conservatively unions suite/family.
            for p in changed:
                why = decl.declared_reasons(entry, deps['families'], p)
                if why:
                    affected.add(n); unclaimed.discard(p)
                    hit_reasons.setdefault(n, []).extend('changed:%s:%s' % (w, p) for w in why)
        j['declaration'] = 'known' if entry is not None else 'unknown'
        j['primary'], j['also'] = mapped[n]
        prior = after.get(n, [])
        j['after'] = [prior] if isinstance(prior, str) else prior
    derived = load_module(root / 'release/tools/derivedcheck.py', '_selector_derived')
    derived.MAP = root / 'release/derived-map.tsv'
    derived_rows = []
    for c in changes:
        if c['status'] == 'R':
            derived_rows += [('D', c['paths'][0]), ('A', c['paths'][1])]
        else:
            derived_rows += [('A' if c['status'] == '?' else c['status'], c['paths'][0])]
    suites, derived_why = derived.suites_for(derived_rows)
    require(all(n in deps['suites'] for n in suites), 'derived-suite-undeclared')
    # Metadata invalidation is intentionally broad; it does not change gatequeue result identities.
    metadata = set(source_names[:6]) | {'tests/gateselect.py', 'tests/gateselect.tsv', 'release/derived-map.tsv'}
    f = {'mode': 'FIXTURE_ONLY', 'catalog': {'layers': list(layers.LAYERS),
         'stages': sorted({tag for primary, also in mapped.values() for tag in [primary, *also]})},
         'jobs': jobs, 'changes': changes, 'affected': sorted(affected), 'unclaimed': sorted(unclaimed),
         'metadata_changed': bool(set(changed) & metadata)}
    policy = parse_policy(policy_path.read_text())
    if policy.get('ask_vocabulary') is not None:
        f['ask_catalog_sha256'] = bind_ask_catalog(policy, (root / 'kernel/unisa_model.inc').read_bytes())
    validate_fixture(f)
    after_read = {p: path_identity(root / p) for p in source_names}
    require(all(before[p] == after_read[p] for p in source_names), 'planning-inputs-changed')
    require(before['POLICY'] == path_identity(policy_path) and before['SELECTOR'] == path_identity(Path(__file__).resolve()), 'planning-tool-changed')
    if request.get('worktree'):
        require(contents == {p: path_identity(root / p) for p in changed}, 'planning-worktree-changed')
    require(checkout_head == bounded(root, ['git', 'rev-parse', '--verify', 'HEAD']).decode().strip(), 'planning-checkout-head-changed')
    identity = {'checkout_head': checkout_head, 'base': base, 'head': head, 'worktree': bool(request.get('worktree')), 'paths': request.get('paths', []),
                'source_files': before, 'worktree_files': contents, 'gate_plan_sha256': hashlib.sha256(raw).hexdigest(),
                'stage_output_sha256': hashlib.sha256(stage_bytes).hexdigest(), 'compiler': path_identity(Path(cc).resolve())}
    return f, identity, hit_reasons, {'status': 'PENDING_NOT_EXECUTED', 'suites': suites, 'reasons': derived_why}


def real_plan(root, request, policy_path, tier, filters):
    require(tier not in ("candidate", "release") or request.get("com"), "full-tier-requires-com")
    policy = parse_policy(policy_path.read_text())
    f, identity, changed_reasons, derived = real_inputs(root, request, policy_path)
    # The pure selector still operates on normalized inputs, now obtained exclusively from owners.
    selected = select(f, policy, tier, filters.get('layers', []), filters.get('stages', []), filters.get('asks', []))
    verify(selected, f, policy)
    selected['format'] = 'GATESELECT1'; selected['mode'] = 'REAL_PLAN_ONLY'
    selected['adapter'] = 'tests/gatedecl.py:declared_reasons(raw_suite,families,path)'
    selected['identity'] = identity
    selected['request'] = dict(request, base=identity['base'], head=identity['head'])
    selected['pending_derived'] = derived
    for n, reasons in changed_reasons.items():
        if n in selected['reasons']:
            selected['reasons'][n] = sorted(set(selected['reasons'][n]) | set(reasons))
    return selected


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--fixture', type=Path)
    ap.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[1])
    ap.add_argument('--policy', type=Path, default=Path(__file__).with_suffix('.tsv'))
    ap.add_argument('--tier', choices=('dev', 'merge', 'candidate', 'release', 'inspect'), default='dev')
    ap.add_argument('--layer', action='append', default=[])
    ap.add_argument('--stage', action='append', default=[])
    ap.add_argument('--ask', action='append', default=[])
    ap.add_argument('--base'); ap.add_argument('--head'); ap.add_argument('--worktree', action='store_true')
    ap.add_argument('--path', action='append', default=[])
    ap.add_argument('--com', action='store_true')
    ap.add_argument('--verify', type=Path); ap.add_argument('--out', type=Path)
    a = ap.parse_args()
    try:
        policy_path = a.policy.resolve()
        if a.fixture:
            f = json.loads(a.fixture.read_text()); p = parse_policy(policy_path.read_text())
            result = verify(json.loads(a.verify.read_text()), f, p) if a.verify else select(f, p, a.tier, a.layer, a.stage, a.ask)
        elif a.verify:
            require(not (a.base or a.head or a.worktree or a.path or a.layer or a.stage or a.ask), 'verify-request-conflict')
            old = json.loads(a.verify.read_text()); require(old.get('format') == 'GATESELECT1', 'plan-format')
            expected = real_plan(a.root.resolve(), old['request'], policy_path, old['tier'], old['filters'])
            require(old == expected, 'plan-changed-or-tampered')
            result = {'status': 'VALID_PLAN_ONLY', 'coverage_verdict': 'NOT_ASSESSED'}
        else:
            require(a.base and a.head, 'real-plan-needs-explicit-base-head')
            request = {'base': a.base, 'head': a.head, 'worktree': a.worktree, 'com': a.com, 'paths': a.path}
            result = real_plan(a.root.resolve(), request, policy_path, a.tier, {'layers': a.layer, 'stages': a.stage, 'asks': a.ask})
        text = json.dumps(result, indent=2) + '\n'
        if a.out:
            a.out.write_text(text)
        else:
            print(text, end='')
    except (SelectionError, ValueError, OSError, KeyError, TypeError, AttributeError) as e:
        ap.exit(2, 'gateselect: ' + str(e) + '\n')


if __name__ == '__main__':
    main()
