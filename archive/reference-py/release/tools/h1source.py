#!/usr/bin/env python3
"""h1source.py [ROOT] -- which H1 table this version's exit table uses (0.0.40, 机房主任 22:45 A/C).

Read-only; writes nothing.  The version is src/version.h; its plan is plans/vV.md (archive/plans/vV.md once
archived).  The plan settles the H1 table in exactly one way:
  own row      a '| H1 |' row in the plan itself (exittable.h1_names must accept it)
  inherit      a line 'H1-INHERIT: PATH sha256=HEX64 host=FORM': PATH exists, its sha256 is HEX64, and it has
               an H1 row exittable accepts -- the version differs, the declaration says why it still applies
  (an explicit empty table is an own row '| H1 | EMPTY |'; nothing else means empty)
Anything else -- no plan, no row and no declaration, a wrong sha, a missing target, a target without an H1
row, two declarations -- exits 2 with the reason.  rc 0 prints 'h1source: OWN|INHERIT|EMPTY PATH'."""
import hashlib, importlib.util, pathlib, re, sys

DECL = re.compile(r'^H1-INHERIT: (\S+) sha256=([0-9a-f]{64}) host=(\S+)\s*$')


class Refused(Exception): pass


def refuse(why): raise Refused(why)


def resolve(root):
    """(kind, path relative to root, host or None) for the H1 table this version uses; raises Refused."""
    root = pathlib.Path(root).resolve()
    spec = importlib.util.spec_from_file_location('exittable', pathlib.Path(__file__).resolve().parent / 'exittable.py')
    X = importlib.util.module_from_spec(spec); spec.loader.exec_module(X)
    try:
        m = re.search(r'UNISACC_VERSION "([^"]+)"', (root / 'src/version.h').read_text())
    except OSError as e:
        refuse('src/version.h unreadable (%s)' % e)
    if not m: refuse('no UNISACC_VERSION in src/version.h')
    v = m.group(1)
    plans = [p for p in (root / ('plans/v%s.md' % v), root / ('archive/plans/v%s.md' % v)) if p.is_file()]
    if len(plans) != 1:
        refuse('version %s: want exactly one plan of plans/ and archive/plans/, found %d' % (v, len(plans)))
    plan = plans[0]; lines = plan.read_text().splitlines()
    own = [l for l in lines if l.startswith('| H1 ')]
    decl = [l for l in lines if l.startswith('H1-INHERIT:')]
    if own and decl: refuse('%s has both its own H1 row and an H1-INHERIT line' % plan.relative_to(root))
    if len(own) > 1 or len(decl) > 1: refuse('%s has more than one H1 row or H1-INHERIT line' % plan.relative_to(root))
    if own:
        try: names = X.h1_names(plan)
        except X.H1Error as e: refuse(str(e))
        return ('OWN' if names else 'EMPTY'), str(plan.relative_to(root)), None
    if not decl:
        refuse('version %s: %s has no H1 row and no H1-INHERIT declaration' % (v, plan.relative_to(root)))
    d = DECL.match(decl[0])
    if not d: refuse('malformed H1-INHERIT line (want "H1-INHERIT: PATH sha256=HEX64 host=FORM"): %r' % decl[0])
    target = root / d.group(1)
    try: data = target.read_bytes()
    except OSError as e: refuse('H1-INHERIT target %s unreadable (%s)' % (d.group(1), e))
    got = hashlib.sha256(data).hexdigest()
    if got != d.group(2): refuse('H1-INHERIT target %s sha256 %s, declared %s' % (d.group(1), got, d.group(2)))
    try: X.h1_names(target)
    except X.H1Error as e: refuse('H1-INHERIT target: %s' % e)
    return 'INHERIT', d.group(1), d.group(3)


def main(argv=None):
    a = list(sys.argv[1:] if argv is None else argv)
    try: kind, path, host = resolve(a[0] if a else '.')
    except Refused as e:
        print('h1source: REFUSED -- %s' % e, file=sys.stderr); return 2
    print('h1source: %s %s%s' % (kind, path, ' (host %s)' % host if host else ''))
    return 0


if __name__ == '__main__':
    sys.exit(main())
