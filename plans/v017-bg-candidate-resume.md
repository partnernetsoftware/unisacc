# v0.1.7 `bg-candidate` 启动/续跑语义与内置 `resume` 设计

状态: `DESIGN_ONLY` (静态阅读 `tools/bg-candidate.cx`; 所有实现与运行验证均为 `UNMEASURED / UNCOVERED`)。

## (1) 实测语义与源码根因 (`tools/bg-candidate.cx`)

- **(a) 重启窗口 `status` 报旧 PID `FAILED`**:
  - `bg-candidate.cx:224-242` 读取 `owner.tsv` 最后一条 `OWNER` 与 `exit.tsv` 中匹配 `pid == c->pid` 的最后一条 `EXIT`。
  - 重启时新 guard 在 `:393` `fork()` 之后, 直到 `:408` 才写入新 `OWNER` 行; 在此窗口内轮询 `status`, 仍会读到旧 `OWNER` + 旧 `EXIT`, 从而输出旧 PID 的 `state=FAILED`。
- **(b) 同 `rev` 全新目录复用缓存却跳过证据**:
  - 移走旧 `BASE/<rev>` 后新建空目录运行 `start`, `chain-run` 虽可命中外部缓存, 但新目录缺少前序 `checkpoint.tsv` 与阶段收据 (`seal_leaf` `:96-97` / `out.argv.tsv`), 导致证据不全、`promote --check` (`:33-38`) 或下游失败。
- **(c) 从失败目录安全续跑 (`rename back`) 流程**:
  1. 确认无在途 `pid`/`pgid` (`bg-candidate status` 非 `RUNNING`, `pgrep` 无残留)。
  2. 校验待恢复目录内 `chain-run.identity`、`plan.tsv`、`checkpoint.tsv` 完整且绑定目标 `HEX40`。
  3. 若旧目录曾改名移出, 原子 `rename` 回 `BASE/<rev>` (确保 `BASE/<rev>` 不存在)。
  4. 直接在原目录执行 `bg-candidate start --rev HEX40` (或新 `resume`), 由 `chain-run` 沿已有 `checkpoint.tsv` 与收据续跑。

## (2) v0.1.7 设计提案: 内置 `resume` 子命令

- 用法: `cx tools/bg-candidate.cx -- resume --rev HEX40 [--from-dir ABS] [--deadline-s N] [--segment-s N] [COMMON]`
- 合同:
  1. 拒绝 `RUNNING` 或已 `DONE` 的候选。
  2. 若指定 `--from-dir`, 先校验其 `chain-run.identity` 属于 `HEX40` 且 `BASE/<rev>` 不存在, 再原子 `rename` 回 `BASE/<rev>`。
  3. 校验 `plan.tsv` 与当前 `ROOT/build/chain-phases.tsv` 语义哈希一致, 拒绝空目录冒充续跑。
  4. 在 `fork()` 前先写入新一代次 `gen=<N+1>` 的 `STARTING` 占位行, guard 在 `:408` 写入同代次 `OWNER` 行。

## (3) v0.1.7 设计提案: `status` 区分陈旧记录字段

- `BG_CANDIDATE` (`:301-303`) 新增 `gen=<N>` 与 `record=current|starting|stale`:
  - `STARTING` 占位存在且同代 `OWNER` 尚未落盘 -> `state=STARTING`, `record=starting`, 不再回显旧 PID 的 `FAILED`。
  - `OWNER` 已落盘且进程存活 -> `state=RUNNING`, `record=current`。
  - `EXIT` 属于旧 `gen` 时标 `record=stale`, 不与当前代次混淆。
