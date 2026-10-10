# K5 首具名切口：exec/ 生成器与检查盘点 + 1 条可批草案

**状态（2026-10-10 ≈23:36 CST，机房主任批 K5-1：YES）**：盘点保留；**K5-1 已批**并推进接线/对拍负例；**不**整锅；**不**改 `exec/opt/check.sh` 验收；**不**旧全量空转；`version.h` 仍 0.0.39（冻结 bump 另裁）。tip 以 push 后 `main` 为准。roadmap：0.0.40 = K5（cc 检查脚本 / cdx 生成器）。

## 盘点（git 跟踪，exec/）

| 类 | 数量 |
|---|---|
| `.py` | 119 |
| `.sh` | 56 |
| 合计 | 175 |
| 名含 check 的 .py/.sh | 111 |
| `*-manifest.tsv` / `gen-manifest.tsv` | 132 |

### 按子目录（py / sh / check*）

| 子目录 | .py | .sh | check* |
|---|---:|---:|---:|
| (root) | 2 | 3 | 1 |
| build | 5 | 0 | 0 |
| c | 35 | 24 | 30 |
| enc | 21 | 9 | 29 |
| facts | 2 | 0 | 0 |
| ffi | 2 | 1 | 3 |
| lex | 10 | 3 | 5 |
| lower | 6 | 5 | 11 |
| opt | 0 | 1 | 1 |
| parse | 2 | 1 | 0 |
| parse2 | 12 | 2 | 13 |
| pipeline | 11 | 4 | 9 |
| pp | 9 | 3 | 7 |
| prune | 2 | 0 | 2 |

### 生成器入口（具名）

| 入口 | 角色 |
|---|---|
| `exec/build/gen.py` | 通用 STAGE→δ JSON 驱动（读 `exec/STAGE/gen-manifest.tsv` 或 `STAGE/SUB-manifest.tsv`） |
| `exec/assemble.py` / `exec/finite_rules.py` | 清单解释与有限规则 |
| `exec/build/{graph,parsebase,parse2base,procs}.py` | 图/基座 |
| `seed/gen.c`（`seed-gen`） | C99 构造器先例：特殊路径 prune / enc / enc/arm / lower / parse2；泛化路径接受 `#! base build/parsebase.py` 或 `build/graph.py` + `#! flags`/`#! start`/`#! graph G` |
| `exec/stamp.sh` | 生成物 fresh/stamp（共享重建闸） |
| `exec/c/tbl.py` / `exec/c/net.py` | δ JSON→表→网络（下游；net 已有 `seed/net.c` 对拍先例） |

### 检查入口（具名）

| 入口 | 角色 |
|---|---|
| `exec/check.sh` | 顶层 E0（toy 表 + exec.c） |
| `exec/{enc,lower,opt,c}/check.sh` | 阶段/族检查 |
| `exec/**/*check.py` / `*check.sh` | 约 111 个具名检查（含 asm C 检查由 sh 拉起） |
| `exec/ledger.sh` / `exec/lex/ledger.sh` | 账本 |

### `seed-gen` 相对各 STAGE 头（只读推断，未跑对拍）

| STAGE | 路径 | 备注 |
|---|---|---|
| prune / enc / lower / parse2 | 特殊分支 | 已有实现入口 |
| opt | 泛化候选 | `#! base parsebase` + `#! flags o2`；无 extra/domains |
| pp | 泛化候选 | `#! base graph.py` + `#! graph G` + flags |
| nativeabi | 泛化受限 | 有 `#! start NC.START`（泛化可读 start）；须实跑核 |
| lex | 未覆盖 | `#! graph Delta` / domains / extra → generic `other` |

**本盘点不冒称**任一 STAGE 已与 Python 逐字节等价；对拍须另授实跑。

## 一条已批切口（K5-1：YES，2026-10-10 ≈23:36）

### 名称：K5-1 `opt` δ 构造切到 `seed/gen.c`（保留 Python 参考）

**不做整锅**：只动 `opt` 这一 STAGE 的 δ JSON 构造路径；不删 `.py`；不改 `exec/opt/check.sh` 验收措辞/预算；不改其它 STAGE；不 bump；不冻结。

| 项 | 内容 |
|---|---|
| 动机 | opt 头最简且落在 `seed-gen` 已声明的泛化路径；体量小（单 flag `--o2`）；有 prune/enc/lower 同类先例 |
| 实现轮廓（批后） | (1) 用宿主 cc 编 `seed/gen.c`→`seed-gen`；(2) 同输入 `seed-gen opt OUT.json` 与 `python3 exec/build/gen.py opt OUT.json`（及带 `--o2`）`cmp`；(3) 故意改坏一行 TSV → 两侧均按名拒绝；(4) 在 stamp/fresh 或具名调用点为 **opt 仅** 增加「优先 seed-gen、失败不静默回退冒充」的接线；(5) `opt/check.sh` 仍可先走 Python gen，第二刀再切检查侧 |
| 验收 | 同输入逐字节相等（opt 与 opt `--o2`）；负例红；确定性双跑；identity 含构造器二进制/源、manifest/TSV 闭包、flags、宿主 cc；墙钟/RSS 分列不承诺「迁 C 更快」 |
| 明确不做 | 不删 `exec/build/gen.py` / assemble；不切 pp/lex/enc 全量；不改产品验收；不开旧全量 queue；不把矩阵历史绿当本刀证据 |
| 门闩依赖 | 本刀**不依赖**分层并发、WF2 填尾、K2b 真白名单、真实冻结首观察；依赖现有 bound/夹具与同输入对拍能力。缺项标「本切口不依赖」 |
| 归属建议 | cdx 实现生成器接线；cc 核 `opt/check.sh` 仍绿且不扩验收 |

