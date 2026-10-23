#!/bin/sh
set -eu
P=/mnt/out/cxbin-closure-probe
mkdir -p "$P/path with spaces" "$P/cache"
chmod 700 "$P/cache"
export CXBIN_CACHE="$P/cache"
cd "$P/path with spaces"
printf '#pragma unisa use direct\nint main(void) { return value(); }\n' > app.cx
printf 'int value(void) { return 1; }\n' > direct.cx
run_expect() {
  want=$1; label=$2
  set +e
  sh /mnt/tests/bound 55 sh /mnt/tests/cxbin "$P/path with spaces/app.cx" > "$P/$label.log" 2>&1
  rc=$?
  set -e
  printf '%s\t%s\t%s\n' "$label" "$rc" "$want"
  test "$rc" = "$want"
  printf '%s\n' "$P/cache/"* > "$P/$label.bins"
}
run_expect 1 direct-first
run_expect 1 direct-hit
cmp "$P/direct-first.bins" "$P/direct-hit.bins"
printf 'int value(void) { return 2; }\n' > direct.cx
run_expect 2 direct-body
if cmp -s "$P/direct-first.bins" "$P/direct-body.bins"; then exit 1; fi
printf '#pragma unisa use leaf\nint value(void) { return leafvalue(); }\n' > direct.cx
printf 'int leafvalue(void) { return 3; }\n' > leaf.cx
run_expect 3 transitive-first
printf 'int leafvalue(void) { return 4; }\n' > leaf.cx
run_expect 4 transitive-body
if cmp -s "$P/transitive-first.bins" "$P/transitive-body.bins"; then exit 1; fi
run_expect 4 transitive-hit
cmp "$P/transitive-body.bins" "$P/transitive-hit.bins"
printf '#pragma unisa use absent\nint main(void) { return 0; }\n' > app.cx
run_expect 2 missing-module
