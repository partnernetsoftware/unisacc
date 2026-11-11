# TAPE-END 复现与验收（0.0.47，不改产品）

目标：`tape.text` 末行固定 `.end`；elf 入口无该行则 `reject: tape truncated (no .end)`。

## 历史实测（ledger 2026-11-05 TRUNC-MEASURED）

stdio 程序 `h.c` 的 `run/O0` tape **10672 行**，行尾截断点 **10670** 个喂 `stream prune|lower|elf`：

- 10667：elf `reject: not covered: a branch to an undefined label`
- **3**：只留前 1–3 行，仍产出可执行，运行 **127 或 SIGSEGV（-11）**，无「跑成功但结果错」

本轮未重跑 10670（不抢 s8b）。本机 `-S` 最小 `main` 得 41 行 tape；把截片当 C 源 `-c` 会 `not covered: top-level construct`，**不是** 当时的 stream-elf 路径。

## 复现命令（产品改后才绿）

```
# 完整 tape：unisacc -S -b lnx/x86_64 FILE.c > tape.text
# 截到第 N 行：head -n N tape.text > cutN.text
# 喂 elf 入口（与当时相同的 stream 管道，不经 C 前端）
# 期望：reject: tape truncated (no .end) 且无 ELF
```

崩溃 3 处：**N=1,2,3**（只留 `_start:` 起前几行）。

## 验收（产品改动后）

- 任意行尾截断：elf rc≠0，stderr 含 `reject: tape truncated (no .end)`，无产物
- 正例带 `.end`：与改前逐字节等价（网重生另项）
- 不改本轮产品
