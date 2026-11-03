#!/bin/sh
# 假 omp:按 FAKE_MODE 输出预设 report 文本,用于测 runDispatch 的各终态。
case "$FAKE_MODE" in
  done)
    printf '%s\n' '{"t":"report","ticket":"T1","state":"done","summary":"ok","verified":["VERIFIED-TOKEN"],"inferred":[],"files":[],"blocked":"","next":[],"tokens":7,"ms":3}'
    ;;
  blocked)
    printf '%s\n' '{"t":"report","ticket":"T1","state":"blocked","summary":"卡住","verified":["VERIFIED-TOKEN"],"inferred":[],"files":[],"blocked":"需要凭证","next":[],"tokens":2,"ms":1}'
    ;;
  noise)
    printf '%s\n' '我先看一下。 {"t":"report","ticket":"T1","state":"done","summary":"ok","verified":["VERIFIED-TOKEN"],"inferred":[],"files":[],"blocked":"","next":[],"tokens":1,"ms":1}'
    ;;
  unverified)
    printf '%s\n' '{"t":"report","ticket":"T1","state":"done","summary":"ok","verified":["NEVER-IN-OUTPUT"],"inferred":[],"files":[],"blocked":"","next":[],"tokens":1,"ms":1}'
    ;;
  files)
    printf '%s\n' '{"t":"report","ticket":"T1","state":"done","summary":"ok","verified":["VERIFIED-TOKEN"],"inferred":[],"files":[{"path":"src/x.ts","change":"改"}],"blocked":"","next":[],"tokens":1,"ms":1}'
    ;;
  hang)
    sleep 30
    ;;
  *)
    printf '%s\n' 'garbage'
    exit 3
    ;;
esac
