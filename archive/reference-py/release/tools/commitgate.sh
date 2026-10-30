#!/bin/bash
# commitgate.sh -m MSG [--suite NAME]... -- PATH... (0.0.39): the recommended commit path.  Runs the named gate
# suites (default: the light infra set) through checkrun.sh, so the decision is the gate's own exit status, and
# commits the given paths only when it is 0.  A red gate -- whatever its log tail says -- leaves no commit.
# COMMITGATE_GATE replaces tests/gate.sh only together with COMMITGATE_SELFTEST=1 (tests/commitgatecheck.sh).
#
# commitgate.sh --index -m MSG [--suite NAME]... -- PATH... (0.0.43): commit exactly what is in the INDEX, so several
# landers can share a file (tests/gate.sh, gatelayers.py, stagemap.c, INVENTORY.md, gatedeps.json, zeropython.tsv,
# glossary-*.tsv) without committing, or waiting for, each other's uncommitted lines.  The lander puts only its own
# hunks into the index (git apply --cached MY.patch, plus git apply MY.patch for its working tree; git add for its new
# files).  commitgate refuses (rc 2) an empty index, unmerged entries, and any indexed change outside the declared
# PATH list (somebody else's work: it is never committed under this message); it runs the gate in the export worktree
# GIT_COMMON_DIR/unisacc-index-tree checked out at the index tree (tests/landlock --export) and the derived check on the
# index (derivedcheck.py without --worktree); index tree or HEAD moved during the checks -> rc 4; then git commit with
# no pathspec, and the commit's tree must be the checked one.  Working-tree bytes are never read or committed.
#
# Landing lock (0.0.43, both modes): the whole run (staging check -> gate -> commit) holds flock on
# GIT_COMMON_DIR/unisacc-landing.lock through tests/landlock, waiting up to COMMITGATE_LOCK_WAIT minutes (default 30)
# with a message naming the holder; not free in time -> rc 6, nothing committed.  Under `tests/landlock CMD` the lock
# is already held (descriptor 9, UNISACC_LANDLOCK_FD=9) and commitgate takes it without waiting for itself.
# Derived files under the lock (recipe; research/c43-workflow-lessons.md section 2):
#   tests/landlock sh -c 'git apply --cached MY.patch && git apply MY.patch &&
#       tests/landlock --derive &&
#       release/tools/commitgate.sh --index -m MSG --suite S... -- PATH... tests/INVENTORY.md tests/gatedeps.json tests/zeropython.tsv'
# where --derive is, in the export worktree X=$(tests/landlock --export) of the index tree:
#   (cd X && sh ./unisacc.com -run tests/inventory.cx -- --write && git add tests/INVENTORY.md &&
#            sh ./unisacc.com -run tests/refreshgatedeps.cx -- --index && git add tests/gatedeps.json &&
#            sh tests/cxbin tests/zeropython.cx --write)
#   tests/landlock --add tests/INVENTORY.md tests/gatedeps.json tests/zeropython.tsv   (export bytes -> index; the same
#            change merged into the working-tree copies, others' uncommitted lines kept)
set -u
R=$(cd "$(dirname "$0")/../.." && pwd)
if [ "${UNISACC_LANDLOCK_FD:-}" = 9 ] && [ /dev/fd/9 -ef "$(git rev-parse --path-format=absolute --git-common-dir 2>/dev/null)/unisacc-landing.lock" ] \
   && flock -n 9 2>/dev/null; then
  unset UNISACC_LANDLOCK_REEXEC   # the landing lock is held on descriptor 9 (by tests/landlock, ours or a caller's)
elif ! command -v flock > /dev/null 2>&1; then
  echo "commitgate: flock not found, the landing lock is NOT taken" >&2
elif git rev-parse --git-dir > /dev/null 2>&1; then
  [ -z "${UNISACC_LANDLOCK_REEXEC:-}" ] || { echo "commitgate: tests/landlock ran but the landing lock is not held on descriptor 9" >&2; exit 2; }
  UNISACC_LANDLOCK_REEXEC=1 exec sh "$R/tests/landlock" bash "$0" "$@"
fi
msg=; suites=(); index=0
while [ $# -gt 0 ]; do
  case $1 in
    --index) index=1; shift;;
    -m) msg=${2:?message}; shift 2;;
    --suite) suites+=(--suite "${2:?suite}"); shift 2;;
    --) shift; break;;
    *) echo "commitgate: unknown argument $1" >&2; exit 2;;
  esac
