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

# 3. fuzz under ASan+UBSan (json.c and csih_message.c linked as separate units)
if cc $CFLAGS -include json.h -o "$TMP/fuzz_json" fuzz/fuzz_json.c json.c \
   && "$TMP/fuzz_json" "$ITERS" > "$TMP/fuzz_json.out" 2>&1; then
    echo "ok   3a: json fuzz $(cat "$TMP/fuzz_json.out")"
else
    echo "FAIL 3a: json fuzz"; cat "$TMP/fuzz_json.out" 2>/dev/null; rc=1
fi
if cc $CFLAGS -include json.h -include csih_message.h -o "$TMP/fuzz_msg" fuzz/fuzz_msg.c csih_message.c json.c \
   && "$TMP/fuzz_msg" "$ITERS" > "$TMP/fuzz_msg.out" 2>&1 \
   && grep -q 'decoded_ok=[1-9]' "$TMP/fuzz_msg.out"; then
    echo "ok   3b: msg fuzz $(cat "$TMP/fuzz_msg.out")"
else
    echo "FAIL 3b: msg fuzz (build/run failed, or decode success path never hit)"; cat "$TMP/fuzz_msg.out" 2>/dev/null; rc=1
fi

[ $rc -eq 0 ] && echo "check ok" || echo "check FAILED"
exit $rc
