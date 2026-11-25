# 产品输入闭包键 (`PRODUCT_INPUT_KEY`) 与回执复用合同规范

维护窗口: `agy-h` | 日期: 2026-11-25 | 上游依据: `fast-iteration.md` (`G1/G2/B2/E`)、`seed/ident.c` (`source-packages-v3`)、`tests/batch-product-inputs.tsv`
状态: 预跑期间纯设计定稿，不触编译、不提交 `main`。

---

## 1. 设计目标与反模式纠偏

现有整链(`tools/candidate-chain.cx`, `tools/release-chain.cx`)与预跑(`tools/prerun-inputs.cx`)以「版本提交号 (`git rev`)」为键，导致以下反模式(`0.1.0` 实证):
- `0.1.0` 相对 `0.0.107` 在产品域(`src cx exec kernel unisa ext`)差异为零，却重走 27 分钟整链与 80+ 分钟全量预跑；
- 修改 `research/` 或 `plans/` 文档页改变了仓库提交号，连带使已有整链/候选身份失效并迫使重合并。

纠偏原则: 把「造字节与验证字节（内容身份驱动）」和「打版本号与发布标签（版本元数据驱动）」彻底解耦。一切产品候选构造与同身份回执复用，只认「产品输入闭包键 (`PRODUCT_INPUT_KEY`)」与「单批完整闭包键 (`RECEIPT_CLOSURE_KEY`)」，不认 git 提交号与三段版本号字符串。

---

## 2. 产品输入闭包键 (`PIK1`) 精确定义

### 2.1 `PIK1` 计算规范 (与 `seed/ident.c` `walk`/`collect` 对齐)

`PRODUCT_INPUT_KEY` (`PIK1`, 64 位小写 hex SHA-256) 由四段规范化行哈希生成:

```tsv
PIK1
sources_sid	<64hex>
toolchain_ident	<64hex>
asm_side	<64hex>
env_contract	<64hex>
```

各字段的物理来源与源码对应关系如下:

(1) `sources_sid`（核心源闭包，直接调用 `seed/ident` 的 `source-packages-v3`，见 `seed/ident.c:278-313`）:
- 物理根目录集合:
  - 12 个逻辑包与目录(`seed/ident.c:139, 152-165, 287-289`): `cx/sys`、`cx/mem`、`cx/text`、`cx/core`、`cx/driver`、`cx/runtime`、`cx/compiler`、`src`、`exec`、`seed`、`weights`、`include`，外加 `cx` 全目录、`kernel`、`iterate/kernel/typekw.tsv`，以及归属 `ext/memx`、`ext/textx`、`ext/sysx`、`ext/crypto`、`ext/flownet`、`flownet` 下的有效源输入；
  - 13 个构建支持工具(`seed/ident.c:279` `support[]`): `tools/{bound, bound.c, cxbin, safedel.cx, lib.sh, build_ref.sh, export_ref.sh, refshim.h, reffoot.h, sourceflat.cx, ape-pack.cx, release-ape.cx, cxlib/releaseversion.cx}`；
  - 3 个额外支撑(`seed/ident.c:280` `extra_support[]`): `build/core-closure.tsv`、`cxroot.tsv`、`ext/textx/deflate.cx`；
  - 软链规则(`Rule L.1`, `seed/ident.c:219-223`): `cx/` 或 `src/` 下指向七目录外的文件只记录 `aliases` 拓扑路径对，不拉取外部目标文件字节；
  - 显式过滤(`seed/ident.c:41-64, 201-205`): 仅收 `.py/.c/.cx/.h/.inc/.tsv/.json/.sh`（及 `exec/c/asm/` 下 `.S` 汇编），排除 `cx/check_{memory,owned}.cx`、`exec/` 下以 `check.{sh,py,cx}` 结尾的检查脚本、`probes/` 下 `.c`/`.h`、`README.md`、`.DS_Store`、`seed/` 非白名单文件（`seed/` 只收 `tbl.c, net.c, json.h, gen.c, facts.h, ident.c, blob.c, sha256.h, pack.c, compilerpack.c, ape.c`）。

