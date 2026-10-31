# 词法上下文 preflight 测量提案（2026-10-31）

状态：**提案，不授权开测/建工具/改 cxmod 或 entry-build/spec**。本文件是可审合同草案，不是产品预算、不是身份一致性实现、不是超限拒绝负例。W-1/CX-U-4/W-6/W-7 不变。

## 1. 目的与不覆盖

测同一 commit 下，从显式根集合展开的**词法上下文**规模：contexts、edges、path bytes、单向 pair 遍历的 visited/peak；为未来身份一致性预算提供观察值。

dependency_diff>0 或 public_diff>0 均表示未证明一致，不能称 identity 通过。
明确不覆盖：
- 不证明现有仓低于任何产品上限；测量上限不是产品预算。
- 不实施合并前拒绝，不替代超限负例。
- 不做所有排列；pair-policy v1/v1r 只覆盖两种顺序。
- 不快照原子性；结束复核只是变化检测。
- 不覆盖 macOS/Windows/ARM，不安装候选，不转授当前工作区。
- 不把 `-cx-plan` 的 realpath 去重图当作完整词法图。

## 2. 真源：同 commit 树与 archive

- 选定 commit C（完整 sha）。共享 index 的 `git ls-files` 不作为真值。
- 树清单：`git ls-tree -rz --full-tree C`，保留 mode/type/object/path；原始输出保存并记 sha256。
- 快照：`git archive --format=tar C | tar -xf - -C out/context-preflight/<epoch>/snap`；tar 与解出文件清单各记 sha256。
- 核对门分类：
  | mode | 对拍键 | 结果 |
  |---|---|---|
  | 100644/100755 | path + mode + 解出文件 sha256（也记录 tree blob id） | 缺失/多余/内容或 mode 不同→SNAPSHOT-MISMATCH |
  | 120000 | path + mode + target 字节（不解析后比较） | 目标字节不同→MISMATCH |
  | 160000 | ls-tree 记录，archive 允许无该 path | GITLINK-UNEXPANDED + 相关根 UNKNOWN，不升格 MISMATCH；若解出同 path 常规文件→MISMATCH |
  保存 ls-tree 原始、extract 清单、分类计数。
- 120000 软链：按 target 字节单列；解析限快照内，累计 seen_paths（规范化词法相对 SNAP）；再遇已见→symlink-cycle；绝对目标、`..` 逃出 SNAP、悬空分 reason→UNKNOWN；禁止回读工作区/仓外。目录软链+词法 `..` 不等于 OS 父目录，不标 symlink-cycle，按多 context 计数。软链不进 `--in`。
- 路径无法无损表达（换行/回车/NUL、以 `#` 开头等）→具名拒绝/UNKNOWN。
- 快照可变：所有实际读取文件、每级解析与 symlink 目标在结束时复核 sha/identity；检测到变化→该根 UNKNOWN，不称原子只读。

## 3. 索引与产品身份

- `--in` 列表只含树中 mode 100644/100755 且 path 以 `.cx` 结尾的普通文件，相对快照根；清单 sha 记录。
- 在快照根执行：`sh <PRODUCT_ABS> tools/symindex.cx -- --index <OUT>/index --in <OUT>/in.lst update --verify`。
- `PRODUCT_ABS` 是显式传入的已装产品绝对路径，记录 sha256 与版本；不从 PATH/仓库根自动猜。当前示例：`/home/devali/repos/unisacc/unisacc.com`，`unisacc 0.0.43`，sha256 `fc1d4513fb1363c8506df8afa08444882337e615bf38d82dd42f022e2c23dc6f`；实际测量必须重新显式绑定，不继承示例。
- 索引出现 `DEFERRED`（45s 上限停止）或 manifest 不完整→本次 UNKNOWN，不当完整索引。
- 测量回执强制字段：`C, lstree_sha, archive_sha, extract_sha, in_sha, index_sha, product_path, product_sha, policy_doc_sha, implementation_source_sha, implementation_binary_sha, ENFORCEMENT, roots_tsv_sha`。
- cxm_norm 实际行号：src/cxmod.c:113、cx/compiler/cxmod.cx:143、cx/cx0/plan.cx:163（以当前文件为准复核对拍）。
- 根生成依赖 symindex `D main`，前提是索引完备；索引 DEFERRED/未完成即 UNKNOWN。

## 4. 根清单：来源、分类与核对门

拟提交 `tests/cx-context-roots.tsv`（未建），列：
`root<TAB>joint_id<TAB>scope<TAB>double_count<TAB>category<TAB>participation<TAB>origin<TAB>provenance<TAB>expect<TAB>sha256`

- `scope` 必填：`single-call` / `joint` / `build-instance`（§14.1）；`joint_id` **仅** `scope=joint` 时非空。
- `participation` 仅 `category=production` 必填：`declared` / `observed`；其他类别留空。

`origin` 必填，只取四值之一，指明该行**由哪个来源产生**，不代表已捕获执行身份：
- `recipe`：来自构造配方的**源码声明**（`tools/entry-build.cx` 的 `src_rel`/`keys` 表）。
- `tree-main`：来自 commit C 树中 `.cx` ∩ 索引 `D main`。
- `batch`：来自 `tests/batches.tsv` SOURCE（批次 id 记于 provenance）。
- `generated`：测试运行时生成或不在 tracked 树中。

**四源不得互相顶替**：`recipe` 是源码声明，**不是实际 argv**；`tree-main` 是**索引发现集**，**不是全集**；
`batch` 是测试登记，不是构造成员。每行只允许一个 `origin`；跨源重合另起行或显式 `double_count`，
不得合并成一个"已核"的根。