### 备选（不本刀，仅登记）

- K5-1b：`pp` 同模式（flags 更多，次选）。
- K5-check-1：薄检查壳（如 `enc/shacheck.sh`+`.py`）改 C——属检查族，与生成器刀分开批。



## K5-1 进度（首刀接线，非冻结）

| 项 | 状态 |
|---|---|
| 宿主 cc → `seed-gen` | `tests/seedoptcheck.sh` 编 `seed/gen.c` |
| 同输入对拍 | `opt` 与 `opt --o2` 与 `exec/build/gen.py` 逐字节 SAME；C 双跑确定性 |
| 负例 | 改坏 `exec/opt/rounds-result.tsv` 一行：Python `expected section plus four columns`、C `section rule column count`，均 rc≠0 且不写出 JSON |
| 接线 | `exec/opt/gen-delta.sh`：默认优先 seed-gen；`SEED_GEN_BIN` 显式缺失 fail-closed；**不**静默回退 Python；`SEED_GEN=0` 仍走参考 |
| `opt/check.sh` | **未改**（仍 `gen.py`） |
| `version.h` | 仍 0.0.39 |
| 产品 buildcompiler | 既有 `SEED_GEN=1` 默认已走 seed-gen（含 opt）；本刀不扩其它 STAGE 切口 |

明确未做：不删 `.py`；不切 pp/lex/enc；不入 gate 全量；不 bump。

## 禁

未批就全量开干；整目录删 `.py`；旧全量空转；空等不交盘点。

## 进度更新（2026-10-11 01:1x，机房主任 01:10 卫生授权；只追加，上文保留为历史）

上文“K5-1 进度（首刀接线）”是 23:36 的快照；其中“负例改坏 exec/opt/rounds-result.tsv”的写法已被替换（原地改共享树，见下）。以下为现状。

### K5-1（opt）——授权范围内限定收口（机房主任 00:30），产品锁 a2454bf1
| 项 | 现状 |
|---|---|
| 提交链 | 7da8833b、ab4f75b8 → 418f7743、70ed1b2a、a60184a3、2ef26c16、e84949e2、08c79443、a2454bf1 |
| 夹具 | tests/seedoptcheck.sh（gate job seedgen-opt，stage 层）：私有 scratch（git archive 固定提交），负例只改 scratch 副本；导出 fail-closed（SEEDOPT_EVIDENCE）；NEG 具名带双子 rc |
| helper | exec/opt/gen-delta.sh：SEED_GEN 只认 0/1；最多一个 flag（--o2）；默认构建按编译器字节/版本/flags/seed 源码做 key，缓存校验 sha；不回退 Python |
| 读数 | run3（私有 wt，a2454bf1，timeout 58）：rc0，28 具名 SAME / 0 DIFF；o1、o2 的 c/py JSON sha 相等；NEG py=1 c=1；5 个导出故障分支有读数；检出 TSV 守恒。三方（cdx/cdx2/grk）限定通过 |
| 未证 / 不做 | 消费者 exec/opt/check.sh 未接（仍 gen.py）；中断路径、损坏缓存、RSS；故障分支只到函数级；JSON 本体与完整 stdout 未存；json.h 探针不走 hit_ok |
| 过程账 | 首刀原地改共享 TSV 无 trap（已修）；外层 timeout 75 违规一次（run1，已自报）；一次修片回执与提交内容不符（2ef26c16，已更正）；共享检出曾有身份不明写者改夹具（不归因） |