(2) `toolchain_ident`（构造器与构建链配方闭包，补全 `seed/ident.c` 未涵盖的构造成因）:
- 当前作为起链构造器的已安装产品身份(`split2` `product_digest`，格式同 `seed/ident.c:365-370` 与 `tools/cxlib/productpkg.cx`，即 `unisacc.com` 与 `unisacc.com.dev` 的内容哈希与长度)；
- 影响产品字节生成的链构造工具链源码闭包(SHA-256 排序逐行拼接):
  `tools/candidate-chain.cx`、`tools/chain-step.cx`、`tools/release-build.cx`、`tools/build.cx`、`tools/kernel-build.cx`、`c4e88da2b21888b:tools/product-build.cx`、`tools/source-job-wave.cx`、`tools/pc-wave.cx`、`tools/selfhost.cx`、`tools/cxlib/nninputs.cx`、`tools/cxlib/netpack.cx`、`tools/cxlib/kernelsha.cx`、`tools/cxlib/productpkg.cx`、`tools/cxlib/netsel.cx`、及其全部传递 `#pragma unisa use` 依赖；
- 宿主 C 编译器与汇编/链接工具身份（如 `cc --version` 与目标 ABI 标志指纹），防止同源不同宿主编译器产出混用。

(3) `asm_side` 与 `env_contract`:
- `asm_side`: 若产品构造额外消费 `exec/c/asm/` 下 `.S` 的字节（在 `source-packages-v3` 中 `collect("exec")` 同样对 `.S` 经 `assembly(logical)` 收录，见 `seed/ident.c:198, 249`），显式核实无漏项并记录；
- `env_contract`: `tools/cxlib/host-env` 冻结构造环境白名单（不含版本号与临时路径）。

(4) 刻意排除项（版本贴标字段不得进 `PIK1`）:
- 不包含 `release/candidate.tsv` 版本串、`build/version-seq.tsv`、`build/freeze.tsv` 截止时间与 `HEAD` git commit SHA；
- 前置自证(`fast-iteration.md F3`): 在启用 `PIK1` 分级前，必须对 `0.0.107` 与 `0.1.0` 已公开 APE 解包逐段对比哈希；若发现任何未被 `PIK1` 收录却影响了 APE 除版本槽外字节的文件，立即补充进 `toolchain_ident` 或 `seed/ident.c:extra_support[]` 后方可启用。

---

## 3. 文件变更影响分类矩阵 (换键 vs 不换键)

| 路径/类别 | 具体涵盖范围 | 是否换 `PIK1` | 处理动作（产品树 / 预跑 / 标签） |
|---|---|---|---|
| 产品源与运行库 | `cx/` (除 `check_{memory,owned}.cx`), `src/`, `ext/` (被闭包收录或产品 `use` 者) | 是 (换键) | 按 `PIK1` 进入产品补丁级完整整链与严格预跑 |
| 事实表、网、权重、汇编核 | `exec/` (除后缀 `check.{sh,py,cx}` 与 `probes/`), `kernel/`, `weights/`, `include/`, `iterate/kernel/typekw.tsv`, `cxroot.tsv` | 是 (换键) | 按 `PIK1` 进入产品构造与严格验证 |
| 引导种子与支撑适配器 | `seed/` 11 白名单文件, `seed/ident.c` 13 个 `support[]` + 3 个 `extra_support[]` | 是 (换键) | 同产品变更 |
| 构造链工具配方 | `tools/release-build.cx`, `chain-step.cx`, `candidate-chain.cx`, `kernel-build.cx`, `product-build.cx` 等及传递 `use` | 是 (`toolchain_ident` 换键) | 涉及产品装配算法变动，须重建候选并核身份 |
| 派生表 (`build/`) | `build/core-closure.tsv` (`extra_support`) | 是 (换键) | 其变意味核心源闭包变，必换 `PIK1` |
| 其余派生接口与索引表 | `build/{iface-hash,layer-api,std-surface,table-index,tlib-packages}.tsv` | 随产品改而换；若仅工具改则不换 `PIK1` | 产品未变时仅跑对应表检查，不重造 APE |
| 版本计划、序表、冻结表 | `plans/`, `build/version-seq.tsv`, `build/freeze.tsv`, `release/candidate.tsv`, `release/rollback.txt` | 否 (不换 `PIK1`) | 标签级发布只走版本槽注入与读回，不重跑产品整链（待 `F3` 证伪通过） |
| 研究文档与规范 | `research/`, `prd.md`, `prd_mempalace.md`, `prd_cx_spec.md`, `AGENTS.md`, `README.md`, `release/` 下 `notes-` 说明页 | 否 (不换 `PIK1`) | 零触发重链，只需 `doc-links-gen`、`no-home-path` 等轻量门 |
| 独立工具与测试登记 | `tools/` (非 13 support 且非构建链配方), `tools/tape/`, `tests/` (`batches.tsv`, `coverage.tsv`, `manifests/`, `check-` 前缀用例) | 否 (不换 `PIK1`) | 只触发 `affected` 选中的对应工具/测试批与 `tools-compile`，不触发产品 APE 重造 |
| 独立外置 `nn` 实验目录 | `<repo>/nn/`, `build/nn/`, `$TMPDIR/nn/` | 否 (不换 `PIK1`) | 仅影响带 `--nn-dir` 的本地开发态缓存键，正规链用 `--no-external-nn` 严格屏蔽 |

