# 超时清理缺口修补方案（run-batch / affected / bound）

日期：2026-11-02。状态：P1+P5 已落地（cc 批准，2026-11-02）：`tools/check-reaper.cx` 12 条断言全绿，批次 `cx-reaper` 已登记；P2/P3/P4 待 P5 绿后另批。审计证据见
`plans/cx-selfspawn-audit-2026-11-02.md` 第 2 节。

## 1. 现存缺口（行号已核对）

| 位置 | 现状 | 缺口 |
|---|---|---|
| `cx/sys/process.cx:96-99`（`np_run`，`tests/run-batch.cx:39` 的唯一超时路径） | 超时 `kill(-pid, SIGKILL); kill(pid, SIGKILL); waitpid(pid)` | 只按进程组杀；`setsid()` 后脱离组的孙进程存活；发信号前不冻结，`kill` 与 `fork` 竞态；不查 `/proc`、不核残留；有残留也照常返回超时，批次只记 FAIL 不记残留证据 |
| `tools/affected.cx:323-341`（`kill_tree`） | `pgrep -P` 宽度优先快照 → 逆序 `sy_killpg`+`sy_signal` → `sy_kill(pid)` | 快照之后新 fork 的进程漏杀；`setsid` 逃逸；依赖外部 `pgrep`；无冻结、无残留核验；`tools/affected.cx:388` 未走 `bound` |
| `tools/bound.c:284-319` | Linux：`PR_SET_CHILD_SUBREAPER`(:416)、子进程 `setsid`(:456)、`ps_snapshot`、`freeze_tree` 全停后 `kill(-root)` + 逐个 SIGKILL、`sweep_adopted` 扫 `/proc`、失败 exit 142 | 非 Linux `sweep_adopted` 空实现(:319)：无 subreaper 时双 fork 到 `init` 的进程既不在组里也不在 ppid 树里，属未核实逃逸（本机只有 Linux，未能实测） |

共同根因：**清理算法只杀“组”，不杀“树”，且发信号前没有把树冻住**。

## 2. 目标合同（改完必须成立）

1. 超时/取消后函数返回“残留进程数”，0 才算清理干净；调用方必须把残留 > 0 写进批次结论（非零退出 +
   `residue=N pids=…`），不得记 PASS。
2. 冻结顺序：先 `SIGSTOP` 根 → 反复扫 `/proc` 把新出现的后代/同组进程也 `SIGSTOP`，直到一轮无新增
   （带上限轮数与 2ms 间隔）；全部冻结后才 `SIGKILL`。被停住的进程不可能再 fork，竞态关闭。
3. 只按 pid 杀，不按 session；`killpg` 只作为“补漏”而非主手段；自身、自身祖先、自身登记的在跑子进程
   永不入列。
4. 单次清理有硬上限（默认 2000ms，与 60s 命令上限同量级），到点返回“未证实干净”，不无限扫。
5. 平台差异只放在 `cx/sys` 或 `bound.c` 的 `_os`/`#ifdef` 内，上层不写平台分支。

## 3. 方案

### P1 共享清理原语（`cx/sys/process.cx`，L0）— 已落地

- `int sy_child_subreaper(void)`：Linux `prctl(PR_SET_CHILD_SUBREAPER,1)`，其它平台 `return 0`
  （语义：不支持的平台不假装有一个 subreaper）。
- `int sy_kill_tree(int root, int deadline_ms, char *rep, int cap)`：
  1. `SIGSTOP root`；
  2. 循环（≤64 轮）：读 `/proc/*/stat`，把 `ppid` 为已冻结 pid、或 `pgrp==root` 的进程 `SIGSTOP`，
     有新增就再来一轮；2ms 间隔；
  3. 对被冻结 pid 逐个 `SIGKILL`，再 `kill(-root, SIGKILL)` 补漏；
  4. `waitpid(root, WNOHANG)` 轮询 ≤500ms；
  5. 残留扫（≤16 轮）：`ppid==getpid()` 且非本进程已登记子进程、且非僵尸 → `SIGKILL`；
  6. 返回残留数；`rep` 写 `pid(stime)` 列表，供日志与失败行使用。
- `/proc/PID/stat` 第 22 字段 starttime 作为身份，防 pid 复用误杀。

### P2 `np_run`（`tests/run-batch.cx` 入口）

- `main` 开始时调 `sy_child_subreaper()`（Linux 生效）。
- `cx/sys/process.cx:98` 的超时分支改为 `sy_kill_tree(pid, 1500, rep, …)`；残留 > 0 时把
  `why` 追加 ` residue=N`，`res->rc` 用 143 区分“超时且不干净”。
- run-batch 把残留写进 `BATCH_RUN` 行；`residue>0` 直接判 FAIL（现在是“超时=FAIL”，但残留不可见）。

### P3 `tools/affected.cx`

- 删掉 `pgrep -P` 快照（`kill_tree` 全函数），改调 `sy_kill_tree`；残留 > 0 时该 job 记
  `status=ERROR residue=N`，`AFFECTED_RUN` 的 `fail` 计数加一。

### P4 `tools/bound.c` 非 Linux

- 保留 Linux 现有实现（subreaper + 冻结 + 残留扫，已核）。
- 非 Linux：`freeze_tree` 的 `ps_snapshot` 打印同时取 `pgid`，补“同组也冻结”；冻结 → 杀后代 → 最后杀
  root（root 死后无 ppid 可查）；退出前用 spawn 时留的管道验证 EOF（全部后代已死才 EOF），EOF 未见且
  无法扫 `/proc` → 按现有合同 exit 142。
- 明确声明：macOS/Windows 的 setsid+双 fork 逃逸仍未实测，只在 Linux 上宣称闭合。

### P5 验证 — 已落地（Linux）

- 新 `tools/check-reaper.cx`（登记批次）：
  - 夹具 A：根 fork 子进程，子进程 `setsid` + 双 fork 后长睡；断言 `sy_kill_tree` 返回 0、逃逸 pid 消失、
    自身与祖先存活。
  - 夹具 B：普通三层树；断言全灭、`waitpid` 无僵尸。
  - 夹具 C：无关裸 `sleep`；断言不受影响。
- run-batch：加一个“超时且 setsid 逃逸”的临时批次输入，断言批次非零且 `residue=0`。
- affected：用同一夹具跑 `--files` 路径，断言无残留（`/proc` 扫描）。
- 回归：`tools/affected.cx --files cx/sys/process.cx tests/run-batch.cx tools/affected.cx --run`。

## 4. 风险

- `cx/sys/process.cx` 新增两个导出函数会动 `cxapis` 棘轮与 `check-cx-sys*` 批次：需要同批更新棘轮
  基线并重跑 `cx-l0-sys*`，否则棘轮红。
- `np_run` 是所有批次（含重测试）的公共路径，改动面大：先 P1+P5 落地并跑 `--all` 的快车道子集，
  再动 P2；P2 出问题时可单独回退到 `killpg+kill`。
- 冻结期间子进程不响应，`waitpid` 必须带超时，否则固定 60s 上限会被吃掉（方案里的 500ms 轮询即为此）。
- 非 Linux 逃逸无实测环境，只能按“未证实”记，不能写“已修复”。
