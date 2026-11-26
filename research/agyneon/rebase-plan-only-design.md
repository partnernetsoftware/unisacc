# `v0.1.4` `prerun-drive --rebase-rev --plan` 只读计划设计与最小实现方案

- 维护者: `agyneon` | 范围: 第一步仅做只读 `--plan` (不写任何文件); 事务化执行 (`frozen-root` 原子替换 + `done.W*` `sy_unlink` + `frozen-source.rev` 更新) 留作第二步。

## 1. 输入与准入硬门

- **输入**:
  - 旧预跑目录 `<out_dir>`: 读取 `frozen-source.rev` (`old_rev`)、枚举 `done.W*` (检查 `sy_dir_next < 0` 即报错中止) 与对应 `strict-W*.tsv`。
  - 目标版本 `new_rev`: 入口经 `git rev-parse --verify <new_rev>^{commit}` 规范化为 40 位 SHA (`new_sha`), 拒绝 `"HEAD"` 等符号名直接进入后续流程。
- **准入硬门**:
  1. `git merge-base --is-ancestor <old_sha> <new_sha>` 必须返回 `0`, 否则 `REFUSE non-ancestor`。
  2. `git diff --name-only <old_sha>..<new_sha>` 必须无截断 (`r.truncated == 0`), 截断即 `REFUSE diff-truncated`。

## 2. 真实产品 SID 实算 (`seed/ident.c`)

- 经 `UNISACC_IDENT_HELPER` (`argv` 直调, 禁用 `sh -c`) 实算 `old_sha` 与 `new_sha` 的完整 `SOURCES_SID`, 覆盖 `seed/ident.c` 全部产品与构建支持输入闭包 (不以四目录 Git 树哈希替代)。
- `SOURCES_SID(old_sha) != SOURCES_SID(new_sha)` 或任一变更路径触及产品/构建支持输入闭包时, 一律 `REFUSE sid-mismatch`。

## 3. 真实依赖闭包与逐窗判定

- 对每个 `done.W*` 窗口:
  1. 解析 `strict-W*.tsv` 的结构化批次 ID 与状态列: 任一批非 `PASS` (`FAIL`/`UNKNOWN`/`INVALID`) 或 `strict-W*.tsv` 缺失 -> `RERUN` (`reason=non-pass-or-missing`)。
  2. 基于 `tests/batches.tsv` (`SOURCE` 入口)、`tests/batch-product-inputs.tsv` (声明的产品/工具输入) 与 `tools/affected.cx` 反查窗口内全部批次的真实依赖闭包, 与变更文件集合 `D` 求交集; 交集非空 -> `RERUN` (`reason=dep-hit`, `hit=<file>`)。**禁用**对报告文本做 `strstr` 子串猜测。
  3. 其余无交集且全 `PASS` 窗口中, 按确定性规则强制抽取 `10%` (至少 1 窗当保留窗 >= 5, 或至少 5 窗当 >= 50) 标记 `RERUN` (`reason=sample-10pct`) 作兜底。

## 4. 结构化只读输出 (零写入)

- 逐窗输出:
  `REBASE_PLAN_WIN\twindow=Wxxxx\taction=RETAIN|RERUN\treason=<...>\thit=<file|->`
- 汇总输出:
  `REBASE_PLAN_SUMMARY\told=<40hex>\tnew=<40hex>\tsid=<sid>\tretained=<N>\trerun=<N>\tsampled=<N>\tchanged=<N>`
- 严格只读: 不创建、修改或删除 `<out_dir>` 内任何文件。

## 5. 最小实现计划与夹具用例清单 (含必红变异)

- **最小实现**: 在 `tools/prerun-drive.cx` 新增 `rebase-rev --plan` 只读入口, 复用 `tools/affected.cx` / `tests/batch-product-inputs.tsv` 闭包查询与 `UNISACC_IDENT_HELPER` `argv` 调用。
- **夹具 (`tools/check-prerun-rebase-plan.cx` + `tools/prerun-rebase-plan-cases.tsv`, 逐列解析比较)**:
  1. `sid_change_refused`: 产品/支持输入改动使 `SOURCES_SID` 变化 -> 必拒; 变异: 去掉 SID 比较必红。
  2. `non_ancestor_refused`: 非祖先 REV -> 必拒; 变异: 放宽祖先检查必红。
  3. `diff_truncated_refused`: `git diff` 输出超限截断 -> 必拒; 变异: 忽略 `r.truncated` 必红。
  4. `indirect_dep_hit_reruns`: 窗口内批次的间接依赖 (`batch-product-inputs.tsv` / `tools/cxlib/`) 被改 -> 必标 `RERUN`; 变异: 退回 `strict` 文本 `strstr` 猜测必红。
  5. `non_pass_window_reruns`: 窗口内含 `FAIL`/`UNKNOWN` 或缺 `strict-W*.tsv` -> 必标 `RERUN`; 变异: 忽略状态列必红。
  6. `replay_w0118_c7_to_c10_refused`: 真实快照 `W0118(C7)->C10` 触及 `cx/sys/posix.cx` -> 必拒; 变异: 放行产品路径必红。