---

## 4. 回执复用的五个合法条件（可执行合同）

本节把 `fast-iteration.md G2` 的五个条件落成不可绕过的合同约束。只要五个条件中有任意一条未满足，复用即被机械拒绝并回退真实执行。

### 条件 1: 完整单批输入闭包键 (`RECEIPT_CLOSURE_KEY` / `RK1`)
每一笔可复用的预跑回执必须绑定如下 `RK1` 五元组字段（拼接计算 64 位小写 hex SHA-256），不得以提交号、批名或版本号代替:
1. `batch_src_closure_sha`: 该批 `source`/`entry`/`args` 根文件及其全部传递 `#pragma unisa use` / `#include "..."` / 字面量文件（由 `affected` + `runner` 闭包解析确定）的有序路径与内容 SHA-256；
2. `product_closure_sha`:
   - 当 `headers == tree` 且不使用 `{candidate}`/`{candidate-root}` 时记 `none`；
   - 当 `headers == product` 或用到 `{candidate}` 时，若批在 `tests/batch-product-inputs.tsv` 有精确 `id` 切片声明且与运行期实测拦截对账吻合，可记「所声明前缀闭包哈希 + 运行期目标架构识别」；否则保守强制绑定整候选 `ape_sha + PIK1`；
3. `runner_sha`: 运行器脚本与支撑闭包哈希(`tests/run.cx`, `tests/run-batch.cx`, `tools/prerun-supervise.cx`, `tools/bound`, `tools/cxlib/runnerprep.cx` 等)；
4. `materials_sha`: `batches.tsv` `materials` 字段所指预构材料的实内容 `material_identity` 哈希（无材料记 `none`）；
5. `env_budget_sha`: 该批在 `batches.tsv` 的 `budget_seconds`、`platform`、`tier`、`headers`、`args`、并发隔离属性（独占 vs 隔离核数）以及相关环境契约哈希。

### 条件 2: 命中时现场重算身份，禁止信任旧收据断言
- 查找复用库(`~/.cache/unisacc/receipts/<batch_id>/<rk1_sha>.tsv`)时，调用方必须在当前工作树上现场重新执行条件 1 的五个输入计算得出 `current_rk1`，绝不允许直接读取旧收据或旧 manifest 里存储的期望键值去跳步。
- 命中后还须校验两重一致性:
  1. 收据文件自身的完整性签名（`receipt_self_sha` 核对，防写入截断或磁盘损坏）；
  2. 若收据所引用的 `origin_log_path` 可读，验证其 `RAW BATCH_RUN` 行的 `id`、`candidate`/`scope` 与 `current_rk1` 一致。
- 任一重计算失败、路径不可读或 1 字节不符 -> 判 `MISS`，立即执行真实批跑。

### 条件 3: 报告单列 `REUSED`，与 `PASS` 严格分离计数
- 严格预跑总表(`strict.tsv`)、`prerun-supervise` 汇总行与发布证据(`decision.txt`、`release/notes-` 说明页)必须遵守四栏独立计数:
  `executed_pass` / `reused` / `failed` / `unknown`（另有不合规拒绝 `invalid`）。
- 禁止把复用的批写入 `status=PASS`；必须写成 `status=REUSED`。
- 每一条 `status=REUSED` 行后面必须紧跟完整溯源元数据行:
  `REUSED_ORIGIN	id=<batch>	rk1=<64hex>	origin_run=<run_id>	origin_utc=<ISO8601>	origin_log=<path>	origin_wall_ms=<N>	origin_load1=<X>	assertions=<A>`
- 上游发布门(`tools/release-exec.cx`, `tools/seal-precheck.cx`)必须拒绝任何「缺 `REUSED_ORIGIN`」「`REUSED` 行混计入 `executed_pass`」或「无原执行日志真实指向」的证据包。