类别（不得由目录名推断，见 §4.1）：
- `production`：参与产品构造的单元。**必须另带 `participation` 取值**，二者不可互相顶替：
  - `declared`：只在配方（`origin=recipe`）出现，**未捕获本次 argv**。只证明"被配方列出"，**不证明实际参与**；
  - `observed`：有该**构造实例**的捕获 argv，且该单元确实出现在其中，才可写"实际参与"。
  `recipe_argv_status=NOT-CAPTURED` 时，所有 `production` 行只能是 `declared`，不得写"已证明参与"。
  理由（源码事实，实测 UNMEASURED）：构造器接受可重复 `--only KEY`（`tools/entry-build.cx:408-411`，≤33）
  与 `--link-only`（`:395`），`:485-495` 比对、`:505` 按 `eb_wanted` 跳过未选中根 ⇒ 一次构造 argv 只编
  一个根也是合法调用 ⇒「参与」是**每个构造实例**的性质，不是静态成员属性。
- `tool`：不属于产品构造、且**已确认**为独立可执行入口的单元。
- `batch-positive` / `batch-negative`：来自 `tests/batches.tsv` SOURCE。
- `generated`：路径由测试运行时生成或不在 tracked 树中的入口；expect=unknown-ok。
- `unclassified`：未归类；不算覆盖，聚合 UNKNOWN。

### 4.1 分类不靠目录猜（基线绑定 commit，不得沿用）

基线：**C = `fc628a0403a788c0b5cd18d31f2d1e6f6f70b9f3`**，`tests/batches.tsv` sha256
`504eadce9d167af72eb8d9a78a4a746e848da12f58083790d4e29d61ae072222`。
下表由该 commit 的树直接算出（不读工作区）：

| 观测（@C） | 值 |
|---|---|
| 树中 mode 100644/100755 的 `.cx` | 1911 |
| 树中 mode 120000 的 `.cx` | 42（不进 `--in`） |
| 含 main 定义的 `.cx`（S2，文本口径） | 850 |
| `tools/` 含 main | 330 |
| 其中登记为批次 SOURCE | 114 |
| **无任何批次** | **216** |
| `src/` 含 main | **仅 `src/unisacc.cx`** |
| 配方 `src_rel` | **33 项**（`src_rel[0..32]`，全唯一） |

⇒ 1. `tools/**` 中 216 个含 main 的单元**既不能默认算 tool、也不算 production**，只能落
`unclassified` 直至逐个确认；按目录批量归为 `tool` 会把一块未知伪装成覆盖。
⇒ 2. `src/**` 若按"含 main"选根会**漏掉 32 个配方成员**。产品根必须由 `origin=recipe` 提供，
不能由 `tree-main` 反推，也不能由目录名反推。

**数字是绑定 C 的观测值，不是合同，也不是当前工作区值**（`tests/batches.tsv` 每轮都在变：同为
`layer=contract` 已从 133 涨到 136）。每次测量必须按实际 C 重算并重报，禁止沿用本表。

### 4.2 候选全集：三源并集 + 差集分报

1. **S1 = recipe**：`tools/entry-build.cx` 的 `src_rel[0..32]`（33 项，无重复）。同时记
   `recipe_argv_status=captured:<sha256>|NOT-CAPTURED`；`NOT-CAPTURED` 时 S1 只表示"配方**声明**"，
   **不等于实际 argv**，不得据此称已捕获执行身份。
2. **S2 = tree-main**：commit C 树中 mode 100644/100755 的 `.cx` ∩ 索引 `D main`。
3. **S3 = batch**：`tests/batches.tsv` 的 SOURCE 去重（@C 为 216 个唯一值）。
4. 候选 = S1 ∪ S2 ∪ S3；差集**分列报告**，不合并、不互相顶替：

   | 差集(@C) | 数量 | 处理 |
   |---|---|---|
   | `S2 \ S3` | 634 | 逐项落 `tool`/`unclassified`；**默认 unclassified** |
   | `S3 \ S2` | 0 | 非零时**按行三分**（见 §4.2a），不得由"不在 S2"直推 STALE |
   | `S1 \ S2` | 32 | **正常**：配方成员无 main。属 **build-instance 的独立计划成员**（`scope=build-instance`、`joint_id` 留空），**不是联合根成员** |
   | `S2 \ S1` | 849 | 报出，不算错 |

   `S1 \ S2` 不得称"联合根成员"：§14.1 的联合根是**同一次调用**的显式多根；`entry-build` 的
   `eb_plan`/`eb_compile` 每次调用只追加**一个** src（`:151-166`、`:227-245`，循环 `:508-523`/`:527-533`），
   33 个 TU 是 **33 次独立调用 + 一次链接**。跨计划 realpath 去重发生在 `ncl`/`eb_creal` 域（`:521`、
   `:558-570`），是构造级合并，不是一张图。

5. 每个候选必须在 roots.tsv 出现，缺失→`ROOTS incomplete`、聚合 UNKNOWN/非零；多余行→`ROOTS stale`
   单列；候选数超 ROOTS 上限→UNKNOWN/非零，不截断。

#### 4.2a `S3 \ S2` 的三分（不得由"不在 S2"直推陈旧）

| 行特征 | 记法 | 根状态 | 是否算 STALE |
|---|---|---|---|
| `mode=SOURCE`，源是模块/夹具/负例，本就不定义 main | `no-main-by-design` | UNKNOWN（无根） | 否 |
| `mode=PREPARED`，`entry` 是模板占位（如 `{nets}/nets.cx`、`{assembler}/contracts.cx`），材料未展开 | `prepared-template` | UNKNOWN | 否 |
| `mode=SOURCE`，源本应定义 main 但树中不存在 | `absent-from-tree` | UNKNOWN | **候选** STALE，需人工复核后才可记 |

另：`S2` 本身是**发现集**——不含 mode 120000 软链、不含 manifest 外路径 ⇒ "不在 S2"与"陈旧"无因果关系。

**S2 是发现集，不是全集**：`D main` 只覆盖索引 manifest 内路径；mode 120000 文件软链**不进 manifest**，
其目标即使含 main 也不在 S2。

@C 实测（`git ls-tree` + blob 内容，不读工作区）：

| 项 | 值 |
|---|---|
| `.cx` 软链 | 42 |
| 目标在 C 树内存在 | 31 |
| **目标在 C 树内不存在（悬空）** | **11** |
| 目标含 main | **0** |

