# v0.1.6 tools-compile 层 1 有界并发池与 LPT 排程补丁草案说明

- 上游设计: `w-agyneon2/plans/v0.1.6-tools-compile-load-bound.md` (第 3 节层 1)
- 补丁草案: `/home/devali/repos/unisacc/.claude/worktrees/w-agyneon3/.work/tools-compile-bounded-pool.patch` (针对 `tests/tools-compile.cx`, 冻结期**未应用**)
- 状态声明: 仅静态设计与补丁草案, **未执行编译、冷测试、生成器或批次**, 运行态指标均为 **`UNMEASURED`**.

## (1) 补丁草案核心改动

1. **`TOOLS_COMPILE_JOBS` 严格解析 (`parse_compile_jobs`)**:
   - 未设或空串 -> 默认 `8` (保持单跑默认行为与所有既有断言兼容);
   - 合法十进制整数 -> 钳制至 `1..8` (`<1 -> 1`, `>8 -> 8`);
   - 含非数字字符 (如 `"abc"`, `"-1"`, `"2x"`) -> fail-closed `die("REFUSE invalid TOOLS_COMPILE_JOBS (expected integer 1..8)")`, 绝不静默降级.
2. **分片内稳定 LPT 降序排程 (`lpt_order`)**:
   - 按 `stat(full, &st).st_size` (跟随符号链接获取真实编译源体积, 符合 `symlinks=follow-each-alias`) 稳定降序排列 `order[0..total-1]`, 同体积保持原 Git 索引升序;
   - `stat` 失败时体积置 `0`, 不提前中断, 交由 `compile(i)` 按既有合同报告.
   - 对 shard 137: `tools/devsvc-core.cx` (301 KB) 必在 `t=0` 独占槽 0 启动, 槽 1 顺序编完其余 7 个小文件, 消除尾部队头拖尾.
3. **无队头阻塞的有界并发工作池 (`waitpid(-1, &st, 0)`)**:
   - 任意时刻在途子进程数严格满足 `0 <= active <= max_jobs`;
   - 每次任一子进程结束立即回收并回填下一个 `order[next]`;
   - `pid[j]`、`pending[j]`、`deadlines[j]` 均按原分片槽位 `j = i - first` 索引, `fork` 失败置 `fork_err=1` 但仍完整回收已启动子进程不遗留孤儿;
   - 延后 `debt_identity` 遍历 `0..total-1` (修正原 `0..n-1` 在非顺序启动下可能漏槽的问题), `results.tsv` 与 stdout 聚合仍严格按 `first..last-1` 输出, 100% 保持 `--default-regression` 字节等价.
4. **不放宽时限 + 摘要可审计**:
   - 单源 `np_run` 仍硬限 `40s`, 不设静默重试;
   - `TOOLS_COMPILE\tSUMMARY` 将硬编码 `jobs=8` 改为 `jobs=%d` (`max_jobs`), 默认未设时仍输出 `jobs=8`.

## (2) 解冻后落地清单与验收计划 (当前 `UNMEASURED`)

1. 在解冻后的 `v0.1.6` 分支应用 `.work/tools-compile-bounded-pool.patch`, 并同步在 `tests/env-semantics.tsv` 登记 `TOOLS_COMPILE_JOBS`:
   - 缺省/空: `8`; 合法整数: 钳制 `1..8`; 非法值: 具名 `REFUSE` (`rc=2`).
2. 验证用例 (解冻后执行):
   - 非法值拒收: `TOOLS_COMPILE_JOBS=abc` 与 `TOOLS_COMPILE_JOBS=-1` 均返回 `rc=2` 且含 `REFUSE invalid TOOLS_COMPILE_JOBS`;
   - 钳制与摘要: `TOOLS_COMPILE_JOBS=0` -> `jobs=1`, `TOOLS_COMPILE_JOBS=2` -> `jobs=2`, `TOOLS_COMPILE_JOBS=99` -> `jobs=8`, 未设 -> `jobs=8`;
   - 回归字节等价: `tests/tools-compile.cx --default-regression` 对 `TOOLS_COMPILE_JOBS=1`/`2`/`8` 与旧版逐字节等价 (`TOOLS_COMPILE_DEFAULT_REGRESSION\tPASS`);
   - shard 137 对照: `J=2 + LPT` vs `J=8` 墙钟与峰值并发数实测.
