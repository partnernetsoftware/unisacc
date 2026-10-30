#!/bin/sh
# csih CX gate (仓库根旁 apps/csih 执行，或 ./check.sh)。任一项失败即 rc=1。
# 产品源已是 .cx；本脚本只做轻量卫生 + csih.sh selftest + （可选）cc fuzz。
# 完整模块闸：UNISACC=…/unisacc.com sh ./unisacc.com -run check.cx -- modules|loop|probes
ROOT=$(CDPATH= cd -- "$(dirname "$0")" && pwd) || exit 1
cd "$ROOT" || exit 1
R=$(cd "$ROOT/../.." && pwd) || exit 1
TMP=$(mktemp -d) || exit 1
trap '[ -z "$TMP" ] || sh "$R/tools/cxbin" "$R/tools/safedel.cx" --purge -- "$TMP"' EXIT
rc=0
ITERS=${ITERS:-200000}
CFLAGS="-std=c99 -g -O1 -fsanitize=address,undefined -fno-sanitize-recover=undefined -w -I$ROOT"
# Prefer native ELF so selftest/context-index stay under 60s (cold -run ~47s).
if [ -z "$CSIH_BIN" ] && [ -x "$R/out/csih-e2e-bin/csih" ]; then
    CSIH_BIN=$R/out/csih-e2e-bin/csih
fi
[ -n "$CSIH_BIN" ] && export CSIH_BIN
UNISACC=${UNISACC:-$R/unisacc.com}
export UNISACC
# Rebuild ELF when missing (prepare); nested e2e/check expect it.
if [ ! -x "$CSIH_BIN" ]; then
    mkdir -p "$R/out/csih-e2e-bin"
    echo "check.sh: building CSIH_BIN=$R/out/csih-e2e-bin/csih"
    if ! sh "$UNISACC" "$ROOT/csih.cx" -o "$R/out/csih-e2e-bin/csih"; then
        echo "FAIL 0: cannot build CSIH_BIN"; rc=1
    else
        CSIH_BIN=$R/out/csih-e2e-bin/csih; export CSIH_BIN
    fi
fi

# 1. product roots must not be leftover .c
if find . -maxdepth 1 -name '*.c' | grep -q .; then
    echo "FAIL 1: apps/csih/*.c still present (product must be .cx)"; find . -maxdepth 1 -name '*.c'; rc=1
else
    echo "ok   1: no product-root .c"
fi

# 2. selftest via CX entry
if ./csih.sh selftest > "$TMP/selftest.log" 2>&1 && tail -1 "$TMP/selftest.log" | grep -q 'selftest ok'; then
    echo "ok   2: selftest ok"
else
    echo "FAIL 2: selftest (see below)"; tail -20 "$TMP/selftest.log"; rc=1
fi

# 3. fuzz under ASan+UBSan. cc only takes .c, so copy .cx providers to $TMP as .c.
cp json.cx "$TMP/json.c" && cp csih_message.cx "$TMP/csih_message.c" && cp csih_message.h "$TMP/" \
  && cp fuzz/fuzz_json.cx "$TMP/fuzz_json.c" && cp fuzz/fuzz_msg.cx "$TMP/fuzz_msg.c" || rc=1
# Fuzz wants ASan/UBSan for memory bugs; leak exit is noise on intentional short runs.
# Library TUs use CX prelude (cx.h) for libc types; drivers -include the .h API only —
# json.cx/csih_message.cx already define those types for CXIF (cannot also -include .h).
export ASAN_OPTIONS="${ASAN_OPTIONS:+$ASAN_OPTIONS:}detect_leaks=0"
CXINC="-include $R/include/cx.h"
# json and csih_message reach zstr, text.text, mem.bytes and mem.heap through
# `#pragma unisa use`, which host cc ignores: compile those beside them,
# declared by fuzz/host_l0.h.
L0INC="-include $R/apps/csih/fuzz/host_l0.h"
# the fuzz drivers carry no #include (tests/cxinclude): their host headers come from here
DRVINC="-include stdio.h -include stdlib.h -include string.h"
L0OBJ="$TMP/zstr.o $TMP/text.o $TMP/bytes.o $TMP/heap.o $TMP/fmt.o $TMP/num.o"
if cp zstr.cx "$TMP/zstr.c" && cp text/text.cx "$TMP/text.c" && cp mem/bytes.cx "$TMP/bytes.c" \
   && cp mem/heap.cx "$TMP/heap.c" && cp text/fmt.cx "$TMP/fmt.c" && cp text/num.cx "$TMP/num.c" \
   && cc $CFLAGS $CXINC -c -o "$TMP/fmt.o" "$TMP/fmt.c" \
   && cc $CFLAGS $CXINC -c -o "$TMP/num.o" "$TMP/num.c" \
   && cc $CFLAGS $CXINC -c -o "$TMP/text.o" "$TMP/text.c" \
   && cc $CFLAGS $CXINC -c -o "$TMP/bytes.o" "$TMP/bytes.c" \
   && cc $CFLAGS $CXINC -c -o "$TMP/heap.o" "$TMP/heap.c" \
   && cc $CFLAGS $CXINC $L0INC -c -o "$TMP/zstr.o" "$TMP/zstr.c"; then
    :