⇒ 1. 就当前 C，"按含 main 选根"不会因软链而漏入口（0/42），但这是**观测巧合，不是合同**；
每次测量仍须重算。
⇒ 2. 42 个目标**全部**含 `..`（如 `adapter/ujswasm/flownet/api.cx → ../../../flownet/api.cx`），
   归一化后仍落在 SNAP 内。故"`..` 越界"必须定义为**归一化后逃出 SNAP 根**，不能按"字符串含 `..`"
   判定——按后者会把 42/42 全判 UNKNOWN，属于口径错误而非安全收紧。
⇒ 3. 11 个悬空目标必须逐条 `UNKNOWN`，不得静默跳过；它们同时也是索引 `--in` 不会覆盖的路径。

### 4.3 `batch-negative` 只能由 `layer` 列判定

实测 @C（345 数据行）：`implementation` 全 IMPLEMENTED，`expect` **全为 `rc0:*`**；`run-batch` 判 PASS
只查"无超时 + rc0 + 候选标识前后一致 + 标记行恰一次"，**不涉及被测内容正负**。
正负信号只在 `layer` 列（contract 136 / interfaces 107 / **negative 62** / properties 15 /
platform 10 / library 8 / product 7）。
⇒ 不得由 `expect` 或批次 id 推断负例；反例：id 含 `negative` 的 `full-negative`，其 source 为
`tests/full-domain.cx`、`layer=product`。`batch-negative` 只作报告分组（`layer=negative` 投影），
不参与覆盖计量；`negative-known` 当前上限 0 行（逐行理由未给出）。

### 4.4 计数单位：按行，不按调用

本节所有候选/对照计数以 **roots.tsv 数据行**为单位：一个候选一行，一行只归一个 category 与一个
`origin`。不得用"调用次数"或"编译单元数"充作候选数；构造级的调用/对象计数见 §14.3，两者不换算、
不相加。`double_count` 仅在联合根成员与独立根同时出现时置 1，聚合只取联合根那份，否则核对门拒绝。

### 4.5 §4 新增的 UNKNOWN

| 项 | 状态 |
|---|---|
| 实际构造 argv / 执行身份 | **NOT-CAPTURED / UNKNOWN**（S1 只是源码声明） |
| 216 个无批次 `tools/` 含 main 单元归 tool 还是 product | **UNKNOWN**（未逐个确认） |
| S2 对 120000 软链目标的覆盖 | **不覆盖**，缺口数 UNKNOWN 待按 C 重算 |
| `mode=PREPARED` 的 2 行（entry `{nets}/nets.cx`、`{assembler}/contracts.cx`） | 材料占位，**根身份 UNKNOWN**，不记覆盖 |
| `S1 \ S2` 中 32 个配方成员的 `joint_id` | **必须为空**（`scope=build-instance`）；标成 joint 即口径错误 |
| `cycles` / `cycle_path` | 判环阶段未实现 ⇒ **UNMEASURED**，不得写 0 或"无环" |

**UNKNOWN 白名单（决定 §9 的 `unknown_known`）**：上表各行原因、以及 §5 的 `unresolved`/`manifest` 读失败/
软链悬空/`recipe_argv_status=NOT-CAPTURED` 等**已在本文件列明**的原因，记 `unknown_known`；
其余一切 UNKNOWN 记 `unknown_unexpected`。二者之和必须等于 `unknown`。

## 5. 词法展开语义

- context 键：对“父 context 词法路径目录 + 模块名点转斜杠 + `.cx`”执行与编译器**同一 cxm_norm**（src/cxmod.c:113；cx/cx0/plan.cx:163；cx/compiler/cxmod.cx:143）：去除空段/`.`，`x/..` 词法折叠，不做文件系统查询。不得自创仅压缩斜杠。
- 注意并记录：`x/..` 按词法折叠，不按“x 是软链时 OS 解析到目标父目录”；这是现行编译器实际语义。preflight 只测量，不修正。
- 每条 `use` 从父 context 目录解析；模块名语法同编译器；`std` 按现行处理；不 realpath 合并 context。
- 同一根内相同规范化字面路径只展开一次；不同根分别展开；聚合再按字面路径去重。
- 环：**建图与判环分两个阶段**，不得用"已见即跳过"承担判环。
  1. **建图阶段（BFS）**：每条 `use` 的规范化字面目标各作为一个 context 入队（`context_queue_len_peak`）；
     "已见即跳过"**只**用于 **context/边去重**，产生去重后的邻接表，不产出任何环结论；
  2. **判环阶段（独立算法）**：对去重邻接表做三色 DFS 或 Tarjan SCC，报 `cycles` 与至少一条具体环路径
     （`cycle_path`，形如 `b -> c -> b`）。只有该阶段可写 `cycles`。
  理由：§7 的遍历是 BFS 队列，没有"当前展开栈"；BFS 用已见集时**交叉边**环检测不到。
  实例（仓外夹具，已删）：`A→B, A→C, B→C, C→B` 时 BFS 父链 `parent(b)=A`、`parent(c)=A`，`b→c`/`c→b`
  目标均已见 ⇒ 按已见集得 `cycles=0`；同夹具下编译器 DFS 报 `cx: module cycle: b -> c -> b`（rc1，
  换根报 `p.b -> c -> p.b`、`p.c -> b -> p.c`）⇒ 已见集**漏报**。
  编译器既有 module cycle 拒绝另记，不混报；判环阶段未实现前 `cycles` 一律 UNMEASURED、不得写 0。
- 环只计**图**上的环（同一根内）；跨根的同一 realpath 不构成环（§14 分区）。
- 依赖目标比较：两边 use 目标经 cxm_norm 后 stat/realpath 比较；缺失=unresolved，ENOTDIR/EACCES/读失败=UNKNOWN。
  **两侧 realpath 不同 ⇒ 二者是不同身份，不是同一个 `canonical→candidate` 核验对**：分别作为 context 入图
  继续探索、只记 `pairs_dependency_diff`，互相不生成 pair，也不互比公开集（详见 §7）。
