# 0.0.107 事件记账: W0201 产品红停与路径 C 重起链

记录窗口: `agy-h` (仅事件账本与时间重估, 不动车树、旧链与旧预跑目录)
决定时点: `2026-11-25 04:21 +0800` (`cc` 依据 `cdx` 私有直证裁定走路径 C)

---

## 1. W0201 产品红停事件与决策依据

1. 停机实测原行 (`~/.cache/unisacc/prerun/v0.0.107-28b6a518fe/`):
   - 停机时刻: `2026-11-25 04:16:43 +0800` (`epoch=1795551403`, 距首轮起链 `02:49:02` 经过 `87.7` 分钟, 距首轮预跑起跑 `03:21:56` 经过 `54.8` 分钟)。
   - `prerun-watch v2` 状态: `PRERUN_WATCH state=STOPPED done=202/247 rate_inc=4.47 rate_cum=3.69 eta_min=10.1 rc124=W0000,W0080,W0088,W0139,W0140,W0178 lanes=2 last=W0201 phase=long` (`rc=1`; 同波 `W0199 fwd-cap` 已 `PASS` 且 `assertions=28`)。
   - `supervise.log` 停机行: `PRERUN_PRODUCT_RED_STOP batch=c99-array-param-typedef domain=product-qualification status=FAIL rc=1 retries=2 window=W0201 triage=inspect-strict-W0201.tsv` 与 `PRERUN_STOP batch=c99-array-param-typedef retries=2 window=W0201 ... ms=3610` (未生成 `done.W0201`)。
   - `strict-W0201.tsv` 原行: `RAW BATCH_RUN id=c99-array-param-typedef status=FAIL rc=1 transport=0 marker=ARRPARAM_SUMMARY candidate=9a1e2630100deace2b6a8d82a1b6c47fdf57b872e82f3cd4e1ee55756c2be544 headers=product scope=actual-one-batch-no-cached-result assertions=0`。
2. 坐实与路径 C 决策依据 (`cc` 04:21 裁定 + `cdx` 私有复现直证):
   - 107 首轮候选 APE (`9a1e263010...`, 来自 `28b6a518fe`) 对用例 `N2` (`typedef int row[3]; void f(row r[2);`) 发出诊断 `error: expected ']'` 却仍返回 `rc=0` 并写出 `N2.o` (`2144B`), 而宿主 `cc` 对同一非法语法返回 `1` 并拒绝产出对象。
   - 依项目铁律(严禁改检查迁就、严禁复用旧候选或旧严格窗口证据), 裁定走路径 C: 修产品诊断退出码缺陷 -> 产品输入变更后重新起链生成新候选 APE -> 新候选全量重新跑 2-lane 严格预跑(不可复用 `28b6a518fe` 既完 `202` 窗)。
   - 证据保留: 首轮链目录 `~/.cache/unisacc/chains/bg-b1/28b6a518fe9fac6b117f31adeb25e1c8ab51701f` 与首轮停机预跑目录 `~/.cache/unisacc/prerun/v0.0.107-28b6a518fe` 保持原样封存作历史证据, 禁止清理或覆写。

---

## 2. 首轮耗时复盘与路径 C 重起链时间重估

| 阶段 / 事件 | 耗时口径 | 依据与起止区间 (`+0800`) | 口径类型 |
|---|---|---|---|
| 首轮 `28b6a518fe` 起链 -> 红停分诊 | `92.9m` | 链 `02:49:02..03:15:16` (`26.2m`) + 转段 (`6.7m`) + 预跑 `03:21:56..04:16:43` (`54.8m`, `202/247`) + 分诊坐实 `04:21:54` (`5.2m`) | 实测 (作废留证) |
| 路径 C1: 修产品 + 复核摸底 + 重备门 | `~30m` | `ompgpt` 修复 `expected ']'` 后置非零返回与 `.o` 拦截 + `cdx` 变异与 `W0198`/`W0200` 等相邻批单跑摸底 + `agyneon`/`cc` 门 5/22/19 重备 (`~04:22..04:52`) | 估算 (`cc` 排程) |
| 路径 C2: 107 新候选重起整链 | `~27m` | 对应 `28b6a518fe` 首轮实测 `26.2m` (`~04:52..05:19`) | 基准外推 |
| 路径 C3: 新候选全量 2-lane 重预跑 | `~70m` | `~247` 窗按首轮累计速率 `3.69` 窗/分计 `~67m` (不含红停重来, `~05:22..06:32`) | 实测外推 (`cc` 估) |
| 路径 C4: 补跑/打包/CI/公开收尾 | `~60m` | `merge`/`pack`/`dry`/`Step 1 X` + `Step 3 CI` (`106` 基准 `15.7m`) + `Y1/publish/readback/Y2/release-post` (`~06:32..07:32`) | 估算 (`cc` 排程) |
| 路径 C 重起链总估 -> `107` 新公开区间 | `~187m` | 自 `04:22` 起预计 `07:10..07:45` 完成公开与解冻 | 保守重估 |

对 `0.1.0` 封板车与 `24` 小时总目标影响核对:
- 准入第 `6` 条 (`T1->T4 <= 120m` 与 `T2->X <= 45m`): 首轮及路径 C 均已明确不满足, 按 `R37` 与 `decision-24h-fable.md` 在发布说明与 [chain-to-public.tsv](chain-to-public.tsv) 如实记未满足。
- `0.1.0` 起链线核对: 若 `0.0.107` 于 `07:45` 前公开解冻, `0.1.0` 零改动封版车按 `110-145m` 估算约 `09:45..10:20` 公开; 距 `fable` 规定的 `0.1.0` 最迟起链线 `13:40` 仍有约 `5 小时 45 分钟`余量, 距 `2026-11-25 17:50` 总截止线余 `7 小时 30 分钟`以上。
