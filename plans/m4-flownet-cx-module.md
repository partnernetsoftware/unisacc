# 需求：把 m4 的「流×网」范式做成可复用 `.cx` 模块

> 提出：2026-10-23（董事长要求）· 执行：cc（cc-unisacc）· 状态：W0 已裁定（三问皆是；W2 读外部 `.tsv`，生成 `.cx` 表为备选；API 只导出函数），待 W1
> 依据：`m4/README.md` 实测、`research/m4-flow-net-contract-draft.md`、`m4/flows.py` / `nets.py` 现状

## 0. 一句话

m4 已经证明「gold → IntNet → 四流 → 字节」这条链能逐字节复现；本需求要把它从 **ujs 的 Python 适配层**变成 **不依赖 ujs、可被第二个消费方复用的 `.cx` 模块**。

## 1. 现状与缺口（实测，非推断）

| 项 | 现状 | 缺口 |
|---|---|---|
| 依赖 | `flows.py` 直接 import `ujs.seed.construct.front.compile.compile_src` / `emit_wasm` / `Oracle`；`nets.py` 吃 `ujs.seed.construct.gold.STAGES` | 硬绑定 ujs，无法移植 |
| 消费方 | 全仓只有 `m4/__main__.py`、`m4/selftest.py` 引用 | 只有自己用，可复用性零证据 |
| 覆盖 | 28 个 ujs 语料，未覆盖即报错 | 覆盖率不足 |
| 决策网 | 10 网中仅 lex/parse/isel/enc 真决策；type/shape/ic 是观测，scope/irsel/reloc 保留 | 6 张网不参与推导 |
| 语言 | Python | 与 .cx 工具链（gate 检查器已 .py→.cx）脱节 |
| 流边界 | 流 1+2 共用 `compile_src`，流 3+4 共用 `emit_wasm` | 四流不是四个独立调用点 |

## 2. 目标定义：什么叫「可复用模块」

满足全部五条才算：

1. **依赖倒置**：模块不 import 任何 ujs/Python 符号；外部通过注入提供「标签源」与「发射钩子」。
2. **单一入口**：对外只暴露一个 `.cx` API 面（构造 / 推导 / 验收 / 记账查询），内部结构可自由重构。
3. **第二消费方**：至少一个**非 m4** 的调用方在闸内使用它，且行为可判。
4. **自举**：在 m9（当前 root 产品）上编译、运行，不依赖 Python 运行时。
5. **拒绝域完整**：未覆盖输入必须具名拒绝，绝不静默回退 gold。

## 3. 验收判据（硬，可机检）

| 编号 | 判据 | 检查方式 | 失败即回退 |
|---|---|---|---|
| D1 | `flownet/` 目录内不出现 `ujs` 符号 | `grep -rn "ujs" flownet/ --include=*.cx` 为空 | 本波不合并 |
| D2 | 字节 parity 不退化 | 28 语料 → 与 ujs gold 逐字节相同 28/28；acc 10/10 全键 exact；gold 回退计数恒 0 | 本波回退 |
| D3 | 第二消费方真实调用 | 该消费方在闸内跑通，且**移除模块调用后必须红**（变异证明，非摆设） | 本波不算完成 |
| D4 | m9 root 自举 | 在 m9 上编译 + 运行；不调 Python；无 `python3` 进程 | 本波回退 |
| D5 | 拒绝域 | 每个未覆盖语料：rc1、stdout 空、不产出文件、stderr 点名原因与位置 | 本波回退 |
| D6 | 单步 ≤60s | 每波新增的闸步都 ≤60 秒看门狗；整波可逐步复现 | 拆步 |

## 4. 工作流（W0–W5，每波独立可回退）

### W0 —— 可行性裁定（**先做，不做完不写代码**）

只需回答三个问题，用最小探针，不碰产品：