- 缺失模块=unresolved+UNKNOWN；ENOTDIR/EACCES/读失败/路径规范化失败（过长/段数）=INVALID+UNKNOWN；不截断、不降级。
- U 事实：由快照索引 shard 读取；context 的 realpath 映射回 tracked path/sha 后取 shard；alias 软链用 realpath 查，但 context 身份仍是字面路径。未映射/未跟踪→UNKNOWN。
- 清单：按 context 词法目录+尾名读 cxpack；ENOENT/无匹配行=全集；EACCES/ENOTDIR/读失败/超长/非法行→UNKNOWN。有效公开集的比较只按第 7 节的 pair 策略执行；比较结果与计数见该节，§5 本身不另做拒绝。
- 跨入口对拍（开测前必须）：夹具覆盖 `./`、空段、`a/../b`、目录软链+`..`、绝对/相对；preflight 键与 unisacc 当前入口、gcc_cx/clang_cx 实际解析路径逐项一致，否则设计不通过。

## 6. 计数口径

- `contexts_total`：该根不同 context ID 数（累计集合大小）。
- `edges_total`：已遍历 use 指令数（累计，含 unresolved/cycle）。
- `path_bytes_total`：该根所有 context 规范化字面路径长度+1 的累计和；另报 longest_path。
- `max_group`：同一 realpath 的 context 组最大成员数；同 realpath 只用于报告与 pair，不用于展开去重。
- `unresolved`、`cycles`、`manifest_errors`、`invalid` 分别计数。
- `std_edges`：`use std`/`std.cx` 的边数（不建 context、不展开）；`use std.X` 按编译器具名拒绝，单列。
- `pairs_dependency_diff`：同一条 `use` 的多个字面目标解析到**不同 realpath** 的计数。**它们不构成 pair**；
  两侧分别入图探索，本计数只记账，不影响入队（§7）。
- 聚合：各根累计求和 + 跨根 unique_contexts（按字面路径去重）；不把求和冒充全局同时峰值。

## 7. pair-policy v1（测量策略，不是生产算法）

版本标识：`pair-policy-v1`。若生产算法改变，另立版本，不复用此数。

- canonical 选择：**根内**每 realpath 组按“BFS 层序 → use 行序”首个 context 为 canonical；roots.tsv 行序只决定报告遍历，不能跨根选 canonical（见 §14）。联合根必须显式标记，联合根内部才共享一个图。
- 方向：单向 canonical→candidate；memo 键 `(canonical context ID, candidate context ID)`；每个 candidate 对照其组 canonical 执行递归核验（依赖递归同样用该组最新选定的 canonical）。
- 遍历：显式工作队列，按上述顺序；不递归 C 栈。`pairs_visited_total` 记首次进入的 memo 对。
- **队列与栈按元素单位分开计峰**（不得相加，不得互相顶替）：

  | 字段 | 元素单位 | 可为数 | 否则 |
  |---|---|---|---|
  | `context_queue_len_peak` | context（待探索模块入口） | 队列确有 context 单元 | — |
  | `pair_queue_len_peak` | pair（同 realpath 组 canonical→candidate） | 队列确有 pair 单元 | 实现只有一个混合队列 ⇒ **NOT_APPLICABLE**，不得用混合长度顶替 |
  | `work_queue_len_peak` | 混合，单位须在收据显式声明（如 `context+pair`） | 实现确用单一工作队列 | 纯 pair 队列 ⇒ NOT_APPLICABLE |
  | `pair_verify_depth_peak` | 栈帧（单次核验的递归/显式栈深度） | 算法确有核验栈 | **纯迭代、无验证栈 ⇒ NOT_APPLICABLE**，不填 0、不虚构深度 |

  `NOT_APPLICABLE`（算法结构里不存在该量）≠ `UNMEASURED`（结构存在但未测）≠ `0`（测到零），三值不互换。
  需要总数只用 `sum_` 前缀（§14.1），不同单位不相加。
- 第二策略 `pair-policy-v1r`：同一根仅把兄弟 use 行序逆序，其余同 v1；单独报告 visited 与四个 peak。明确只覆盖两种顺序，不等于所有排列。
有效公开集比较与计数策略（v1 全遍历）：
- 每个 pair 比较 canonical 与 candidate 的有效公开集（按 §5 的清单规则，逐名集合；无清单/无匹配行=全部非 static）。记录 `pairs_public_equal`、`pairs_public_diff`、`pairs_errors`。
- v1 **不因差异或错误停止遍历**：有效集差异只记录；上下文/清单/目标解析错误计入 `pairs_errors`，该出错边不入队继续展开，其他边继续；根标 UNKNOWN。`pairs_visited_total`/四个 peak 只是该全遍历策略的测量值，不是任意生产算法的值。
- 不能对“任意首差异即停算法”声称上界：只有当生产算法与 v1 使用**相同的配对生成、去重键、canonical 选择和工作队列调度**，且停止仅裁剪同一条遍历时，全遍历值才可作该算法的条件上界；canonical 或调度不同可能使 peak 更大。除此之外一律 UNKNOWN，不比较、不外推。
- 遇错状态机（必须逐条实现）：
  | 事件 | 计数 | 入队/展开 | 根状态 |
  |---|---|---|---|
  | public set 相等 | public_equal++ | pair 已访 | 不变 |
  | public set 不等 | public_diff++ | 不因 diff 停；依赖子 pair 仍可入队 | 不变（纯测量） |
  | 清单 EACCES/ENOTDIR/读失败/非法 | pairs_errors++、manifest_errors++ | 该 context 出边不入队 | UNKNOWN |
  | use 目标缺失 | unresolved++、pairs_errors++ | 不入队 | UNKNOWN |
  | 目标 realpath 不同 | dependency_diff++ | 两侧**各自作为 context 入图**（`contexts_queued++`）继续探索；**互相不生成 pair、不互比公开集**；diff 只记账，不阻止任何入队 | UNKNOWN（该 use 边身份不明） |
  | 规范化失败 | invalid++、pairs_errors++ | 不入队 | UNKNOWN |
  | proxy=1 | equal/diff/errors 必须为空/禁用峰门 | — | 该根只能 `UNKNOWN`，不得写任何 measurement_complete 形式 |
  `pairs_visited_total` 含首次进入 memo 的出错 pair。
  **入队与生成 pair 是两件事**（消除原"各按字面目标分别入队"与"差异只停该边展开"的互斥）：

  | 动作 | 触发 | 计数 |
  |---|---|---|
  | 图探索入队 | 每条 `use` 的字面目标各自作为 context 入队列（用于建邻接与 realpath 组） | `contexts_queued` |
  | 生成 pair | **仅当** candidate 与 canonical **同 realpath 组**（同身份兄弟 context） | `pairs_visited_total`（memo 去重后） |
  | `pairs_dependency_diff` | 同一 `use` 的多个字面目标指向不同 realpath | 只记账，既不生成 pair，也不阻止入队 |

  某侧解析失败走 `unresolved`/`invalid` 行，仍不入 pair。
