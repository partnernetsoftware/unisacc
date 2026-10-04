#!/bin/bash
# Bounded LOCAL acceptance of an already-built immutable model candidate.
# MODEL_COM=/absolute/candidate GATE_STATE=/private/queue UA=/private/reference \
#   SEED_DIR=/private/seed ./tests/release.sh [--com]
# The ONE supported shape (R14-2 5): --jobs 4 --window 55 (50 until 0.0.21), run through
# tests/term.sh with every variable given as `env NAME=value`; SEED_DIR holds
# unisacc-seed.com (make seed-com) so the four com-comboot jobs verify instead
# of skipping -- STRICT=1 turns a skipped comboot job into a failure.
# Repeat the identical command after rc=75 (pending). Each invocation <=55s;
# queue work gets 50s, leaving time for input checks and final copy verification.
# Optional RELEASE_OUT receives this exact candidate only after local completion.
# rc=0 means local gate complete, NOT release ready: six-platform model/native/
# bootstrap evidence is external and is neither inferred nor marked passed here.
# No VM startup, building, tagging, uploading, or default product replacement.
set -eu
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
case "${1:-}" in
    --help) head -11 "$0"; exit 0;;
    ''|--com) [ "$#" -le 1 ] || exit 2;;
    *) echo 'usage: MODEL_COM=file GATE_STATE=dir UA=reference tests/release.sh [--com]' >&2; exit 2;;
esac
[ "${SUITES:-1}" = 1 ] || { echo 'release: SUITES=0 cannot establish acceptance' >&2; exit 2; }
: "${MODEL_COM:?explicit already-built candidate required}"
: "${GATE_STATE:?explicit persistent private queue directory required}"
: "${UA:?explicit existing private reference required}"
: "${SEED_DIR:?explicit seed directory required (make seed-com SEED_DIR=...); comboot jobs fail under STRICT=1 without it}"
[ -s "$SEED_DIR/unisacc-seed.com" ] || { echo "release: no $SEED_DIR/unisacc-seed.com" >&2; exit 2; }
[ -f "$MODEL_COM" ] && [ -x "$MODEL_COM" ] && [ -s "$MODEL_COM" ] || { echo 'release: missing/empty/non-executable MODEL_COM' >&2; exit 2; }
[ "$UA" != /tmp/ua_ref ] && [ -f "$UA" ] && [ -x "$UA" ] || { echo 'release: UA must be an existing private reference' >&2; exit 2; }
if [ "${RELEASE_BOUND:-0}" != 1 ]; then
    exec python3 "$R/tests/bound.py" 55 env RELEASE_BOUND=1 "$0" "$@"