### 条件 4: 闭包未声明完整或非确定性的批永不复用 (`NEVER_REUSE`)
满足以下任一规则的批在登记表或 `tests/batch-never-reuse.tsv` 标记 `NEVER_REUSE`，复用门见其必拒（强制真跑）:
1. 未在 `tests/batch-product-inputs.tsv` 中有已审计完整输入声明的 `product` 批（目前 `narrowed=0` 或未接运行器实读对账审计者）；
2. 历史上次结果不是 `PASS` 的批（`FAIL`、`UNKNOWN`、`SKIP` 绝不允许被复用）；
3. 显式时间/版本槽位/外部状态敏感批:
   - 读取 `release/candidate.tsv` 当前版本号做判断的到期门（如 `check-env-semantics`、`check-verseq-gen`、`check-releaseseq`）；
   - 外部读回/网络/CI 状态/时间窗口门（`check-release-post`、`check-seal-precheck`）；
   - 资源竞争/超时自杀/锁机制测试自身（如 `check-prerun-watch`、`check-devsvc-core`、`check-precommit-list-cap` 超限测试、含进程组清理门）。

### 条件 5: 版内随机抽样对拍 (`>= 10%`) 与周更全量熔断
- 每车随机抽样实跑: 只要本次存在任何 `REUSED` 批，预跑调度器必须基于不可预测但可审计的确定性种子 `SHA256(target_version + PIK1 + base_commit)` 从候选 `REUSED` 集合中抽取至少 `10%`（若不足 `100` 个复用批则至少抽 `10` 个，总数 `< 10` 则全跑）进行真实执行（忽略复用缓存、冷态实跑）:
  - 对拍一致: 记录 `REUSE_AUDIT_SAMPLE	id=<batch>	match=1	sample_rc=0	wall_ms=<M>`；
  - 对拍出现任何 1 例不一致（实跑 `rc`/`status`/`marker`/`assertions` 与原 `REUSED_ORIGIN` 不合）: 立即触发全局熔断 (`REUSE_POISON_DETECTED`)，当场丢弃本次全部 `REUSED` 状态、把所有原拟复用批转入全量真跑队列，并将失配批记为闭包漏项阻断事件！
- 周更全量清零周期 (`7 天`): 复用门检查上次「零 `REUSED` 全量实跑通行章 (`ZERO_REUSE_FULL_PASS`)」时间戳；距今超过 `7 x 86400 = 604800` 秒时，自动禁用全部 `REUSED`，强制运行一轮完整无复用预跑后才允许重新签发下一周的复用资格。

---

## 5. 可检查实现规格与先红后绿验收清单

### 5.1 拟定实现载体与存储布局
- 键计算与验证模块: `tools/cxlib/prodkey.cx` + `tools/product-input-key.cx`（产出 `PIK1`、逐批 `RK1`、并比对工作区增量对 `PIK1` 的影响判定）；
- 合同验证工具: `tools/check-product-input-key.cx`（注册批 `product-input-key`, `commit` 级 `< 25s`）。

### 5.2 八条先红后绿验收测试用例（写入 `check-product-input-key.cx`）

| 序号 | 测试用例 | 操作注入 | 预期结果 (先红后绿门槛) |
|---|---|---|---|
| 1 | 改产品源码换键 | 复制仓到 `$TMPDIR`, 修改 `cx/core/alu.cx` 一行注释/代码 | `PIK1` 前后 64hex 必须改变（不变则红） |
| 2 | 改文档/研究页不换键 | 修改 `research/` 与 `plans/` 文件各一处 | `PIK1` 前后 64hex 严格相等（改变则红） |
| 3 | 改版本槽元数据不换键 | 按规范只修改 `build/version-seq.tsv` 与 `release/` 说明页 | `PIK1` 前后 64hex 严格相等（改变则红） |
| 4 | 改构造链配方换键 | 修改 `tools/release-build.cx` 一字节 | `toolchain_ident` 与 `PIK1` 必须改变 |
| 5 | 批闭包缺失/未声明拒复用 | 对标为 `NEVER_REUSE` 或无 `batch-product-inputs` 声明的批查复用 | 强制返回 `REUSE_DENIED: unclosed-or-never-reuse`（允许复用则红） |
| 6 | 篡改收据字节或换批源码拒复用 | 修改已存收据内一字节 `assertions` 或改测试源码一字节再查命中 | 现场重算 `current_rk1` 不合或签名失效，强制返回 `MISS` |
| 7 | `REUSED` 计数隔离验证 | 构造含 `1 executed_pass + 2 reused` 汇总喂入发布门校验 | `seal-precheck` 核出 `executed_pass=1 reused=2`; 若误把 `reused` 加进 `PASS` 或缺失 `origin_log` 则必须拒绝 |
| 8 | 抽样对拍失配熔断验证 | 模拟抽样 10% 中 1 批实跑 `FAIL` vs 原 `REUSED_ORIGIN=PASS` | 触发 `REUSE_POISON_DETECTED`, 全局撤销所有 `REUSED` 判级并强制全跑 |