- 若实现没有执行有效集比较，只能输出 `pairs_proxy=1`，不得称实际递归核验，也不得用于 pair 峰门。
- 输出字段统一：`pairs_visited_total`、`contexts_queued`、`context_queue_len_peak`、`pair_queue_len_peak`、`work_queue_len_peak`、`pair_verify_depth_peak`、`pairs_public_equal`、`pairs_public_diff`、`pairs_dependency_diff`、`pairs_errors`、`pairs_proxy`（0/1）、`std_edges`；v1r 另列 `pairs_visited_total_v1r`、`contexts_queued_v1r` 与上述四个 peak 的 `_v1r` 版本。
  **不设单一 `pair_peak` 字段**：它同时暗示"只数 pair"和"队列与栈合一"，两者都不成立。
- pair 数字只说明该策略的测量值，不作为生产实现上限或产品预算证明。

## 8. 测量上限与 deadline（独立于产品）

候选测量安全上限（待核准，不是产品合同）：
- MAX_ROOTS = 2048（候选超限 UNKNOWN/非零）
- MAX_CONTEXTS_PER_ROOT = 20000
- MAX_EDGES_PER_ROOT = 200000
- MAX_PATH_BYTES_PER_ROOT = 8 MiB
- MAX_MS = 50000（外部 60s 内留清理/汇总）
达到任一上限或超时→该根/聚合 UNKNOWN、rc 非零；不输出“低于上限/完整”。

## 9. 输出与状态

每根：
`ROOT<TAB>joint_id<TAB>scope<TAB>double_count<TAB>category<TAB>participation<TAB>status=measurement_complete|UNKNOWN<TAB>reason<TAB>contexts_total<TAB>contexts_queued<TAB>edges_total<TAB>std_edges<TAB>pairs_visited_total<TAB>context_queue_len_peak<TAB>pair_queue_len_peak<TAB>work_queue_len_peak<TAB>pair_verify_depth_peak<TAB>pairs_visited_total_v1r<TAB>contexts_queued_v1r<TAB>context_queue_len_peak_v1r<TAB>pair_queue_len_peak_v1r<TAB>work_queue_len_peak_v1r<TAB>pair_verify_depth_peak_v1r<TAB>pairs_public_equal<TAB>pairs_public_diff<TAB>pairs_dependency_diff<TAB>pairs_errors<TAB>pairs_proxy<TAB>path_bytes_total<TAB>longest_path<TAB>max_group<TAB>unresolved<TAB>cycles<TAB>cycle_path<TAB>manifest_errors<TAB>invalid<TAB>sha256`
聚合（**每类别一行，不合并**）：
`AGGREGATE<TAB>category=<c><TAB>total=<n><TAB>measurement_complete=<n><TAB>unknown=<n><TAB>unknown_known=<n><TAB>unknown_unexpected=<n><TAB>status=<s>`
逐 `production` / `tool` / `batch-positive` / `batch-negative` / `generated` / `unclassified` 各一行；
缺行即 `AGGREGATE incomplete`（rc 非零），不得只出 production 一行。

**总完成的类别集合事先固定**（发布后只能加门、不能减门；减门须另立版本并重跑）：

| 类别 | 是否计入总完成 | 说明 |
|---|---|---|
| `production` | 是 | 核心 |
| `tool` | 是 | 含 main 的可执行入口，**不得**因已知难测而移出（`tools/` 下 216 个无批次单元属此类） |
| `batch-positive` | 是 | 测试登记的阳性入口 |
| `batch-negative` | 否（单列） | UNKNOWN 不算生产覆盖，也不阻塞；计数仍须报 |
| `generated` | 否 | 永久不计 |
| `unclassified` | **禁止** | 出现在必需集合即为配置错误（rc 非零），它不算覆盖 |

状态命名规则：
- 只有**必需集合全部** `measurement_complete`、且无 `ROOTS incomplete`、无触限/超时，才可写
  **`aggregate_measurement_complete`**；
- 若仅 `production` 完成而其他必需类别未完成，只能写 **`production_measurement_complete`**，
  **不得**写 `aggregate_measurement_complete`，也不得写无前缀的 `measurement_complete`；
- 一律不写裸 `complete`，避免被读成全仓/规范通过；未达条件 → `UNKNOWN` 且 rc 非零。

两个 UNKNOWN 计数的定义（此前全文未定义，白名单见 §4.5）：
`unknown_known` = `reason` 命中 §4.5 白名单；`unknown_unexpected` = 其余一切。每个 UNKNOWN 根二者择一，
且 `unknown_known + unknown_unexpected = unknown`；不满足即聚合 rc 非零。

## 10. 拟路径与所有权

- `plans/cx-context-preflight-2026-10-31.md`（本文件，提案）
- 未来获批后：`tools/cx-context-preflight.cx`、`tests/cx-context-roots.tsv`、`tools/check-cx-context-preflight.cx`、batch `cx-context-preflight`
- 快照/索引/报告只写 `out/context-preflight/<epoch>/`；不写仓库根、不改已装产品、不改 cxmod/entry-build/spec。
- 未获四项细化核准前不建上述工具/测试文件、不开测。

## 11. 测中不写快照的强制措施

