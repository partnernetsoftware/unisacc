#!/bin/bash
# Fault injection: equal image bytes do not excuse a failed compiler process.
set -eu
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
. "$R/tests/lib.sh"
[ -x "$UA" ] || { echo 'nativebootcheck requires an existing UA'; exit 1; }
T=$(scratch)
H=$(host_target)
BAD=lnx/x86_64; [ "$H" != "$BAD" ] || BAD=lnx/arm64
cat > "$T/compiler" <<'EOF'
#!/bin/sh
"$NATIVEBOOT_REAL_UA" "$@"
rc=$?
[ "$rc" -eq 0 ] || exit "$rc"
[ "$3" != "$NATIVEBOOT_FAIL_TARGET" ] || exit 7
EOF
chmod +x "$T/compiler"
if bound 55 env UA="$T/compiler" NATIVEBOOT_REAL_UA="$UA" NATIVEBOOT_FAIL_TARGET="$BAD" \
    bash tests/nativeboot.sh > "$T/log" 2>&1; then
    cat "$T/log"; echo 'FAIL nativeboot accepted a nonzero compiler with valid image bytes'; exit 1
fi
grep -F "FAIL compile $BAD exited 7" "$T/log" >/dev/null || { cat "$T/log"; exit 1; }
printf '#!/bin/sh\nexit 0\n' > "$T/empty"; chmod +x "$T/empty"
if bound 10 env UA="$T/empty" bash tests/nativeboot.sh > "$T/log" 2>&1; then
    echo 'FAIL nativeboot accepted an empty successful compile'; exit 1
fi
grep -F 'produced an empty image' "$T/log" >/dev/null || { cat "$T/log"; exit 1; }
echo 'nativeboot fault controls: valid bytes + exit 7 rejected; empty + exit 0 rejected'
