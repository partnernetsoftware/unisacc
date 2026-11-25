# 0.0.107 第二轮 (`F2`) 预跑采样口径页

编写窗口: `agy-h` (仅定义第二轮采样口径与命令, 具体 `F2` / `T_prerun2` / `PID2` 待第二轮预跑起跑后按实填报)
适用前提: 路径 C 第二轮全新 `F2` 候选 (`--window-lanes 2`, 严禁复用第一轮 `28b6a518fe` 任何 `strict` 行)

---

## 1. 第二轮预跑输入锚点与账本隔离

| 字段 | 第二轮占位 / 取值规范 | 口径说明与隔离规则 |
|---|---|---|
| 候选提交 `F2` 与 `APE_SHA_2` | `<F2_10>` / `<APE_SHA_2>` (待新链 `exit.tsv` 实测填入) | 与首轮 `28b6a518fe` / `9a1e263010...` 完全隔离 |
| 第二轮起跑锚点 `T_prerun2` | `<YYYY-MM-DD HH:MM:SS +0800>` (`epoch=<T_PRERUN2_EP>`) | 取第二轮 `prerun-drive run` 实起时刻 (不沿用首轮 `03:21:56` / `1795548116`) |
| 守护进程句柄 `PID2` / `PGID2` | `<PID2>` / `<PGID2>` (由 `agy-flow` 起跑回报并用 `ps` 实核) | 用于监控外层 `5400s` 守护存活与退出态 |
| 第二轮窗口表与日志路径 | `PR2="$HOME/.cache/unisacc/prerun/v0.0.107-<F2_10>"` | 窗口表 `$PR2/windows.tsv` (预估 `~247` 窗), 日志 `$PR2/supervise.log`; 首轮 `v0.0.107-28b6a518fe` 原样封存不动 |
| 第二轮独立速率账本 | `research/agy-h/prerun-rate-107-r2.tsv` (镜像 `~/.local/share/unisacc/research/agy-h/`) | 新建 15 列 TSV 专记第二轮 `F2` 采样行; 不覆写或混入首轮 `prerun-rate-107.tsv` |

---

## 2. `priority` 置顶机制与预期前 20 窗位置

- 置顶规则来源 (`tools/prerun-inputs.cx:27-29`, `tools/prerun-drive.cx:499,726-757`): `pd_load_priority_file` 读取 `priority.tsv`, 凡非 `PASS` 条目赋 `priority = 2 (run first)`, 在 `plan` 编排阶段于 `W0000` (`warmup`) 之后优先落入最前窗口(`heavy`/`long` 按优先级先出队), 原先未入上轮 `106` 表而落到首轮后段 `W0198..W0205` 的 `107` 新增与修复批由此提前。
- 预期前 `20` 窗位置 (`W0001..W0019`):
  - `W0000`: 固定 `warmup` (`cxmod-tri-exports`)。
  - `W0001..W0019` (具体窗号以第二轮 `prerun-drive plan` 实产 `$PR2/windows.tsv` 为准): `c99-array-param-typedef` (`W0201` 修复对象)、`fwd-cap` (首轮 `W0199`)、`c99-global-redecl-conflict` (首轮 `W0198`)、`c99-tentative-array-incomplete` (首轮 `W0200`)、`c99-extern-array-incomplete` 及首轮停机未跑的 `W0202..W0246` / `priority=2` 风险批均预期落入前 `20` 窗, 在第二轮开跑后约 `2..8` 分钟(第 1 次 `10m` 采样前)即完成首轮验证, 避免再跑 `55` 分钟才暴露尾部产品红。
- 起跑第 `0` 分钟校验动作: `$PR2/windows.tsv` 生成后立即以 `head -n 25 "$PR2/windows.tsv"` 核实置顶批落位与总数。

---

## 3. 外层守护 `timeout 5400s` 到时点口径