- 首选：独立 mount namespace + 只读 bind。示意（细节须先做 dry-run 验证）：
  bwrap --unshare-all --die-with-parent --chdir /work \
    --ro-bind SNAP /work --ro-bind PRODUCT /product \
    --ro-bind /usr /usr --ro-bind /lib /lib --ro-bind /lib64 /lib64 \
    --bind OUT /out --tmpfs /tmp --proc /proc --dev /dev \
    --setenv TMPDIR /tmp --setenv UNISACC_RUN_CACHE 0 \
    /product/unisacc.com /work/tools/cx-context-preflight.cx -- ... --out /out/report
  其中 SNAP 与 PRODUCT 外置且只读；OUT 是快照外唯一可写处；索引 /out/index、日志 /out/logs、临时 /tmp。cwd=/work，工具只接收 /work 或 /out 下的显式路径；宿主工作区不挂载。
  动态产品需要的解释器/库按实际 ldd 单独 ro-bind；不把整个 / 暴露为可写。
- 先证明方案可用：完整 dry-run 清单：bwrap 启动；/work 写失败 errno；/out 写成功；/product 只读可见；所需解释器/库可加载；宿主工作区路径探测失败；可见路径白名单核对。任一失败→UNKNOWN，不进入测量。
- 若 `bwrap` 或等价只读挂载不可用，本次测量状态 UNKNOWN，不用“前后复核”冒充强制只读。
- 次选：`chmod -R a-w SNAP` 并记录 mode；这只是同用户可逆约束，不构成强制。采用时 `enforcement=permission-only`，强制状态必须 UNKNOWN，不得写任何 measurement_complete 形式。
- 快照外目录：`out/context-preflight/<epoch>/{index,logs,tmp,report}`；`TMPDIR`、索引路径、工具输出均显式指向这里。
- 已装产品：记录绝对路径、版本、sha256；`UNISACC_RUN_CACHE=0` 只作为候选设置，不能证明无写入。若使用产品 `-run` 执行工具，另设只读 bind/沙箱；否则改为预先构建的工具二进制并用显式产品身份绑定。任何对快照的写入或无法排除的写路径→`SNAPSHOT-WRITE-UNKNOWN`，聚合 UNKNOWN。
- 结束复核仍是检测而非原子保证；只有强制只读措施成功才标 `enforcement=readonly-mount`。

## 12. 根清单与 pair-policy 文件身份

- 根清单：未来 `tests/cx-context-roots.tsv` 的 sha256 与 commit 一并记录；本文件只给生成规则与核对门，不内嵌未生成的根行。
- pair-policy 身份：`pair-policy-v1` 的定义即本文件第 7 节；记录三个独立字段：`policy_doc_sha`（本文件 sha256）、`implementation_source_sha`（实际 preflight .cx 源 sha）、`implementation_binary_sha`（实际执行二进制/产物 sha）。本文件 sha 只是提案身份，不能替代实现身份；测量数字必须绑定后两者。规则或实现变更必须改版本号，不复用 v1 数字。

## 13. 只读强制能力探针（先探测，后谈测量）

独立探针只回答“本机能否强制快照只读且隔离宿主工作区”，不读源码、不建索引、不做 preflight 测量。步骤与判定：

1. `command -v bwrap`；若存在，运行 dry-run：
   `bwrap --unshare-all --die-with-parent --ro-bind SNAP /work --bind OUT /out --tmpfs /tmp --proc /proc --dev /dev --chdir /work sh -c 'touch /work/probe 2>/dev/null; test $? -ne 0 && touch /out/probe && test -f /out/probe'`
   同时验证 `/product` 只读可见、所需解释器/库可加载、宿主工作区路径不可见。全部成功→`ENFORCEMENT=bwrap-ro-bind`。
2. 若无 bwrap，尝试 rootless mount namespace（含 private propagation）：
   `unshare --user --map-root-user --mount --propagation private sh -c 'mkdir -p /tmp/probe-snap /tmp/probe-out; mount --bind SNAP /tmp/probe-snap; mount -o remount,bind,ro /tmp/probe-snap; touch /tmp/probe-snap/x 2>/dev/null; test $? -ne 0; touch /tmp/probe-out/x'`
   只有“只读 bind 拒绝写 + OUT 可写 + 宿主路径不可见”三项同时成立才记 `ENFORCEMENT=unshare-ro-bind`；单 `unshare --user --map-root-user true` 成功不算。
3. 任一命令缺失/权限拒绝/挂载失败/无法验证隔离→`ENFORCEMENT=none`，本次 preflight UNKNOWN；不得退化为 chmod/permission-only 后称只读。
4. 记录每条探针命令、退出码、mount 表（若可读）、产品 `file`/`ldd` 与所需只读绑定清单；探针失败不阻塞设计评审，但阻塞测量。

本机当前观察（仅探针，不是 preflight）：`command -v bwrap` 退出1（未找到）；`unshare --user --map-root-user true` 退出0；另一次独立探针在自有 out/cx-ro-probe-FUiMLX 下用 `unshare --user --map-root-user --mount --propagation private` 对 bind 后 remount,bind,ro，触摸得到 `Read-only file system` 并打印 READONLY_WRITE_REFUSED、rc0。这只证明当前 Linux 用户+mount namespace 的只读 bind 可用；尚未证明运行 cx 工具所需解释器/库/产品依赖齐全、其他路径不可写/工作区不可见、快照合同完整。因此 `ENFORCEMENT` 仍不是完整通过，测量保持 UNKNOWN，直到第 1/2 条完整 dry-run（含依赖与隔离）通过。

## 14. 根分区与作用域

- 每个根代表独立编译图。context ID、canonical 选择、pair memo 与 pair 统计都以根身份分区：ID 含 root 前缀，或每个根独立编号后加 root；不得跨根按同一 realpath 合并、比较或声称会合并——它们未必在同一链接图。
- roots.tsv 的行序只决定报告/遍历顺序，不能把另一根的消费结果当本根 canonical，也不能用根序推断跨根合并。
- roots.tsv 增列 `joint_id`：空=独立根；同一 `joint_id` 的多行组成一个联合根，成员 root 行可重复。无 joint_id 的多 path 行拒绝。联合根与成员单根同时存在时须显式 `double_count=1`，否则核对门拒绝重复计量。
- 多源码同一命令是“联合根”场景，必须显式加 category/joint 标记；一个联合根的图内可以有自己的 realpath 组与 pair，但不等于把多个独立根合计后再比较。
- 各根 `pairs_visited_total` 与四个 peak 的作用域仅该根（或显式联合根）；跨根求和只作统计量，不能作为任一图的峰值或产品上限证据。
- 跨根 aggregate 的 unique_contexts 也只用于规模统计，不触发身份比较或拒绝。