fi
MODEL_COM=$(cd "$(dirname "$MODEL_COM")" && printf '%s/%s' "$PWD" "$(basename "$MODEL_COM")")
export MODEL_COM UA SEED_DIR STRICT=1
before=$(shasum -a 256 "$MODEL_COM"); before=${before%% *}
printf 'local candidate: %s sha256 %s\n' "$MODEL_COM" "$before"
# Invoke through tests/term.sh externally if desired; env arguments preserve
# MODEL_COM explicitly across Terminal's whitelist. Never call ua_ready here.
rc=0
exclusive=()  # fat is three ~17 s shards since 0.0.13 and no longer needs a window of its own
# These pass alone (6-33 s) but hit their bounds under --jobs 4 in every 0.0.13
# release queue: a cold network build (bindprep-arm64, memx86-ua-1) and an
# inner 20 s hard timeout (lib-carrier-import-model).  They get a window each.
# 0.0.21: exec-tableself (the self-source through seven table stages) reached 48 s alone and the
# Python image step of each bigclosure target exceeds its 45 s bound under contention; each gets a window
for s in exec-memx86-ua-1 lib-carrier-import-model exec-tableself bigclosure-osx-arm64 bigclosure-osx-x86_64 bigclosure-lnx-arm64 bigclosure-lnx-x86_64 bigclosure-win-arm64 bigclosure-win-x86_64; do exclusive+=(--exclusive-suite "$s"); done
if [ "$(uname -s)" = Darwin ]; then exclusive+=(--exclusive-suite exec-bindprep-arm64); fi
for flag in Wall Wextra Werror; do exclusive+=(--exclusive-suite "exec-warningdriver-ua-$flag"); done
# gate.sh registers the x86 assembly binding suite only on Darwin.
# the binding check is four jobs since R12-0 ②; the x86_64 prep step is the one that needs a warm cache
if [ "$(uname -s)" = Darwin ]; then exclusive+=(--exclusive-suite exec-bindprep-x86_64); fi
# R17-6 E1: the networks these suites build cold took 48 s+ in the first window
# of every queue (0.0.16: exec-bindprep-arm64 and exec-warningdriver-ua-Wall
# timed out once each and had to be popped by hand).  Warm them first, one
# bounded step per invocation (rc 75 like a queue window), never in the queue.
mkdir -p "$GATE_STATE"
# R18-11 ②: the queue's results are only valid for the tree it started on.
# 0.0.17 lost 93/401 to a prd.md commit made mid-queue; say so at once.
# R19-0: what invalidates the queue is a change to the declared inputs, not
# any commit (0.0.18 lost a queue to a plans/ commit).  Compare their trees.
DECLARED="src exec tests include kernel weights unisa examples unisacc.c README.md ARCHITECTURE.md AGENTS.md prd.md release scripts Makefile"
head_now=$(cd "$R" && git ls-tree HEAD -- $DECLARED 2>/dev/null | shasum -a 256 | cut -c1-64)
if [ -s "$GATE_STATE/head" ]; then
    head_was=$(cat "$GATE_STATE/head")
    if [ "$head_was" != "$head_now" ]; then
        # 0.0.21 (owner: a release run and ongoing changes must not conflict): the state
        # may continue on a newer tree.  gatequeue keeps a result only when the job's own
        # fingerprint (its declared inputs, settings and executables) is unchanged, so a
        # test or doc fix re-runs exactly the jobs it touches, not the whole queue.
        echo "release: tree changed since this state began ($head_was -> $head_now); reusing results whose job fingerprints are unchanged" >&2
        echo "$head_now" > "$GATE_STATE/head"
    fi
else
    echo "$head_now" > "$GATE_STATE/head"
fi
dirty=$(cd "$R" && git status --porcelain -- $DECLARED 2>/dev/null | head -3)
[ -z "$dirty" ] || { printf 'release: declared inputs are modified in the working tree; the queue would be invalidated:\n%s\n' "$dirty" >&2; exit 1; }
warm_done=0
# 0.0.22: the caches live in the checkout, so the markers are per checkout -- a state continued
# in a new worktree warms again (0.0.21: bindprep/warningdriver timed out cold at 53 s)
wk=$(printf '%s' "$R" | cksum | cut -d' ' -f1)
for w in 1 2 3; do
    [ -f "$GATE_STATE/warm.$w.$wk" ] && { warm_done=$w; continue; }
    case $w in
        1) [ "$(uname -s)" = Darwin ] && python3 "$R/tests/bound.py" 50 env CORE_ASM_ARCH=arm64 "$R/exec/c/asm/bindprep.sh" >/dev/null 2>&1;;
        2) [ "$(uname -s)" = Darwin ] && python3 "$R/tests/bound.py" 50 env CORE_ASM_ARCH=x86_64 "$R/exec/c/asm/bindprep.sh" >/dev/null 2>&1;;
        3) python3 "$R/tests/bound.py" 50 "$R/exec/c/warningcheck.sh" ua Wall >/dev/null 2>&1;;
    esac
    : > "$GATE_STATE/warm.$w.$wk"
    echo "warm-up $w/3 done in $R (cold model caches built outside the queue)"
    exit 75