### K5-1b（pp）——限定收口（机房主任 01:10），产品锁 1c70d9e8
| 项 | 现状 |
|---|---|
| 提交链 | 12b95470 → 1c70d9e8 |
| helper | exec/pp/gen-delta.sh：SEED_GEN 只认 0/1；manifest 六个 flag 各至多一次；--osx 与 --win 分别检查成员、互斥 rc2；缓存键同 opt 版，不含 pp flag |
| 夹具 | tests/seedppcheck.sh 1\|2（gate job seedgen-pp-1 / seedgen-pp-2，事先 3+3 拆分）：无 flag、--osx、--win / --arm64、--osx --arm64、--win --arm64 六组 c/py 对拍加 C 双跑；NEG-tsv（exec/pp/autoinc-result.tsv 第 2 行删末列）；NEG-osxwin；helper 拒 7 种坏 flag；默认路线冷建/命中/json.h 重建 |
| 读数（1c70d9e8，私有 wt，timeout 58） | pp-1 rc0，30 SAME / 0 DIFF；pp-2 rc0，13 SAME / 0 DIFF；六组 c/py sha 两两相等；NEG 均 py=1 c=1；TSV 守恒。三方限定通过 |
| 原账（12b95470，单列） | pp-1 红：helper 未早拒紧邻 --osx --win（case 模式共用空格），已由 1c70d9e8 修；pp-2 在首红后照跑，属范围偏差，读数不并入 |
| 发现 | C 生成器对 --osx --win 的拒绝是“manifest let option is not yet covered”，不是互斥检查；生成器未改 |
| 未证 / 不做 | 三个消费者（exec/pp/run.sh、exec/pipeline/prepare.sh、exec/c/chain.sh）未接，不称迁移或提效；buildcompiler 的 shared 步骤本就走 C，不属本刀；--locations、--no-autoinc 与 os/arch 交叉未对拍；第二次 C 双跑摘要与成功路径子 rc 未导出；中断、损坏缓存、RSS 未证 |

### 共同
- version.h 仍 0.0.39，不 bump；验收措辞、check.sh 未改；共享检出禁跑，首跑只在私有 worktree。
- gate-infra：在 ab4f75b8 的干净 worktree 上实测为红（README-real-reviewed-snapshot），之后的 tip 未复跑；与本两刀无关，另列待指派。
- 下一刀（K5-1c）草案由 cdx 主笔，未授权实现。

### 更正（01:2x，cdx2 / grk 审 8cb27b24；只追加）
- 上文“共同”一节说 gate-infra 的红“与本两刀无关”——收窄为：**只知道 ab4f75b8 上实测为红，之后未复跑；它与 K5-1 / K5-1b 有无因果关系未证明，另列待核。**
- 上文 K5-1 表“读数”一行写“三方（cdx/cdx2/grk）限定通过”——收窄为：三方核过 run3 的读数，其中 grk 的原话是“读数核过、不称绿”；“授权范围内限定收口”是机房主任 00:30 的裁定，不是三方各自的结论。
- K5-1b 表的“三方限定通过”：以机房主任 01:10 的收口裁定为准。

### K5-1c（prepare.sh 的 pp 调用接 gen-delta）——限定收口（机房主任 01:43），产品锁 96272cd2
生产只改 exec/pipeline/prepare.sh:17（私有 SEED_GEN_DIR="$OUT/.seed-gen-cache"，三项代价已明批）；夹具 tests/seedppconsumercheck.sh（gate seedgen-ppconsumer-1/-2）。受控片 1 rc0 22/0、片 2 rc0 18/0；唯一真实冷路线 lnx/x86_64（NETWORK=1）models rc0、外层 42.88 s，发布集 24 项不含私有缓存，e2 与 Python 同输入逐字节相同。缺项按 schema 顺延（Python 参考真实 rc/argv、冷编单段耗时、运行时 SEED_GEN_CC 与编译器身份、冷后两处源码 sha）；编译器键只与调用名解析为 /usr/bin/cc 相容。不称六目标端到端、不称提速；第二、第三消费者未接；gen-delta 头注释过时（延后）。回执：/tmp/cc40-prep/k5-1c/receipt-controlled-and-cold.md（仓外）。


### K5-1d（chain.sh 的 pp 调用接 gen-delta）——受控绿已合 main（机房主任 02:4x），产品 tip 1bd685b0
生产只改 `exec/c/chain.sh`（私有 `SEED_GEN_DIR="$T/.seed-gen-cache"` + `e2-gen.{out,err}`）与 `exec/pp/gen-delta.sh` **头注释**；夹具 `tests/seedppchaincheck.sh`（gate `seedgen-ppchain`）。tip 复跑 rc0 **20/0**、外层 **50.784 s**；e2=ee8ac744… 三路同；夹具 sha 28a0c4c2 与 tip 全等（闭合 cdx 所阻脏树 overlay 身份缺口）。version.h 仍 0.0.39。不称五片全绿/提速。回执：`research/c40-k5-1d-controlled-receipt.md`。

#### K5-1d 当前口径（机房主任 02:55 限定收口；只追加，上段为历史）
上段“tip 复跑 rc0 20/0、外层 50.784 s”是 grk 那次复跑的读数（回执未引 02:47 授权），按 02:55 裁定作**只读旁证、不并入**。限定收口以 cc 依 02:47 授权的干净复跑为准：锁 1bd685b0，夹具身份 28a0c4c2 跑前落盘并闭合，gate rc0、20 SAME / 0 DIFF、外层 49.583 s（两次同机并发重叠，墙钟都不代表单独耗时）。schema INCOMPLETE 顺延（共享检出四文件前后 sha 未采；残留 scratch 来源 UNKNOWN；参考子 rc 未存；证据复制 `|| true`）；夹具无 INCOMPLETE 行 ≠ schema 齐。不称 exec-chain 五片全绿、不称净提速。回执：/tmp/cc40-prep/k5-1d/receipt-rerun-0247.md（仓外）。

