# CX 双后端统一与提速计划

## 1. 目标

对外只有一个 CX 机制，两个执行后端：

- **unisacc 后端**：`unisacc.com` 自己解析模块图、生成 CXIF、执行默认运行。
- **宿主后端**：`gcc_cx.com` / `clang_cx.com` 用同一模块语义展开成普通 C，再交给系统编译器。

验收不是“两边都能跑”，而是同一输入得到同一语义和可接受的诊断；日常运行速度有明确预算。

## 2. 当前事实（2026-11-01 更新）

| 事实 | 证据 |
|---|---|
| P0–P2 已落地 | `tests/cx-host-run.cx` 对拍与 realpath 夹具；`cx0-extract --check` 为 `EXACT` |
| 三源已合一 | realpath、symbol-aware 去重、distinct-use、multi-rewrite 进入 `cx/cx0/plan.cx`；`src/cxmod.c` 由它投影；`cx/compiler/cxmod.cx` 同步；MAXUSE=48 |
| realpath 身份 | 叶模块/依赖一致的双入口合并；依赖不同的双入口具名拒绝 |
| 默认运行速度 | unisacc 唯一源约 450ms 冷；同路径缓存约 11ms 热；`-c` 约 403ms；`-cx-plan` 约 6ms |
| 宿主速度 | gcc_cx 约 46ms 热 / 73ms 冷；clang_cx 约 85/128 |
| 当前候选 | `out/p5-rebuild/ape/unisacc-next.com` sha `552c5f96…`，release smoke 5/5、check-install-fixes 0、候选 smoke 16/0；未安装 |
| 已装产品 | `unisacc.com` 仍为 sha `21782dd5…`；不能用候选结果冒充已装身份 |
| 现有回归 | `cx-host-run` PASS；host suite 11×2 fails=0；`cx0-unit 28/0`；`cx-funit-host 10/0`；`cx-layers` PASS；ratchets 7/0 |

## 3. 原则

1. `cx/spec.md` 是唯一对外语义；`cx/cx0/plan.cx` 是解析权威，`src/cxmod.c` 只能是可再生成的兼容投影，`cx/compiler/cxmod.cx` 必须与权威同步。
2. 不允许再新增第二套层规则或第二套模块解析器。宿主复用同一 plan 语义；`cx-layers` 仍是 unisacc 全仓审计，宿主只接只读结果。
3. 速度优化不得用缓存结果冒充 PASS；只允许缓存可测的构建产物，并继续校验输入身份。
4. 未实测、未重建产品的格子记 UNKNOWN，不升格。

## 4. 切片与顺序

### P0：基线与对拍语料（先做）

- 固定语料：无 use、单 use、跨层 use、多符号、整模块+符号混序、alias、注释/续行/条件 pragma、cxpack、同 realpath 双入口。
- 新增或扩展 `tests/cx-host-run.cx`：保存两份 `-cx-plan` 输出，做归一化比较；运行退出码比较。
- 记录 unisacc 当前差异（预期先失败），不把失败藏成 PASS。

验收：语料在两边都实际执行；差异有逐项收据；不新增 Python。

### P1：三源合一

- 把 `src/cxmod.c` 中已验证的 `CXM_HOST_REALPATH`、symbol-aware 去重、distinct-use 计数迁入 `cx/cx0/plan.cx` 的 `CX0_COMPAT_BODY`，或通过生成器条件保留。
- 修 `tools/cx0-extract.cx`，使 `--check` 通过；重新生成 `src/cxmod.c`，禁止手改生成物。
- 同步 `cx/compiler/cxmod.cx` 的同一语义；MAXUSE 数值先统一为同一常量，不靠两份手工数。
- 用 `cx0-unit`、`cx-funit-host`、`cx-host-run`、host suite 验证。

验收：`cx0-extract --check` rc0；`cx0-unit` PASS；同一对拍语料两边 plan 同构；三处源码无未解释差异。

### P2：realpath 身份与歧义

- 统一 `cxm_samefile` 语义；同一物理源的两个词法入口，依赖一致时合并，依赖不一致时具名拒绝，不再按首入路径静默选择。
- 增加正反例：双软链一致、同名模块不同内容、缺失入口。

验收：拒绝行含两个入口与冲突模块；一致用例两边运行结果相同；`cx-host-run` 覆盖。

### P3：速度预算（A/B 分开做）

**A：冻结基线（已完成，作为 fallback）**

- 固定语料：tiny、模块链、多符号、cxpack、realpath；同一机器记录冷/热各 10 次中位数。
- 当前基线：unisacc 唯一源冷 10 次共 4347ms（约 435ms/次），同路径热 10 次共 116ms（约 11.6ms/次）；`-c` 约 403ms；`-cx-plan` 约 6ms；gcc_cx 46/73；clang_cx 85/128。
- 一次 `UNISACC_RR_TIME=1 -run` 的构造阶段：construction 74.5ms；model inflate e2/e1/e3/prune/lower/elf 合计约 115ms；inference 合计约 204ms；output 0.06ms。冷路径主要不是缓存失效。
- A 只记录，不改产品；不把 11.6ms 缓存命中写成冷路径达标。B 的首个候选方向是减少每进程重复的 model inflate/构造，而不是再改 run-cache 命中逻辑。

**B：仓外性能实验（未开始）**

- 在 `~/repos/unisacc-p3-b` 用同一 rev 的 `git archive` 独立克隆做实验，不碰共享 main、不开 worktree。
- 先拆构造/编译阶段；只改已测瓶颈；每次保留 A/B 同机同源冷热对比。
- 目标：热默认运行 ≤90ms、冷 ≤150ms；宿主不退化。
- B 只在 Pareto 改善且 P0–P2/P4 全绿时按路径搬回 main；否则丢弃，A 保持 fallback。

验收：A/B 同语料 10 次冷热中位数、内存/启动数字；B 不得用缓存命中替代冷测；未达目标记 UNKNOWN，不记 PASS。

### P4：测试与门禁

- `tests/run.cx -- --smoke`、`cx0-unit`、`cx-funit-host`、`cx-host-run`、host suite、`cx-layers` 全量跑通。
- 新增对拍批次登记到 `tests/batches.tsv`，标记真实预算；不伪造 PASS。
- `tools/ratchets.cx` 七项通过；文档写清未覆盖平台和编译期所有权边界。

### P5：产品候选（候选已完成，安装未做）

- 已按 `tools/release-build.cx` 生成 P3 包、五片并 `tools/release-ape.cx pack`：
  `out/p5-rebuild/ape/unisacc-next.com` sha `552c5f96…`。
- 已过 `release-ape smoke` 5/5、`check-install-fixes` failed=0、候选自身 smoke 16/0。
- **未替换 `./unisacc.com`**；安装仍需两代五片逐字节相同及安装授权。
- 安装后重复 P0/P2/P3 的关键用例；跨平台按独立证据，不外推。

## 5. 依赖与风险

- P0–P2 已完成；P4 门禁已跑过同一候选前源码。P3-B 必须用独立克隆，避免污染 P0–P2 的共享 main。
- 生成投影只经 `tools/cx0-extract.cx`；禁止手改 `src/cxmod.c`。`--check` 现在为 `EXACT`。
- MAXUSE 已统一为 48；realpath 依赖一致才合并，冲突具名拒绝。
- P5 候选已存在但未安装；安装前必须完成两代五片比较，未安装不得声称产品身份。
- 编译期所有权、包管理/workspace 集成不在本计划；运行期句柄检查不得称为所有权。