done
python3 "$R/tests/gatequeue.py" --com --jobs "${RELEASE_JOBS:-4}" --window 55 "${exclusive[@]}" --state "$GATE_STATE" || rc=$?
after=$(shasum -a 256 "$MODEL_COM"); after=${after%% *}
[ "$before" = "$after" ] || { echo 'release: candidate changed during acceptance' >&2; exit 1; }
case "$rc" in
    75) echo "local acceptance PENDING: repeat with GATE_STATE=$GATE_STATE; release NOT ready"; exit 75;;
    0) ;;
    *) echo "local acceptance FAILED (rc=$rc); release NOT ready" >&2; exit "$rc";;
esac
python3 - "$GATE_STATE" "$MODEL_COM" "$before" "${RELEASE_OUT:-}" <<'CHECK'
import hashlib,json,pathlib,re,shutil,sys,tempfile
state,source,digest,out=sys.argv[1:]
p=pathlib.Path(state); data=json.loads((p/'results.json').read_text())
jobs,results=data['jobs'],data['results']
if not jobs or set(jobs)!=set(results) or any(r['rc'] for r in results.values()):
    raise SystemExit('release: incomplete/failed queue evidence')
# Counts and named omissions occur in gate logs; zero counts are not omissions.
zero_skip=re.compile(r'\b(?:skip|skipped)[ \t]+0+\b|\bno target skipped\b',re.I)
skip=re.compile(r'\bskip(?:ped)?\b',re.I)
# These exact notices describe Windows execution outside this LOCAL gate.
# Each notice is allowed only once, only in its owning suite; retain all other
# text for omission checks and report the outstanding obligations explicitly.
windows_run='  skip windows (-run needs the UTM machine started)'
windows_boot='  skip Windows self-build (run in CI: release-check winsuite; by hand: --windows win/arm64 and --windows win/x86_64)'
windows_ci_posix='  skip winposix (run in CI: release-check winsuite)'          # 0.0.25 X7
windows_ci_fwd='  skip Windows images (run in CI: release-check winsuite)'
outside={'run':windows_run, 'com-run':windows_run, 'nativeboot':windows_boot, 'winposix':windows_ci_posix, 'forward':windows_ci_fwd}
unverified=[]
for name in jobs:
    try:log=(p/(name+'.log')).read_text(errors='replace')
    except FileNotFoundError:raise SystemExit('release: missing evidence log: '+name)
    lines=log.splitlines()
    notice=outside.get(name)
    if notice in lines:
        if lines.count(notice)!=1:
            raise SystemExit('release: repeated omission notice: '+name)
        lines.remove(notice)
        unverified.append((name,notice.strip()))
    evidence='\n'.join(lines)
    if not evidence.strip() or skip.search(zero_skip.sub('',evidence)):
        raise SystemExit('release: empty or skipped evidence: '+name)
for name,notice in unverified:
    print('UNVERIFIED Windows obligation outside LOCAL gate:',name+':',notice)
source=pathlib.Path(source)
def sha(path):
    with path.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
if sha(source)!=digest:raise SystemExit('release: candidate changed before copy')
if out:
    dest=pathlib.Path(out);dest.mkdir(parents=True,exist_ok=True)
    with tempfile.NamedTemporaryFile(dir=dest,prefix='.candidate-',delete=False) as f:tmp=pathlib.Path(f.name)
    try:
        shutil.copyfile(source,tmp);tmp.chmod(source.stat().st_mode & 0o777)
        if sha(tmp)!=digest or sha(source)!=digest:raise SystemExit('release: copy hash mismatch')
        tmp.replace(dest/'unisacc.com')
        shutil.copyfile(pathlib.Path(str(source)+'.build.json'), dest/'unisacc.com.build.json')
    finally:tmp.unlink(missing_ok=True)
    print('local-tested candidate copied:',dest/'unisacc.com','sha256',digest)
print('LOCAL gate passed for candidate sha256',digest)
print('release NOT ready: external six-platform model execution/bootstrap evidence remains unverified here')
CHECK