### 14.1 三个身份/计数域，互不替代

| 域 | 定义 | 事实来源 | 只可断言 |
|---|---|---|---|
| **单次调用** single-call | 一次 `cxm`/产品调用的根集合（通常一个 src） | 该次 argv（未捕获则 UNKNOWN） | 该调用内部的 context/pair 峰值 |
| **联合根** joint | 同一次调用中显式多根，共享一张图，`joint_id` 标记 | 该次 argv + roots.tsv 标记 | 联合根内部的 realpath 组与 pair |
| **build-instance** | 一个构造/链接实例：多次计划调用 + 一次链接 | 构造器源码行为；实际 argv 未捕获=UNKNOWN | 构造级候选与去重，**不是**任何一次调用的图 |

roots.tsv 的 `scope` 列只取 `single-call` / `joint` / `build-instance`；`joint_id` 仅 `scope=joint` 时非空。
`S1 \ S2`（配方成员）全部为 `scope=build-instance`，**`joint_id` 必须为空**。

三条硬规则：
1. **单根/单调用合格不蕴含 build-instance 身份一致**，反之亦然；三者结论不得互相引用为证据。
2. 每个域的求和域与峰值域**分开命名**：`*_total` 累计和只能进 `AGGREGATE`，`peak`/`max_*` 只在该域内部成立；跨域比较一律 UNKNOWN。
3. `sum_` 前缀只用于跨域/跨 joint 统计，且不得写成任何 `peak`/`max_*`。

### 14.2 build-instance：跨计划 realpath 去重（源码事实；实测 UNMEASURED）

核 `tools/entry-build.cx`：

| 事实 | 位置 |
|---|---|
| `eb_plan` argv 只追加**一个** src | `:151-166` |
| `eb_compile` 同样只追加**一个** src | `:227-245` |
| 33 个 TU 分别 `while (i < 33) eb_compile/eb_plan` 逐个调用 | `:508-523`、`:527-533` |
| 33 次 plan 输出**全部**进同一 `ncl`/`eb_creal` 去重域 | `:521`、`:558-570` |
| 超 `EB_MAXC` → `eb_die("closure-cap")`（中止，非静默） | `:571` |
| 接纳后逐个 `eb_compile`，最后一次 `eb_link` | `:589`、`:606` |

⇒ 这是**构造级跨 root 合并**：两个不同词法根若 realpath 命中同一键，构造期只编译一次。单根 preflight
按词法根分别求和会**多计**；按 realpath 合并只报一份又**无法区分**"同根内重复"与"跨根合并"。

`eb_real`（`:346-349`）在任何 `sy_realpath` 失败（ENOENT/EACCES/ENOTDIR/ELOOP/过长）时**静默回退**
词法路径，调用方拿不到错误类别 ⇒ 同一物理文件可能被视为两个单元（漏合并），也可能两次都回退而
"看起来未重复"。`sy_realpath` 本身在 `cx/sys/fs.cx:95`，成功返回长度、失败 `-errno`；L0 正确，
问题在调用方吞错。

因此：
- 参与比较的根键曾回退词法路径 ⇒ `compare_set_has_fallback_key=1`，该构造实例
  `construction_identity=UNKNOWN`；某个新项自身 realpath 成功**不能**排除先前键不可靠。
- 不用 `identity_verified` 之类命名，只记可观察事实 `unit_path_resolved` / `unit_path_fallback`，
  两者之和 = 接纳项数；命名不声称依赖闭包或有效公开集一致。
- `plan_realpath_fallback_rows` 是**横切标志**（分散在各分区内部），不进任何互斥行等式，只记不等式
  `plan_realpath_fallback_rows <= plan_output_rows`。

### 14.3 计数单位：行、调用、对象分开

`tools/entry-build.cx:534-596` 的出口互斥，计数必须按各自单位分立：

| 字段 | 单位 | 含义 |
|---|---|---|
| `build_plan_calls` | 调用 | 成功返回的 `eb_plan` 次数（理想 33；`only_n` 时为 0） |
| `plan_output_rows` | 行 | plan 输出的非空行总数 |
| `plan_malformed_rows` | 行 | 无 TAB 行（`:546` 静默 `continue`，现无计数） |
| `plan_invalid_len_rows` | 行 | 长度 `<=0` 或 `>=576`（`:551` 静默 `continue`，现无计数） |
| `plan_root_hits` | 行 | 命中 33 根 realpath（`:561-563`） |
| `plan_closure_hits` | 行 | 命中既有闭包（`:565-567`、`:570`） |
| `build_units`（= `ncl`） | 对象 | 接纳的**新增闭包对象**，**不含 33 根** |
| `plan_fatal` | 次 | `plan-path` / `closure-cap` 中止 |

唯一可断言的等式，**且仅当** `plan_fatal=0` 且每行分类互斥完备：

```
plan_output_rows == plan_malformed_rows + plan_invalid_len_rows
                  + plan_root_hits + plan_closure_hits + build_units
```

不满足只报原值 + `closure_selfcheck=UNKNOWN`；不得为凑等式把畸形/越界行塞进"重复"。`closure-cap`
是**中止**不是接纳 ⇒ `closure_complete=0`、`build_units` 不完整，只能 UNKNOWN。`build_plan_calls`
与 `build_units` **不可**相加成等式（单位不同）。

### 14.4 本节 UNKNOWN 清单（不得升格）