done
[ -n "$msg" ] && [ $# -gt 0 ] || { echo "commitgate: need -m MSG and -- PATH..." >&2; exit 2; }
# default set: infrastructure only (机房主任 ruling B).  A product change (freezecheck.product closure, plus unisacc.c) must name at least one
# suite outside the contract layer, or nothing is checked for it and nothing is committed.
# Resolve scoped directories/pathspecs against tracked and untracked paths, using
# the same product predicate as the freeze closure. Keep standalone unisacc.c.
# 0.0.40-prep (机房主任 18:28): every shipping path in the commit must be declared as an input of at least
# one NAMED suite (tests/gatedeps.json files/trees, or its family's); "any non-contract suite" is not enough.
# Unknown or unclassified suites fail closed.  The classifier is tests/shipclassify.cx (no Python on the commit path),
# compiled once per source/compiler by tests/cxbin (its -run compile is 3.4 s, its run 2 ms).
named=$(printf '%s\n' ${suites[@]+"${suites[@]}"} | grep -v '^--suite$' || true)
sh "$R/tests/cxbin" "$R/tests/shipclassify.cx" --commitgate "$R" "$named" "$@" || exit 2
[ ${#suites[@]} -gt 0 ] || suites=(--suite gate-layers --suite script-inventory --suite checkrun)
gate=$R/tests/gate.sh
if [ -n "${COMMITGATE_GATE:-}" ]; then
  [ "${COMMITGATE_SELFTEST:-}" = 1 ] || { echo "commitgate: COMMITGATE_GATE is for the self-test only" >&2; exit 2; }
  gate=$COMMITGATE_GATE
fi
# the gate checks this repository, so the commit must land in it too (a scratch repo only under the self-test)
top=$(git rev-parse --show-toplevel 2>/dev/null) || { echo "commitgate: not in a git repository" >&2; exit 2; }
[ "$top" = "$R" ] || [ "${COMMITGATE_SELFTEST:-}" = 1 ] || { echo "commitgate: cwd repository $top is not the checked repository $R" >&2; exit 2; }
log=$(mktemp "${TMPDIR:-/tmp}/commitgate.XXXXXX")
gdir=.; dsel=(--worktree -- "$@"); cpath=(-- "$@")   # default: the gate from the cwd, the working-tree bytes of PATH...
if [ "$index" = 1 ]; then   # --index: what is in the index, all of it inside PATH..., checked in its own export
  [ -z "$(git ls-files -u)" ] || { echo "commitgate --index: the index has unmerged entries, nothing committed" >&2; exit 2; }
  all=$(git diff --cached --no-renames --name-only HEAD --) && mine=$(git diff --cached --no-renames --name-only HEAD -- "$@") \
    || { echo "commitgate --index: cannot read the index" >&2; exit 2; }
  [ -n "$all" ] || { echo "commitgate --index: nothing in the index to commit" >&2; exit 2; }
  if [ "$all" != "$mine" ]; then
    echo "commitgate --index: the index holds changes outside the declared paths (another lander's?), nothing committed:" >&2
    printf '%s\n' "$all" | grep -vxF -e "$mine" | sed 's/^/  /' >&2; exit 2
  fi
  head=$(git rev-parse HEAD) && tree=$(git write-tree) || { echo "commitgate --index: cannot read HEAD or the index tree" >&2; exit 2; }
  gdir=$(sh "$R/tests/landlock" --export) || { echo "commitgate --index: cannot export the index tree" >&2; exit 2; }
  [ -n "${COMMITGATE_GATE:-}" ] || gate=$gdir/tests/gate.sh
  dsel=(); cpath=()
fi
inputs() { if [ "$index" = 1 ]; then echo "$(git rev-parse HEAD) $(git write-tree)"; else snap "$@"; fi; }
# 0.0.40-prep (机房主任 18:28): the committed inputs must be the checked inputs -- index entry (mode/blob/stage),
# working-tree bytes and mode, and membership of every given path are snapshotted before and after the gate.
# Boundary (named): only the COMMITTED pathspecs are bound; a suite's other inputs, its gatedeps declaration and
# its tools are not snapshotted here.  Coverage is declaration-level (gatedeps), not proof of assertion
# semantics or of a valid reviewed stamp.
snap() { python3 - "$@" <<'PY_SNAP'
import hashlib, os, subprocess, sys
h = hashlib.sha256()
h.update(subprocess.check_output(['git','ls-files','-s','-z','--'] + sys.argv[1:]))
for p in sorted(set(subprocess.check_output(['git','ls-files','-z','--cached','--others','--exclude-standard','--'] + sys.argv[1:]).decode().split('\0')) - {''}):
    h.update(p.encode() + b'\0')
    try:
        st = os.lstat(p); h.update(oct(st.st_mode).encode())
        h.update(os.readlink(p).encode() if os.path.islink(p) else open(p, 'rb').read())
    except FileNotFoundError: h.update(b'<absent>')
print(h.hexdigest())
PY_SNAP
}
before=$(inputs "$@") || { echo "commitgate: cannot snapshot inputs" >&2; exit 2; }
(cd "$gdir" && exec "$R/release/tools/checkrun.sh" "$log" -- "$gate" ${suites[@]+"${suites[@]}"}) 9>&-; gate_rc=$?   # 9: the landing lock stays out of the gate
# Shadow only. The existing selected suites, exit status, coverage and derived gate
# remain authoritative. Scratch self-tests have no real gate universe to compare.
if [ "$index" = 1 ]; then
  echo "commitgate: gateselect shadow UNAVAILABLE (--index; original gate retained)"
elif [ "$top" = "$R" ] && [ "${COMMITGATE_SELFTEST:-}" != 1 ]; then
  shadow_actual=(); shadow_paths=()
  for n in "${suites[@]}"; do [ "$n" = --suite ] || shadow_actual+=(--actual-suite "$n"); done
  for p in "$@"; do shadow_paths+=(--path "$p"); done
  if ! env UNISACC_TIME_SCALE=1 python3 "$R/tests/bound.py" 5 python3 "$R/tests/gateselect.py" --root "$R" \
      --com --tier dev --base HEAD --head HEAD --worktree "${shadow_paths[@]}" \
      --shadow-report --gate-log "$log" "${shadow_actual[@]}" --out "$log.shadow.json" 9>&-; then
    echo "commitgate: gateselect shadow UNAVAILABLE (5s budget or planning refusal); original gate rc=$gate_rc" >&2
  fi
else
  echo "commitgate: gateselect shadow UNAVAILABLE (scratch self-test; original gate retained)"
fi
[ "$gate_rc" -eq 0 ] || { echo "commitgate: gate rc=$gate_rc, nothing committed (log $log)" >&2; exit 1; }
after=$(inputs "$@") || { echo "commitgate: cannot snapshot inputs" >&2; exit 2; }
[ "$before" = "$after" ] || { echo "commitgate: inputs changed while the gate ran ($before -> $after), nothing committed (log $log)" >&2; exit 4; }
# 0.0.43 step 4 (research/c43-derived-consistency.md): the derived files of the committed paths must agree with their
# sources.  release/tools/derivedcheck.py maps the paths to their derived-consistency suites (release/derived-map.tsv),
# runs them in a clean export of HEAD with these paths' working-tree bytes, and fails on any non-zero rc.  It is a
# separate step: every step inside it (the export's git calls and each mapped suite) has its own bound of at most 55 s,
# so a commit that maps several suites is not limited by one outer bound; an empty mapping costs one git call.
dlog=$(mktemp "${TMPDIR:-/tmp}/derivedcheck.XXXXXX")
# --index: no --worktree, so derivedcheck overlays the index on HEAD (its default mode).
python3 -I "$R/release/tools/derivedcheck.py" --root "$top" ${dsel[@]+"${dsel[@]}"} > "$dlog" 2>&1 9>&- \
  || { echo "commitgate: derived consistency refused, nothing committed (log $dlog)" >&2; cat "$dlog" >&2; exit 5; }
if [ "$index" = 1 ]; then
  [ "$before" = "$(inputs)" ] || { echo "commitgate --index: the index or HEAD moved during the derived check, nothing committed (log $log)" >&2; exit 4; }
fi
UNISACC_COMMITGATE_RUN=1 git commit -q -m "$msg" ${cpath[@]+"${cpath[@]}"} 9>&- || { echo "commitgate: gate was green but git commit failed" >&2; exit 3; }
if [ "$index" = 1 ] && { [ "$(git rev-parse HEAD^{tree})" != "$tree" ] || [ "$(git rev-parse HEAD^)" != "$head" ]; }; then
  echo "commitgate --index: committed $(git rev-parse --short HEAD), but it is not the checked index tree on $head -- inspect it (log $log)" >&2; exit 4
fi
echo "commitgate: committed $(git rev-parse --short HEAD) after gate rc=0 (log $log)"