else
    echo "FAIL 3: host build of the L0 helpers"; rc=1
fi
if cc $CFLAGS $CXINC $L0INC -c -o "$TMP/json.o" "$TMP/json.c" \
   && cc $CFLAGS $DRVINC -include json.h -c -o "$TMP/fuzz_json.o" "$TMP/fuzz_json.c" \
   && cc $CFLAGS -o "$TMP/fuzz_json" "$TMP/fuzz_json.o" "$TMP/json.o" $L0OBJ \
   && "$TMP/fuzz_json" "$ITERS" > "$TMP/fuzz_json.out" 2>&1; then
    echo "ok   3a: json fuzz $(cat "$TMP/fuzz_json.out")"
else
    echo "FAIL 3a: json fuzz"; cat "$TMP/fuzz_json.out" 2>/dev/null; rc=1
fi
if cc $CFLAGS $CXINC $L0INC -include json.h -c -o "$TMP/csih_message.o" "$TMP/csih_message.c" \
   && cc $CFLAGS $DRVINC -include json.h -include csih_message.h -c -o "$TMP/fuzz_msg.o" "$TMP/fuzz_msg.c" \
   && cc $CFLAGS -o "$TMP/fuzz_msg" "$TMP/fuzz_msg.o" "$TMP/csih_message.o" "$TMP/json.o" $L0OBJ \
   && "$TMP/fuzz_msg" "$ITERS" > "$TMP/fuzz_msg.out" 2>&1 \
   && grep -q 'decoded_ok=[1-9]' "$TMP/fuzz_msg.out"; then
    echo "ok   3b: msg fuzz $(cat "$TMP/fuzz_msg.out")"
else
    echo "FAIL 3b: msg fuzz (build/run failed, or decode success path never hit)"; cat "$TMP/fuzz_msg.out" 2>/dev/null; rc=1
fi

# 4. context-index golden (unchanged contract; drives csih.sh CX entry)
if mkdir -p "$TMP/ci" && sh "$UNISACC" fixtures/context_index_fixture.cx -- "$TMP/ci/owned" > /dev/null \
   && ./csih.sh context-index "$TMP/ci/owned" csih2 "$(printf 'a%.0s' $(seq 64))" > "$TMP/ci/raw.json" 2> "$TMP/ci/err.log" \
   && sed -e 's|"captured_ms":[0-9]*|"captured_ms":<MS>|' -e "s|$TMP/ci/owned|<DIR>|g" \
          -e "s|\"turn_start_cwd\":\"$ROOT\"|\"turn_start_cwd\":\"<CWD>\"|" "$TMP/ci/raw.json" > "$TMP/ci/norm.json" \
   && cmp -s "$TMP/ci/norm.json" fixtures/context_index.golden.json; then
    echo "ok   4: context-index packet matches golden"
else
    echo "FAIL 4: context-index golden mismatch (see $TMP/ci/norm.json vs fixtures/context_index.golden.json)"; head -c 400 "$TMP/ci/err.log" 2>/dev/null; rc=1
fi

# 5. hard-coded lengths match their string literals (a 9 for an 11-byte escape once ate a char)
if sh "$UNISACC" "$ROOT/tools/litlen.cx" -- "$ROOT"/*.cx "$ROOT"/*.h "$ROOT"/tools/*.cx > "$TMP/litlen.out" 2>&1; then
    echo "ok   5: literal lengths match their counts"
else
    echo "FAIL 5: literal length mismatch"; cat "$TMP/litlen.out"; rc=1
fi

[ $rc -eq 0 ] && echo "check ok" || echo "check FAILED"
exit $rc
