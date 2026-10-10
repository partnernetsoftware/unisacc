#!/bin/sh
# csih 模块化 + 内存安全检查（仓库根执行：./check.sh）。任一项失败即 rc=1。
#  1. 模块公共头不得被源文件 #include（依赖经 -include 注入）
#  2. selftest 绿（rc=0 且输出 selftest ok）
#  3. json / csih_message 两个 fuzz 在 ASan+UBSan 下无报告，且解码成功路径被覆盖
ROOT=$(CDPATH= cd -- "$(dirname "$0")" && pwd) || exit 1
cd "$ROOT" || exit 1
TMP=$(mktemp -d) || exit 1
trap 'rm -rf "$TMP"' EXIT
rc=0
ITERS=${ITERS:-200000}
CFLAGS="-std=c99 -g -O1 -fsanitize=address,undefined -fno-sanitize-recover=undefined -w -I$ROOT"

# 1. no source #include of public module headers
if grep -rnE --include='*.c' --include='*.inc' '#[[:space:]]*include[[:space:]]+"(csih_cols|csih_home|json)\.h"' . ; then
    echo "FAIL 1: own-module public header #include in source (use csih.sh -include)"; rc=1
else
    echo "ok   1: no source #include of csih_cols.h / csih_home.h / json.h"
fi

# 2. selftest
if ./csih.sh selftest > "$TMP/selftest.log" 2>&1 && tail -1 "$TMP/selftest.log" | grep -q 'selftest ok'; then
    echo "ok   2: selftest ok"
else
    echo "FAIL 2: selftest (see below)"; tail -20 "$TMP/selftest.log"; rc=1
fi

# 3. fuzz under ASan+UBSan. cc only takes .c, so the .cx providers are copied to
#    $TMP as .c for the cc build; the repo keeps the .cx names.
cp json.cx "$TMP/json.c" && cp csih_message.c "$TMP/csih_message.c" && cp csih_message.h "$TMP/" || rc=1
if cc $CFLAGS -include json.h -I"$ROOT" -o "$TMP/fuzz_json" fuzz/fuzz_json.c "$TMP/json.c" \
   && "$TMP/fuzz_json" "$ITERS" > "$TMP/fuzz_json.out" 2>&1; then
    echo "ok   3a: json fuzz $(cat "$TMP/fuzz_json.out")"
else
    echo "FAIL 3a: json fuzz"; cat "$TMP/fuzz_json.out" 2>/dev/null; rc=1
fi
if cc $CFLAGS -include json.h -include csih_message.h -o "$TMP/fuzz_msg" fuzz/fuzz_msg.c "$TMP/csih_message.c" "$TMP/json.c" \
   && "$TMP/fuzz_msg" "$ITERS" > "$TMP/fuzz_msg.out" 2>&1 \
   && grep -q 'decoded_ok=[1-9]' "$TMP/fuzz_msg.out"; then
    echo "ok   3b: msg fuzz $(cat "$TMP/fuzz_msg.out")"
else
    echo "FAIL 3b: msg fuzz (build/run failed, or decode success path never hit)"; cat "$TMP/fuzz_msg.out" 2>/dev/null; rc=1
fi

# 4. cc C99 full build + selftest of the file table (cc/unisacc_ffi.h shim; .cx copied to temp .c).
#    Warnings are not gated here; the link and the cc-built selftest are.
for f in cols home json; do cp "$f.cx" "$TMP/$f.c" || rc=1; done
if cc -std=c99 -I cc -I . -include json.h -include csih_cols.h -include csih_home.h \
      tui.c render.c term.c chat.c clock.c tools.c "$TMP/cols.c" "$TMP/home.c" file.c shell.c edit.c gate.c "$TMP/json.c" \
      session.c agent.c plugin.c net.c \
      reload_state.c reload_session_decode.c reload_session_encode.c reload_io.c reload_load.c \
      reload_consume.c journal_checkpoint.c reload_owner.c csih_message.c csih_message_io.c context_index.c \
      -o "$TMP/csih_cc" -lcurl -lm -ldl > "$TMP/cc_build.log" 2>&1 \
   && "$TMP/csih_cc" selftest > "$TMP/cc_selftest.log" 2>&1 && tail -1 "$TMP/cc_selftest.log" | grep -q 'selftest ok'; then
    echo "ok   4: cc -std=c99 full build + selftest ok"
else
    echo "FAIL 4: cc build/selftest"; grep -m3 -E 'error|undefined' "$TMP/cc_build.log"; tail -3 "$TMP/cc_selftest.log" 2>/dev/null; rc=1
fi

# 5. context-index golden: owned fixture (two notices, fixed mtimes) through the real
#    tui_context_index path. Normalized: monotonic captured_ms, temp owned dir, runtime cwd.
if mkdir -p "$TMP/ci" && python3 fixtures/context_index_fixture.py "$TMP/ci/owned" \
   && ./csih.sh context-index "$TMP/ci/owned" csih2 "$(printf 'a%.0s' $(seq 64))" > "$TMP/ci/raw.json" 2> "$TMP/ci/err.log" \
   && sed -e 's|"captured_ms":[0-9]*|"captured_ms":<MS>|' -e "s|$TMP/ci/owned|<DIR>|g" \
          -e "s|\"turn_start_cwd\":\"$ROOT\"|\"turn_start_cwd\":\"<CWD>\"|" "$TMP/ci/raw.json" > "$TMP/ci/norm.json" \
   && cmp -s "$TMP/ci/norm.json" fixtures/context_index.golden.json; then
    echo "ok   5: context-index packet matches golden"
else
    echo "FAIL 5: context-index golden mismatch (see $TMP/ci/norm.json vs fixtures/context_index.golden.json)"; head -c 400 "$TMP/ci/err.log" 2>/dev/null; rc=1
fi

[ $rc -eq 0 ] && echo "check ok" || echo "check FAILED"
exit $rc
