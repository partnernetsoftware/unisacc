#!/bin/bash
# Bounded LOCAL acceptance of an already-built immutable model candidate.
# MODEL_COM=/absolute/candidate GATE_STATE=/private/queue UA=/private/reference \
#   ./tests/release.sh [--com]
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
[ -f "$MODEL_COM" ] && [ -x "$MODEL_COM" ] && [ -s "$MODEL_COM" ] || { echo 'release: missing/empty/non-executable MODEL_COM' >&2; exit 2; }
[ "$UA" != /tmp/ua_ref ] && [ -f "$UA" ] && [ -x "$UA" ] || { echo 'release: UA must be an existing private reference' >&2; exit 2; }
if [ "${RELEASE_BOUND:-0}" != 1 ]; then
    exec python3 "$R/tests/bound.py" 55 env RELEASE_BOUND=1 "$0" "$@"
fi
MODEL_COM=$(cd "$(dirname "$MODEL_COM")" && printf '%s/%s' "$PWD" "$(basename "$MODEL_COM")")
export MODEL_COM UA STRICT=1
before=$(shasum -a 256 "$MODEL_COM"); before=${before%% *}
printf 'local candidate: %s sha256 %s\n' "$MODEL_COM" "$before"
# Invoke through tests/term.sh externally if desired; env arguments preserve
# MODEL_COM explicitly across Terminal's whitelist. Never call ua_ready here.
rc=0
exclusive=(--exclusive-suite fat)  # bigclosure is six per-target jobs now (all.sh shape); no exclusive window
for flag in Wall Wextra Werror; do exclusive+=(--exclusive-suite "exec-warningdriver-ua-$flag"); done
# gate.sh registers the x86 assembly binding suite only on Darwin.
if [ "$(uname -s)" = Darwin ]; then exclusive+=(--exclusive-suite exec-bindx86); fi
python3 "$R/tests/gatequeue.py" --com --jobs "${RELEASE_JOBS:-4}" --window 50 "${exclusive[@]}" --state "$GATE_STATE" || rc=$?
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
windows_boot='  skip Windows self-build (not run; independent proofs: --windows win/arm64 and --windows win/x86_64)'
outside={'run':windows_run, 'com-run':windows_run, 'nativeboot':windows_boot}
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
