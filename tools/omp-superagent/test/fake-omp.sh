#!/bin/sh
# 真实 RPC 形状:先接 prompt，发送事件，等待 get_last_assistant_text。
if [ "$FAKE_MODE" = nohandshake ]; then
  IFS= read -r prompt || exit 0
  touch received-prompt
  exit 0
fi
printf '{"type":"extension_ui_request","method":"notify","message":"sa-worker-ready:%s"}\n' "$SA_WORKER_NONCE"
IFS= read -r prompt || exit 2
case "$FAKE_MODE" in
  hang) sleep 30; exit 0 ;;
  crash) exit 3 ;;
  badjson) printf 'not-json\n'; exit 0 ;;
esac
printf '%s\n' '{"type":"response","id":"prompt","success":true}'
printf '%s\n' '{"type":"tool_execution_start","toolName":"read","toolCallId":"1","args":{"path":"example.txt"}}'
case "$FAKE_MODE" in
  forged) printf '%s\n' '{"type":"message_end","message":{"role":"assistant","content":[{"type":"text","text":"VERIFIED-TOKEN"}]}}' ;;
  receipt) printf '%s\n' '{"type":"tool_execution_end","toolName":"read","toolCallId":"1","result":{"content":[{"type":"text","text":"\n[工具收据 ID: 1]"}]},"isError":false}' ;;
  *) printf '%s\n' '{"type":"tool_execution_end","toolName":"read","toolCallId":"1","result":{"content":[{"type":"text","text":"VERIFIED-TOKEN"}]},"isError":false}' ;;
esac
printf '%s\n' '{"type":"prompt_result","id":"prompt","status":"completed"}'
IFS= read -r last || exit 2
case "$last" in *get_last_assistant_text*) ;; *) exit 4 ;; esac
# 测试使用固定 ticket；mismatch 故意返回不同 ticket。
ticket=T1
[ "$FAKE_MODE" = mismatch ] && ticket=OTHER
state=done
blocked=''
[ "$FAKE_MODE" = blocked ] && state=blocked && blocked='需要凭证'
verified=VERIFIED-TOKEN
[ "$FAKE_MODE" = receipt ] && verified='[工具收据 ID: 1]'
[ "$FAKE_MODE" = unverified ] && verified=NEVER-IN-OUTPUT
files='[]'
[ "$FAKE_MODE" = files ] && files='[{"path":"src/x.ts","change":"改"}]'
report=$(printf '{"t":"report","ticket":"%s","state":"%s","summary":"ok","verified":[{"toolCallId":"1","kind":"output","value":"%s"}],"inferred":[],"files":%s,"blocked":"%s","next":[],"tokens":7,"ms":3}' "$ticket" "$state" "$verified" "$files" "$blocked")
[ "$FAKE_MODE" = noise ] && report="noise $report"
# report 为 JSON 字符串，shell 中转义反斜线和双引号。
escaped=$(printf '%s' "$report" | sed 's/\\/\\\\/g; s/"/\\"/g')
printf '{"type":"response","id":"last","success":true,"data":{"text":"%s"}}\n' "$escaped"
[ "$FAKE_MODE" = nonzero ] && exit 3
exit 0