- 首轮教训: `28b6a518fe` 使用 `timeout -k 10s 3600s` (`60m`), 若无 `W0201` 中断也将贴近 `04:21:56` 触发截断并需二次 `--resume`。
- 第二轮配置: 外层改用 `timeout -k 10s 5400s` (`90` 分钟), 对应到时点 `T_timeout2 = T_prerun2 + 5400s` (`T_prerun2 + 90 分钟`)。
- 余量核对: 基准 `247` 窗按首轮实测 `~4 窗/分` 估算净跑约 `62` 分钟 (`247 / 4 = 61.75m`), 外层 `5400s` (`90m`) 较基准净跑多留 `~28` 分钟余量, 可一次性完整跑完无需因 `3600s` 硬截断再接 `--resume` (若发生红停等人工处置仍可用同 `F2`、`--resume` 续跑并沿用同一 `T_prerun2` 计算 `elapsed`)。

---

## 4. `10/20/30/40/50/60` 分钟采样节拍与转保守档判据

基准参照: 以第一轮 `0.0.107` 实测均值与近 4 窗回升速率取 `4 窗/分` 作第二轮重估基准 (`247` 窗基准约 `62` 分钟 跑完; 第一轮 `202/247` `rate_cum=3.69`、`rate_inc=4.47`)。

| 相对时点 | 节点时间 | 基准进度 (`4 窗/分` 重估) | 本轮重点核查与触发阈值 |
|---|---|---|---|
| `T_prerun2 + 0m` | `T_prerun2` | `0/247` (`lanes=2`) | 记 `T_prerun2`、`PID2/PGID2`、`head -n 25 "$PR2/windows.tsv"` 核置顶前 20 窗; 建 `prerun-rate-107-r2.tsv` 第 1 行 |
| `T_prerun2 + 10m` | `+10m` | 约 `40/247` (推断) | 核 `done >= 14` 与 `lanes=2`; 逐条核前 20 窗中 `c99-array-param-typedef`、`fwd-cap` 等置顶批是否全部 `rc=0 PASS` (`chain-day-roster.md:24`) |
| `T_prerun2 + 20m` | `+20m` | 约 `80/247` (推断) | 核 `heavy` 窗通过进度、`outer_overhead_ms` (`~2s` 正常, 防 `DVQUALIFY_NATIVE_RUN_BATCH` 漂移) 与有无 `rc=124` 停机 |
| `T_prerun2 + 30m` | `+30m` | 约 `120/247` (推断) | 核 `done < 110` 触发 `fable` 判据 (`decision-24h-fable.md:41`); 注: `107` 全车虽已于 `03:53` 执行保守档, 仍以 `done < 110` 或落后 `>20%` 判定是否需复查外耗/降速并重核 `13:40` 最迟线 |
| `T_prerun2 + 40m` | `+40m` | 约 `160/247` (推断) | 核 `long` 相连续推进速度、`rc124` 自愈率与 `KG` 新增数 |
| `T_prerun2 + 50m` | `+50m` | 约 `200/247` (推断) | 核 `normal` 相入场提速与 `eta_min`; 若有 `PRERUN_STOP` 按 S1 于 `2m` 内分诊 |
| `T_prerun2 + 60m` | `+60m` | 约 `240/247` -> `DONE` (推断) | 若未完工续每 `10m` 或短程跟踪至 `PRERUN_RUN status=DONE`; 完工后立即执行 `DONE` 冻结态终采 (`eta_min=0`) 并更新 `chain-to-public.tsv` 新 `F2` 行 |

---

## 5. `prerun-watch v2` 标准只读命令行

起跑后全程严禁加 `--append-rate` (`v2` 已移除并拒收), 全程仅用以下三条命令每 10 分钟只读采数并向 `1:cc` 汇报:

```sh
PR2="$HOME/.cache/unisacc/prerun/v0.0.107-<F2_10>"
RATE_R2="research/agy-h/prerun-rate-107-r2.tsv"

# 1. prerun-watch v2 只读采样 (首采无 RATE_R2 时先省略 --rate-tsv, 建表后传入 --rate-tsv "$RATE_R2")
sh ./unisacc.com research/agy-h/prerun-watch.cx -- \
  --prerun-dir "$PR2" --rate-tsv "$RATE_R2" --prerun-start "$T_PRERUN2_EP"

# 2. 查看 supervise.log 末行与 outer_overhead_ms / PRERUN_STOP / PRERUN_RUN
tail -n 12 "$PR2/supervise.log"

# 3. 查看第二轮独立速率账本与置顶前 20 窗实际分布
tail -n 3 "$RATE_R2" && head -n 25 "$PR2/windows.tsv"
```
