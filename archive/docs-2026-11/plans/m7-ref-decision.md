# M7 冻结参考 47 批 — 主人决策备忘（0.0.48 N1）

不重建 ⇒ 这 47 批只能披露 **KNOWN-GAP**，不得记 PASS。

## 1. `make ref`（12+10=22）— **KNOWN-GAP 结案 2026-11-08**

身份本应是 `e16028ffa1^` = `b902c8fbfb`。`git archive` 121M 含 `unisacc.c` 与 `src/internal_types.h`。

最小失败：父树 `tools/sourceflat.cx` 经当前 APE 跑出 `unisacc: model driver: cli`（rc=1）；活动树 `sourceflat.cx` 读的是活动根 `unisacc.c` 而非 REFSRC。30 分钟盒到时不再磨。

**22 批 KNOWN-GAP**，不得 PASS。
## 2. `streams` + `ref`（10）

批：`properties-cases-layout-*`、`properties-cases-lower`、`properties-cases-random`、`stream-e4-o2`。

- 同时要 streams 壳（可自造）和 **同一份 make ref**。
- 不重建 REF：10 批 KNOWN-GAP。

## 3. `stream-*-sha:sha256`（18）

| 钉 | n | 来源 |
|---|---|---|
| stream-lower-sha | 6 | 独立 lower 容器字节钉，来自已发布 P3/streampack 收据 |
| stream-e2-sha | 4 | 同上 e2 |
| stream-e1-sha | 4 | 同上 e1 |
| stream-e3/e4/elf/prune-sha | 4 | 各 1 |
| streams-o1+ref | 1 | O1 流容器 + REF |

- **来源**：c46-3/c47-1 候选 P3 成员 sha，不是 git archive 里的 shim `unisacc-streamN.com`（38B）。
- **不重建/不从旧候选拷钉**：18 批永远 KNOWN-GAP（伪 sha 禁止）。

## 4. 其它 sha 钉（5）

- `candidate-profile-sha`（2）：`candidate-identity-lnx-x86_64`、`hosts-same-candidate-lnx-x86_64` — 来自候选 profile 收据。
- `current-source-id:sha256` / `recorded-source-id`（2）：source-job 上下文。
- `adopted-package`+sha（1）。

不提供冻结收据 ⇒ KNOWN-GAP。

## 5. `src/tapeprune.c`（1）

`stream-prune-cases`：classic 参考源。git archive 可缺。不从冻结树取出 ⇒ KNOWN-GAP。

## 合计 47

披露口径：**KNOWN-GAP / NOT_CONSTRUCTIBLE**，不得用 shim 或本机 gcc 冒充 REF/sha。

## 结案记录(cc,2026-11-08 02:20)
`make ref`(optimization-* 12 批 + streams+ref 10 批 = 22 批)尝试三条路径均未成:① 活动树缺 `unisacc.c`(已归档);② 链接归档 `src/*.c` 仍缺符号与 `internal_types.h`;③ `git archive e16028ffa1^`(归档前完整活动树,121M)构造得 `unisacc: model driver: cli`——经典参考的构造本身依赖已归档的模型驱动。结论:22 批定为 KNOWN-GAP(经典参考不可重建),0.0.48 N1 的披露口径固定为「经典参考编译器因归档不可重建,optimization-* 无字节神谕」。不再投入。
