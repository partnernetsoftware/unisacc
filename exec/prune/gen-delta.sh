#!/bin/sh
# K5-1g: construct prune δ JSON. Prefer seed/gen.c (seed-gen); Python remains the independent byte reference
# (SEED_GEN=0). On seed-gen failure do not silently fall back to gen.py.  Consumer: exec/pipeline/prepare.sh.
# 0.0.40 (机房主任 04:09): prune's manifest declares no flag, so this helper takes OUT and nothing else (any flag
# rc 2).  SEED_GEN is 0 or 1 (anything else rc 2).  Same keyed, sha-checked default seed-gen build as the opt and
# pp helpers.  Differences from exec/opt/gen-delta.sh: argument check (no flag at all vs at most one --o2) and the
# stage name passed to the generator; from exec/pp/gen-delta.sh: no flag whitelist, no --osx/--win exclusion.
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
usage() { echo "usage: exec/prune/gen-delta.sh OUT.json" >&2; exit 2; }
[ $# -eq 1 ] || usage
OUT=$1; shift
B=$R/tests/bound
case "${SEED_GEN-1}" in
    0) exec "$B" 55 python3 exec/build/gen.py prune "$OUT";;
    1) ;;
    *) echo "exec/prune/gen-delta.sh: SEED_GEN must be 0 or 1, got '${SEED_GEN}'" >&2; exit 2;;
esac
if [ -n "${SEED_GEN_BIN:-}" ]; then
    # explicit binary: fail closed if missing/unusable (no rebuild, no Python)
    [ -x "$SEED_GEN_BIN" ] || { echo "exec/prune/gen-delta.sh: SEED_GEN_BIN not executable: $SEED_GEN_BIN" >&2; exit 2; }
    G=$SEED_GEN_BIN
else
    CC=${SEED_GEN_CC:-cc}; FLAGS="-std=c99 -O2 -w"
    # one process computes the key, so no failing step hides behind a pipeline's last status
    key=$(python3 - "$CC" "$FLAGS" <<'PY'
import glob, hashlib, os, shutil, subprocess, sys
cc, flags = sys.argv[1], sys.argv[2]
path = shutil.which(cc)
if not path: sys.exit('no compiler %s' % cc)
real = os.path.realpath(path)
v = subprocess.run([path, '--version'], capture_output=True)
if v.returncode != 0: sys.exit('%s --version rc %d' % (cc, v.returncode))
h = hashlib.sha256()
for part in (real.encode(), open(real, 'rb').read(), flags.encode(), v.stdout):
    h.update(hashlib.sha256(part).digest())
srcs = sorted(glob.glob('seed/*.c') + glob.glob('seed/*.h'))
if not srcs: sys.exit('no seed sources')
for f in srcs: h.update(f.encode() + b'\0' + hashlib.sha256(open(f, 'rb').read()).digest())
print(h.hexdigest()[:16])
PY
) || { echo "exec/prune/gen-delta.sh: cannot key the seed-gen build" >&2; exit 2; }
    _td=${TMPDIR:-/tmp}; D=${SEED_GEN_DIR:-${_td%/}/unisacc-seedbin}   # trailing / of TMPDIR dropped, as exec/stamp.sh does
    mkdir -p "$D"
    G=$D/seed-gen-$key
    sha() { python3 -c 'import hashlib,sys;print(hashlib.sha256(open(sys.argv[1],"rb").read()).hexdigest())' "$1"; }
    # a cached binary is used only with its recorded sha; anything else is rebuilt
    if [ ! -x "$G" ] || [ ! -f "$G.sha256" ] || [ "$(sha "$G" 2>/dev/null)" != "$(cat "$G.sha256")" ]; then
        rm -f "$G" "$G.sha256"
        "$B" 55 "$CC" $FLAGS -I"$R/seed" "$R/seed/gen.c" -o "$G.$$" \
            && sha "$G.$$" > "$G.sha256.$$" && mv -f "$G.$$" "$G" && mv -f "$G.sha256.$$" "$G.sha256" \
            || { rm -f "$G.$$" "$G.sha256.$$"; echo "exec/prune/gen-delta.sh: seed-gen build failed" >&2; exit 1; }
    fi
fi
# prefer seed-gen; failure is failure (no Python masquerade)
exec "$B" 55 "$G" prune "$OUT"
