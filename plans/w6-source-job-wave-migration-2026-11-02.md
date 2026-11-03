# source-job-wave 调用方迁移计划（W6 后续，冻结期只读）

日期：2026-11-02。状态：**只计划，不改任何工具**。冻结窗口内 `tools/ tests/ cx/ src/ include/ weights/ exec/` 不动。

## 结论

`tools/source-job-wave.cx` 不是孤立旧文件：它仍在 **omp-bdy W5 慢车道纪元**的正式构造链里（`../moltbaby/plans/omp-bdy-handoff-2026-11-02.md:60`、`../moltbaby/plans/omp-board.md`：107 job = 5 tool + 34 gen + 34 tbl + 34 net）。
因此 W6 现在**不能归档**；要先把 `check-source-job-count.cx` 对它的读取解耦，等 W5 结束、bdy 改走 `entry-build` 后，再清表+归档。

## 调用方清单（只读核对）

| 调用方 | 类型 | 现状 | 迁移动作 |
|---|---|---|---|
| `tools/check-source-job-count.cx` | 运行期读取 | `loadz("tools/source-job-wave.cx")`，取 `ids[`、`counts[1]!=`、`counts[0]!=`，断言 recipes==gen、ids==5+3*gen、tools==5 | 改为只读 `source-package.cx` 的 recipes，直接算 ids=5+3*recipes；删除 wave 文件读取 |
| `../moltbaby/plans/omp-bdy-handoff-2026-11-02.md` | 运行手册 | 明确 W5 继续跑 `source-job-wave.cx` 107 job | W5 完成后改为 `entry-build`，或保留历史注记 |
| `../moltbaby/plans/omp-board.md` | 状态板 | omp-bdy 当前任务=source-job-wave(107 job) | W5 完成后更新任务行 |
| `AGENTS.md` / `CLAUDE.md` | 规则 | 把 `source-job-wave` 列为正规构造链 60s 例外 | 归档后从例外列表移除，换成 `entry-build`（或标注历史） |
| `tests/cxraw.tsv` / `cxlibc.tsv` / `cxplatform.tsv` | 计数表 | 行：cxraw 36、cxlibc 54、cxplatform 1 | 归档同批删行；表行减少即合法下调 |
| `tools/source-package.cx` | 配方源 | 提供 `SpModel recipes[]`，是 gen/tbl/net 数量的源头 | 保留；成为新的唯一事实源 |
| `tools/entry-build.cx` | 替代入口 | 后续构造入口 | W5 完成后作为新链路入口 |

当前 `tests/batches.tsv` 无 `source-job-count` 批次；`check-source-job-count.cx` 也未进批次。迁移时补一个独立 batch，避免检查只靠手工命令。

## 迁移顺序（W5 结束后执行）

1. **解耦检查**（只改 `tools/check-source-job-count.cx`）：
   - 保留 `count_recipes("tools/source-package.cx")`；
   - 删除 `loadz("tools/source-job-wave.cx")` 及 `ids/counts` 提取；
   - 断言改为：`recipes > 0`、`tools == 5`、`ids == 5 + 3*recipes`（ids 由 recipes 计算，不读 wave）；
   - 输出保持 `SOURCE_JOB_COUNT recipes=... tools=5 gen=tbl=net=... jobs=...`，便于旧日志对照。
2. **补夹具（先红后绿）**：
   - 临时把 recipes 数量改 1 个（或提供一个 `--manifest` 注入入口），确认旧 `ids`/`gen` 断言会红；
   - 恢复 recipes，确认新检查绿；
   - 夹具不得改 `source-package.cx` 的正式语义，测试结束还原。
3. **登记批次**：
   - `tools/register.cx -- cx-source-job-count tools/check-source-job-count.cx SOURCE_JOB_COUNT "source-package job counts" "tools/source-package.cx,tools/check-source-job-count.cx" interfaces`
   - 批次实跑 PASS 后才算迁移完成。
4. **等待 W5 收尾**：
   - 核对 bdy 不再调用 `source-job-wave.cx`（运行手册/状态板已改 `entry-build`）；
   - 全仓复扫：`grep -RIn source-job-wave .git/hooks .github release systemd cron tools tests exec plans`，只允许历史文档命中。
5. **归档**：
   - `git mv tools/source-job-wave.cx archive/tools-w6/`；
   - 同批删 `tests/cxraw.tsv`、`cxlibc.tsv`、`cxplatform.tsv` 三行（数值 36/54/1），并记录四表行数与棘轮基线变化；
   - `tools/ratchets.cx` 9/9；`tests/toolsloc.cx -- --write` 下调基线；
   - 更新 `AGENTS.md`/`CLAUDE.md`/`plans/omp-*` 的构造链入口；归档 README 记 SHA256。

## 验收

- `check-source-job-count.cx` 在**没有** `source-job-wave.cx` 的临时树中 PASS（证明解耦）。
- 新批次 `cx-source-job-count` PASS；旧 107-job 口径仍与 `source-package.cx` recipes 一致。
- `source-job-wave` 归档后 `ratchets` 9/9，`toolsloc` 只降不升，四棘轮表行减少已记录。
- 不删除 `exec/parse2` 等 `.tsv` 生成物；生成器迁移另案（本次仅 source-job-wave）。

## 阻塞与风险

- **阻塞**：W5 纪元正在使用 `source-job-wave.cx`；冻结期内改/移它会打断 bdy 构造。
- 风险：`check-source-job-count.cx` 当前还检查 wave 文件自身一致性；解耦后这只检查会消失，因此必须用 recipes→ids 的公式和独立批次补回“新增 recipe 不会静默漂移”的保护。
- 风险：`source-job-wave` 的 `ids[131]` 还出现在旧日志/手册；归档后保留 archive 路径，不删除历史证据。
- 下一动作：**仅等 cc 宣布 W5 结束**；冻结期内不启动本计划的任何步骤。
