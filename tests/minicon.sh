#!/bin/sh
# minicon.sh -- 0.0.36 M1: minicon's loader (tests/minicon/loader.c) built by the system cc, the
# shipped unisacc.com (product route) and, when UA is set, the reference compiler; every build must
# behave like cc on the loader's own contract: exit codes, payload argv/exit/signal pass-through,
# and no extract directory left behind.
cd "$(dirname "$0")/.." || exit 1
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
case "$(uname -s)/$(uname -m)" in
  Darwin/arm64) cell=osx-aarch64;; Darwin/x86_64) cell=osx-x86_64;;
  Linux/aarch64) cell=lnx-aarch64;; Linux/x86_64) cell=lnx-x86_64;;
  *) echo "minicon  unsupported host"; exit 1;;
esac
mkdir -p "$T/cells/$cell"
printf '#!/bin/sh\necho "payload $#:$*"\ncase "$1" in sig) kill -TERM $$;; esac\nexit 3\n' > "$T/cells/$cell/minicon"
B="cc"; tests/bound 30 cc -std=c99 -D_DEFAULT_SOURCE -o "$T/cc" tests/minicon/loader.c || { echo "minicon  cc build failed"; exit 1; }
# the product route: MODEL_COM (the candidate in a release queue) or the installed ./unisacc.com;
# NO_COM=1 (gate job `minicon`) leaves it to `com-minicon` -- the installed .com reads this tree's include/
if [ -z "${NO_COM:-}" ]; then
    tests/bound 30 sh "${MODEL_COM:-./unisacc.com}" -o "$T/com" tests/minicon/loader.c || { echo "minicon  FAIL unisacc.com build"; exit 1; }
    B="$B com"
fi
# PREBUILT=path: a loader cross-built elsewhere (e.g. -b lnx/arm64 on the host, run in a Linux guest)
if [ -n "${PREBUILT:-}" ]; then cp "$PREBUILT" "$T/pre" && chmod +x "$T/pre" && B="$B pre"; fi
if [ -n "${UA:-}" ]; then tests/bound 30 "$UA" -o "$T/ua" tests/minicon/loader.c || { echo "minicon  FAIL UA build"; exit 1; }; B="$B ua"; fi
run() {   # run BIN CASE -> one normalized line
  b=$1; shift
  case "$1" in
    noenv) out=$(env -u MINICON_COM_CELLS tests/bound 10 "$T/$b" 2>&1); rc=$?;;
    nopay) out=$(MINICON_COM_CELLS="$T/none" tests/bound 10 "$T/$b" 2>&1); rc=$?;;
    *) out=$(MINICON_COM_CELLS="$T/cells" tests/bound 10 "$T/$b" "$@" 2>&1); rc=$?;;
  esac
  left=$(ls -d /tmp/minicon.com.*.* 2>/dev/null | wc -l | tr -d ' ')
  echo "rc=$rc left=$left $(echo "$out" | sed "s#$T#T#g; s#/tmp/minicon.com\.[0-9]*\.[A-Za-z0-9]*#X#g" | tr '\n' '|')"
}
ok=0; bad=0
for c in noenv nopay "a b" "" "sig" "x y z"; do
  ref=$(run cc $c); [ -z "${VERBOSE:-}" ] || echo "  cc [$c]: $ref"
  for b in $B; do
    [ "$b" = cc ] && continue
    got=$(run $b $c)
    if [ "$got" = "$ref" ]; then ok=$((ok+1)); else bad=$((bad+1)); echo "  WRONG $b [$c]: $got  cc: $ref"; fi
  done
done
echo "minicon  builds [$B]  cases 6  agree $ok  wrong $bad"
[ $bad -eq 0 ] && [ $ok -gt 0 ]