| 项 | 状态 |
|---|---|
| 实际构造 argv / 执行身份 | **NOT-CAPTURED / UNKNOWN** |
| `construction_identity` | **UNKNOWN**（当前无手段升为已核） |
| `unit_path_fallback` 行的真实依赖是否同一 | **UNKNOWN** |
| `build_*` 全部字段的实测值 | **UNMEASURED**（本节是字段设计，非测量结果） |
| `D main` 是否覆盖全部 tracked 入口 | **否**（见 §4.2） |

未捕获即不得写"运行验证"、不得当 PASS。本节不建工具，不改 `entry-build`/`cxmod`/`spec`。

## 15. 本轮状态（csr 复审 4559d89ca6 后）

csr 判定项 3/4/5/6/8b 闭合，项 2/7/8a 主体闭合；本轮补：§5 cxm_norm 行号 113/143/163；§4 TSV 增 joint_id/double_count；§9 ROOT 增 v1r 列、AGGREGATE/category 统一 measurement_complete；§7 统一字段补 dependency_diff/std_edges；§1 声明 diff>0 非一致证明。仍未建工具/根清单、未开测；需 csr 对 4559 的硬残留再确认。

## 16. §4/§14 书面修订（omp-ds 代笔，cdx 临时移交写权）

修订范围仅 §4（新增 4.1–4.5，含 `origin` 列、三源差集、按行计数）与 §14（新增 14.1–14.4，含三域
分立、跨计划 realpath 去重与出错 UNKNOWN、行/调用/对象分立、UNKNOWN 清单）。**未建工具、未建
roots.tsv、未测量、未改 `cxmod`/`entry-build`/`spec`。**

- 数字全部绑定 **C = `fc628a0403a788c0b5cd18d31f2d1e6f6f70b9f3`**（`batches.tsv` sha256
  `504eadce…2222`），每次测量须按实际 C 重算，不沿用。
- 未连动 §6/§7/§9：§14.3 的 `build_*` 字段属**构造实例**报告，不进每根 `ROOT` 行；如需并入 §9
  输出格式，须 cdx 先裁定（已在回执中指出候选位置）。
- `recipe` 只表示源码声明；实际 argv 未捕获 ⇒ `recipe_argv_status=NOT-CAPTURED`、
  `construction_identity=UNKNOWN`。所有权与强制只读仍 UNKNOWN。

## 16.1 第二轮书面修订：§4/§5/§6/§7/§9/§14 一致性（omp-ds 代笔，go 授权临时移交）

基线核对：接手时文件 sha256 = `e9a413e2b8f7a6c6d77a2bb6d71f7c17190747cf0393c3ac659f7d4090948c39`（371 行，
`fab357760d` 之后）。已先核实际身份与他人改动并保留，未回退 go 回报的旧基线（`1734336427`/`7a5f…`）。
**未建工具、未建 roots.tsv、未测量、未改 `cxmod`/`entry-build`/`spec`；本轮结束归还 go 写权。**

| 位置 | 改动 | 起因 |
|---|---|---|
| §4 类别 `production` | 拆分 `participation` = `declared` / `observed`；`NOT-CAPTURED` 时只能 `declared`，不得写"已证明参与" | 原文「经配方或捕获 argv 证明」与同节「recipe 不是 argv」冲突；`--only`/`--link-only` 使参与成为实例性质 |
| §4.2 `S1 \ S2` | 改称 **build-instance 独立计划成员**（`scope=build-instance`、`joint_id` 留空），删除"联合根成员" | 与 §14.1 联合根定义冲突（每次 `eb_plan` 只传 1 个 src） |
| §4.2 `S3 \ S2` + 新增 §4.2a | 三分为 `no-main-by-design` / `prepared-template` / `absent-from-tree`（仅末者候选 STALE） | 批次源可无 main；PREPARED 的 entry 是模板；S2 不含软链与 manifest 外路径 |
| §5 环 | 建图（BFS，已见集只做边去重）与判环（三色 DFS / Tarjan SCC）分两阶段；报 `cycles` + `cycle_path`；未实现前 `cycles` UNMEASURED | 原文是 DFS 祖先栈语义却配 BFS 遍历，交叉边环漏检（夹具实测） |
| §5/§6 `pairs_dependency_diff` | 明确两侧 realpath 不同 ⇒ 不同身份，不构成 pair，分别入图，只记账 | cdx 裁定；原文"同 pair 两侧目标不同"自相矛盾 |
| §7 状态机 | 表格与表后句的"分别入队"vs"差异只停该边展开"合并为唯一规则：入队（`contexts_queued`）与生成 pair（仅同 realpath 组）分离 | 两处对同一事件给相反动作 |
| §7 峰值 | 删除单一 `pair_peak`，改 `context_queue_len_peak` / `pair_queue_len_peak` / `work_queue_len_peak` / `pair_verify_depth_peak`（各按元素单位），并定义 `NOT_APPLICABLE ≠ UNMEASURED ≠ 0` | 原字段把队列长度与栈深混成一个量，且名字暗示只数 pair |
| §9 每根 | 加 `scope`、`participation`、`contexts_queued`、`cycle_path` 与四个 peak（含 `_v1r`） | 与上述字段对齐 |
| §9 聚合 | 每类别一行；**事先固定**必需集合 `{production, tool, batch-positive}`；仅 production 完成只能写 `production_measurement_complete`；全必需完成才 `aggregate_measurement_complete`；`unclassified` 禁止入必需集；定义 `unknown_known`/`unknown_unexpected` 且二者之和等于 `unknown` | 原文只绑 production 却是无范围"测量完成"；两个 UNKNOWN 计数全文未定义 |
| §14.1 | 增 `scope` 取值域与 `joint_id` 唯一性（仅 `scope=joint` 非空） | 承接 `S1 \ S2` 归属 |

取代关系：§15 中「AGGREGATE/category 统一 measurement_complete」、§16 中「未连动 §6/§7/§9」两句，
均由本节取代（§9 现按类别分行并要求范围化状态名；§6/§7/§9 已连动）。

状态：全部字段与规则在算法定稿/实现前一律 **UNKNOWN 或 UNMEASURED / NOT_APPLICABLE**，不写 0、不写覆盖率；
本修订**不是实施授权**，不构成对任何测量的资格。§16 的"未连动 §6/§7/§9"一句由本节取代。
