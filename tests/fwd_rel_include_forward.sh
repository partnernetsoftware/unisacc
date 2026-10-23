#!/bin/sh
# Relative -include must survive the /tmp synthetic forward unit (scheme A:
# fe_units_fwd skips user opti[] for that unit only). POS + NEG same cut.
set -e
UA=${UA:?}
HERE=$(cd "$(dirname "$0")" && pwd)
BOUND=${BOUND:-$HERE/bound}
BOUND=$(cd "$(dirname "$BOUND")" && pwd)/$(basename "$BOUND")
T=$(mktemp -d)
_SD=$HERE   # the one deletion guard: argv CLI tests/safedel.cx over include/unisa_safedel.h
trap 'cd "$_SD" && sh "$_SD/cxbin" "$_SD/safedel.cx" --purge -- "${T:?}"' EXIT HUP INT TERM
mkdir -p "$T/apps/csih"
printf '/* forced */\n' > "$T/apps/csih/h.h"
# time() is undefined here → forward stub → second compile of /tmp unit
printf 'long time(long *); int printf(const char *, ...);\nint main(void){ printf("%%ld\\n", (long)time(0)); return 0; }\n' > "$T/u.c"

# POS: relative -include + forward
out=$(cd "$T" && "$BOUND" 30 "$UA" -include apps/csih/h.h u.c 2>&1) || {
  echo "FAIL POS relative -include with forward: $out" >&2; exit 1
}
case "$out" in
  *"no such file for #include"*) echo "FAIL POS still resolved under /tmp: $out" >&2; exit 1;;
esac

# POS: absolute still works
out=$(cd "$T" && "$BOUND" 30 "$UA" -include "$T/apps/csih/h.h" u.c 2>&1) || {
  echo "FAIL POS absolute -include with forward: $out" >&2; exit 1
}

# NEG: missing -include must still error (not silently skipped forever)
if cd "$T" && "$BOUND" 30 "$UA" -include apps/csih/missing.h u.c >/dev/null 2>"$T/neg.err"; then
  echo "FAIL NEG missing -include should error" >&2; exit 1
fi
if ! grep -q 'no such file for #include\|cannot open\|error:' "$T/neg.err"; then
  # forward path may print the include error on stderr; require some failure text
  if [ ! -s "$T/neg.err" ]; then
    echo "FAIL NEG missing -include: no diagnostic" >&2; exit 1
  fi
fi
echo "fwd-rel-include-forward: POS relative+forward ok; NEG missing refuses"