| 问题 | 探针 | 若否，方案调整 |
|---|---|---|
| `.cx` 能否表达 10 张网的键空间（合计约 3.6K 键：lex 100 / parse 330 / type 1863 / scope 48 / shape 54 / irsel 117 / ic 975 / isel 42 / enc 144 / reloc 4） | 生成一张最小的表驱动 `.cx`，在 m9 上编译并跑通 acc 的一个子集 | 改为「按需构造 + 稀疏表」，或把网降级为纯函数形式 |
| `.cx` 有无文件读写，能否读外部 gold 表（`.tsv`） | 一个 `.cx` 读表 → 打印；确认路径/错误处理 | 把 gold 表**编译进** `.cx`（生成源码），代价是编译时间 |
| `.cx` 有无模块/库引用机制（A.cx 用 B.cx） | 两个文件的最小示例，经 `commitgate` | 合并为单文件，用命名前缀分区 |

**产物**：`research/flownet-w0-feasibility.md`（三个结论 + 探针路径 + 实测输出）。
**闸**：探针各自可复现、≤60s。
**W0 结论（2026-10-23 实测，详见产物）**：①是——生成的表驱动 `.cx` 在 m9 上编译 2.85 s，10 网 3677 键穷举 10/10 exact（acc 子集与全量皆过，变异即红）；②是——单文件 `.cx` 只读外部 `.tsv`（网参数 + gold）即 10/10 exact，缺文件/目录/坏行均「路径[:行]: 原因」rc 1；③是——多翻译单元即模块机制（`unisacc.com A.cx B.cx`），入树探针 `tests/flownetw0.cx`+`tests/flownetw0mod.cx` 两个闸步经 commitgate，去掉 B 即 rc 127 具名拒绝。实测约束：未定义的外部**对象**被当作 0 读出不拒绝，故 flownet API 只导出函数、`fn_` 前缀。按 §8-3：不降级，W2 输入选运行时读 `.tsv`。

### W1 —— 行为基线快照（纯测量，零改代码）

- 固化当前 m4 行为：28 语料的输出 SHA256、acc 逐网 (exact,total)、trace 记账、selftest 的 EXPECT 表。
- 落 `tests/flownet-baseline.tsv` + 一个 `--check` 步骤。
- **闸**：基线自身稳定（连跑两次逐字节相同）。

### W2 —— `.cx` 骨架 + parity（仍不接第二消费方）

- 新增 `flownet/{contract,nets,flow,api}.cx`，语义对齐 m4 四模块。
- 输入仅来自 W0 定的表机制；**此波允许临时适配器**（读 W1 基线做对拍），但不得 import ujs。
- **闸**：D2 全绿（28/28、acc 10/10、gold 回退 0）；D1 已成立。

### W3 —— 接口反转 + 第二消费方（可复用性的唯一证据）

- 把「标签源 / 发射钩子」变成注入面；写 ujs 侧**薄适配层**（放在 `m4/` 或 `adapter/`，允许 Python 或 .cx）。
- 选一个既有 gate 检查器（建议 `inventory` 或 `auditnet`）改用 `flownet` 做一次真实推导。
- **闸**：D3（移除调用必须红）+ D4（m9 自举）。

### W4 —— 决策网扩展（观测 → 决策）

- 对 type / scope / shape / ic 四网，先定义「决策」的测量口径：**变异必须改变输出或拒绝**。
- 逐网做：改前端消费该网结果 → 用 selftest 变异探针证明输出/拒收变化 → 更新 EXPECT。
- 只做到「测得出 + 口径钉死」；`scope` 的不可达分支、`irsel` 的无询问点需先给出可达性方案，否则如实标 RESERVED。
- **闸**：每网一条变异探针，全绿才升格；不可达就保持 RESERVED，不得虚报。

### W5 —— 覆盖与文档

- 语料从 28 扩到全量（以仓内可编译集为准），逐类记录覆盖/拒绝。
- 交付 `flownet/README.md`（API、注入契约、错误码）与 `docs/flownet-migration.md`（如何从 m4 迁过来）。

## 5. 红线（违反即回退，不讨论）

