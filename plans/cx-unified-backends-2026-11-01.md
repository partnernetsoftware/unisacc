# CX 双后端统一与提速计划

## 1. 目标

对外只有一个 CX 机制，两个执行后端：

- **unisacc 后端**：`unisacc.com` 自己解析模块图、生成 CXIF、执行默认运行。
- **宿主后端**：`gcc_cx.com` / `clang_cx.com` 用同一模块语义展开成普通 C，再交给系统编译器。

验收不是“两边都能跑”，而是同一输入得到同一语义和可接受的诊断；日常运行速度有明确预算。

## 2. 当前事实

| 事实 | 证据 |
|---|---|
| 宿主多符号导入比未重建的 unisacc 新 | 同源 `use m::x; use m::y`：host `-cx-plan` 两个 CXIF 头，unisacc 一个 |
| 投影已漂移 | `sh ./unisacc.com tools/cx0-extract.cx -- --check` 当前 rc2：`expected-unique-compatibility-anchor` |
| 宿主补丁不在权威 body | `src/cxmod.c` 的 `CXM_HOST_REALPATH` 与 symbol-aware dedup 未出现在 `cx/cx0/plan.cx` 的 `CX0_COMPAT_BODY` |
| 三处 MAXUSE 不一致 | `src/cxmod.c`=32 且按 distinct target；`cx/cx0/plan.cx`=32 按原始 use；`cx/compiler/cxmod.cx`=48 |
| 默认运行速度 | unisacc 151ms/次热、约 460ms/次冷；gcc_cx 43/73；clang_cx 85/128 |
| 现有回归 | `cx-host-run` PASS；host suite 11 units×2 drivers `fails=0`；`cx-layers` PASS |

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

### P3：速度预算

- 先测 unisacc 默认运行的阶段（启动、解析、CXIF、执行），把 151ms 热路径拆开；未测相位记 UNMEASURED。
- 目标先定可测预算：热默认运行 ≤ 宿主 gcc_cx 的 2 倍（约 ≤90ms），冷运行 ≤150ms；达到后再收紧。
- 宿主保持当前预算：gcc 冷 ≤100ms、热 ≤50ms；clang 冷 ≤150ms、热 ≤90ms。
- 只优化已测瓶颈；每次改动保留冷/热两组数字，禁止用缓存命中替代冷测。

验收：同一语料 10 次冷/热中位数；优化前后同机同源对比；失败路径不进缓存。

### P4：测试与门禁

- `tests/run.cx -- --smoke`、`cx0-unit`、`cx-funit-host`、`cx-host-run`、host suite、`cx-layers` 全量跑通。
- 新增对拍批次登记到 `tests/batches.tsv`，标记真实预算；不伪造 PASS。
- `tools/ratchets.cx` 七项通过；文档写清未覆盖平台和编译期所有权边界。

### P5：产品候选

- 重建 `unisacc.com` 候选，核身份后再安装；未重建不得用源码结果推断产品。
- 安装后重复 P0/P2/P3 的关键用例；跨平台仍按独立平台证据，不外推。

## 5. 依赖与风险

- P0 必须先于 P1/P3；P1 完成前不改宿主新功能，否则继续扩大分叉。
- 最大风险是 `src/cxmod.c` 被生成器覆盖；P1 前禁止再手改投影。
- MAXUSE/realpath 的语义要在 spec 合同下落定，不能只求测试变绿。
- 编译期所有权、包管理/workspace 集成不在本计划；运行期句柄检查不得称为所有权。