1. **不许静默回退 gold**：gold 回退计数 >0 即失败（沿用 m4 现有强制）。
2. **不许把 28 语料当全覆盖**；覆盖率必须报「覆盖/拒绝/未测」三态。
3. **不许在 `flownet/` 里出现 Python 调用**；适配层只能在边界外。
4. **不许改编译器语义来迁就模块**：本需求不动 R0–R10 的 TDESC/state 切片。
5. **每波必须独立可复现、单步 ≤60s**；不接受「一次性大改 + 整闸」。
6. **D3 的变异证明不可省**：第二消费方移除调用后不红 = 可复用性未成立。

## 6. 与既有路线图的关系

- **不替代** `research/c43-roadmap-0043-and-refactor.md` 的 R0–R10（那是编译器内部 TDESC/state/cell 重构）。
- **并行**：本需求只做「范式模块化」，不改字节；R3 起会改字节，两者不要在同一个 commit 里混。
- **协同点**：R 路线 (E) 第 4 项（`export.py` → `stategen.cx` 产出 k2 事实）与本需求 W3 的「注入面」同向——都在把 Python 从关键路径上撤出。

## 7. 交付物

1. `research/flownet-w0-feasibility.md`（W0 结论）
2. `tests/flownet-baseline.tsv` + check 步骤（W1）
3. `flownet/*.cx` + 适配层（W2–W3）
4. 第二消费方的改动 + 变异证明（W3）
5. `flownet/README.md`、`docs/flownet-migration.md`（W5）
6. 每波一个 commit，message 带 `fix:`/`tests:` 前缀与闸结果

## 8. 开放问题（主人授 cc 自裁，2026-10-23 已裁，见各条「裁定」）

1. **模块归属**：`flownet/` 是新顶层目录，还是并入 `exec/`？（涉及打包清单与 derived-map）
   - **裁定：新顶层 `flownet/`，不并入 `exec/`。** `exec/` 是产品闭包（打包、provenance、reseal、freeze 口径都按产品管）；本需求承诺不改字节、不动编译器语义（§5-4、§6），放进闭包会把每一波都变成产品刀。`flownet/` 的检查步在 `release/derived-map.tsv` 与 `tests/ungated-checks.tsv`/gate 中单独登记。
2. **第二消费方选谁**：`inventory` / `auditnet` / 新写的 `.cx` 自测，三者成本与风险不同。
   - **裁定：`tests/tdescprobe.cx`（声明符 → 规范类型描述 TDESC 的派生）。** 理由：它本身就是「有限表上的一条流」，`.cx` 范式实验 E2 已用流单元跑通过同一推导（`research/c43-cx-paradigm-experiments.md`）；它在闸内（`tdescprobe-oracle`、`com-tdescprobe-1..3`），106 条结果可对 cc 机检，D3「移除调用必须红」可直接变异证明。`inventory` 的逻辑与流×网无关（硬套会成摆设）；`auditnet` 依赖完整产品构建与 model-audit，W3 成本与风险都更高；新写自测不满足「既有闸内调用方」。
3. **W0 若 `.cx` 表达不了表驱动**：是降级为「稀疏构造 + 纯函数」，还是先做 `.cx` 侧的最小表机制（那就是语言特性工作，范围外）？
   - **裁定：先按 W0 实测；若不行，首选「生成 `.cx` 源码、表编译进去」（§4 W0 表第二行已列的方案），其次稀疏构造 / 纯函数；`.cx` 语言特性工作不纳入本需求。** 注：`.cx` 已有文件读写（`include/cx.h` 的 `cx_read`/`cx_write`），读外部 `.tsv` 预期可行，W0 第二探针实测确认。
4. **W4 的 scope/irsel**：若前端不改就不可达，是标 RESERVED 收口，还是把「改前端」纳入本需求？
   - **裁定：不可达即标 RESERVED 收口，「改前端」不纳入本需求。** 改 ujs 前端属另一产品线，且 §5 红线 4 禁止为迁就模块改编译器语义；升格另立需求。
