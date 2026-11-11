# unisacc 路线图：0.0.43 收口 + 流/网范式重构（综合稿，2026-10-22）

依据：本工作流的 PLAN STATUS、GATE HEALTH、RELEASE CHAIN 三份审计，三份设计（tdesc / state / cell），两位评审，以及综合时对仓库的只读复核。

**仓库现状（实际跑过）**
- 本地 HEAD 是 `7ca340d5`，比 origin/main（`7a2b87b2`）多 1 个提交，**还没推送**。
- 7ca340d5 是 `tests: tdescprobe.cx`，也就是 TDESC 的第 0 刀（窄版）：
  - 枚举 {P, [3], (int)} 深度 1..3、基类型 int，共 23 个合法派生 × 8 种上下文 = 106 个 sizeof 探针；
  - 基线：产品 86 OK / 18 REFUSE / 2 WRONG，经典参考 85 / 8 / 13；
  - 新增 gate 作业：tdescprobe-oracle、tdescprobe-classic、com-tdescprobe-1..3。
- 这笔提交只改了 tests/、research/ 下的文件。按 `tests/freezecheck.py:22-27` 的 `product()` 判断，**推断**它不碰产品闭包，但没有实测。
- 产品基线是在**过期的根产品**（sha256 前缀 10b3c756，构建于 769b7849）上录的，重建后要复核。

**标记约定**
- 「实测」：引自审计里实际跑过的结果。
- 「推断」：只读推断。
- 「设计」：未验证的设计估计。

---

## (A) 0.0.43 收口范围：要关的项是有限的

**收口原则**
- 0.0.43 只关下面 M0–M9 这 10 项。
- 其余每一项要么进 DEFERRED 表，要么进发布说明的「已知缺陷」。
- 重构（D 节）的所有改字节刀都**不进 0.0.43**。零字节刀可以在等待队列时并行做，但不作为收口条件。

### A.1 必关清单（含验收证据）

| # | 项 | 验收证据（缺一不结） | 依据 |
|---|---|---|---|
| M0 | **主人一次性裁定，把范围定死**：P1 boundary-r1 推迟；H1 是否接受 16 核宿主；freezecheck 怎么处置；A·P·A 拒绝要不要做（见 M3）；采纳 A.2 推迟表 | `release/ruling-requests.tsv` 中该行为 APPLIED，并附 ref；`plans/v0.0.43.md` 写入一行「收口范围裁定」 | `prd.md:71`（P1 只授了具名范围）；`plans/v0.0.43.md:64`（H1 写的是 8 核）；`plans/v0.0.43.md:5,60` |
| M1 | **freezecheck 转绿** | `sh ./unisacc.com -run tests/freezecheck.cx` rc=0（门禁 job freezecheck；`freezecheck-selftest` 证新口径） | **已落（R10 选 (a)，2026-10-23）**：冻结窗起点改为 `release/freeze.tsv` 里显式的冻结提交（无本版本行 = 不在冻结窗，rc 0；该行在发布准备时加），并接受 `fix(<scope>):`；waiver 不变。实测 rc=0。原状：rc=1，23 笔违规；`freezecheck.py:44` 只认字面前缀 `fix:`，版本提交 dbbc4fe7 把整个开发期都算成了冻结窗。`tests/freezecheck.py` 的 main() 仍是旧口径，但没有任何调用方（只有 `product()` 被 commitgate/pre-commit 导入） |
| M2 | **台账类红一次清掉**（只改 tests/、文档，不碰闭包） | 以下套件单跑全部 rc=0：fresh-order-0..8、revivedscan、subtract-safety | 见 B 节 B2 |
| M3 | **声明符缺陷类有界**：tdescprobe 进闸，产品基线在重建后的产品上复核 | com-tdescprobe-1..3 rc=0（ratchet 不变）；tdescprobe-oracle rc=0 | 7ca340d5。理由：产品**仍有静默错码**（`int (*x[3])[3]; sizeof(*x[0])` 产品给 4，cc 给 12，实测；e05 运行时 SIGSEGV，实测）。见下方「M3 的裁定点」 |
| M4 | **重建根产品**，在宿主修正后的环境里重跑 CONTRACT 层 | `ident . check unisacc.com` rc=0（verified）；CONTRACT 层 160 个套件中，除 B1 列明的宿主项外全绿 | 当前 ident rc=1（实测）；137/160 的绿是在旧产品上得到的 |
| M5 | **晚绑定按类别出一份在新候选上核过的状态表**：普通 / data / typed / callback / 运行桥，运行桥只写明「保持旧路」 | `research/c43-latebind-status.md`，每行带 run 证据；未核的行写 UNVERIFIED，不补绿 | `plans/v0.0.43.md:12`；`spec.md:842` LB-4 |
| M6 | **第 4 项基线逐行复核并更新** | 基线表更新：P4a 探针移进 tests/c；P4e 登记；P4c 维持「待派」 | `plans/v0.0.43.md:14`；P4a 与 P4e 已实测 pass |
| M7 | **本轮有效闸** | 本地 `tests/release.sh --com` rc=0（或按 0.0.42 先例：出口表 NEEDS_RULING=0、product_red=0）**并且** release-check 对结项提交为 success | `release/RELEASE-PIPELINE.md:113`（§10） |
| M8 | **发布说明如实写** | 说明里：win 接受不计绿；已知缺陷列出 tdescprobe 的 WRONG/REFUSE 集合、经典参考 13 WRONG、hex-A′、extent ≥2^31 | `plans/v0.0.43.md:31`；`prd.md:58` |
| M9 | **授权链**：封存、Draft、Latest、签名、公开 | 见 C 节，每段有一条具名授权 | `AGENTS.md:38,70`；`plans/v0.0.43.md:5,60` |

#### M1 的处置选项（由主人裁）

- **(a) 推荐：改口径，不改历史。**
  - 冻结窗的起点改成显式的「冻结提交」，写在 plan 里，而不是 version.h 的 bump 提交；
  - `freezecheck.py` 同时接受 `fix(<scope>):` 前缀。
  - 只改 tests/ 下的检查，**不碰闭包**。
  - 理由：现在的规则会让「开发期」永久处于冻结红，这是流程模型和检查规则不一致（GATE HEALTH B 节）。
- **(b) 逐条登记 23 笔到 `release/freeze-waivers.tsv`。**
  - 问题是 waiver 的语义是「红修」，套在 15 笔 `feat:` 上是名不副实（RELEASE CHAIN S0）。
- **(c) 重做一次版本 bump 刀。**
  - 代价最大，不推荐。

#### M3 的裁定点

产品现有 2 个 WRONG，都是 `(*x[N])[M]` 这一形状。语料里没有这个形状（grep 结果为 0，有 typedef 漏网的可能）。有两个选项：

- **(i) 推荐：在今天的声明符代码里加一条具名拒绝。**
  - 形式：在声明时对 `(*x[N])[M]` 整类报 not covered。
  - 这和现有的「pointer to pointer to array」整类拒绝（tdescprobe 里那 8 条 REFUSE）同构。
  - 效果：2 WRONG 变成 REFUSE，e05 的 SIGSEGV 也一并封住。
  - 这是一个 `fix:` 刀，代价是 e3 reseal、facts/abi.manifest 刷新、改 ratchet 记录。
  - 它**不是路径补丁**：它把静默错码变成诚实拒绝。真正的修复留给 TDESC（D 节）。
- **(ii) 不改产品。**
  - 2 WRONG 写进发布说明的已知缺陷，0.0.43 带着已知的静默错码发布。
- 不确定性：(i) 的拒绝能否在所有上下文（全局、局部、参数、成员）都在同一位置触发，没有实测。如果一刀做不完，就回退到 (ii)。

### A.2 推迟到 0.0.44 的项（每项说明为什么不违反书面承诺）

| 项 | 推迟理由（原文依据） |
|---|---|
| P1 lib-provider-pp-boundary-r1 | 书面写的是授权，不是必交：「只获具名范围，不能扩成全libc迁移或发布绿」（`prd.md:71`）。另外 labs 原型实测「P/M 不变条件不成立」（`research/c43-boundary-catalogue-decision-r1.md:9`）。它在优先表里排第一，所以推迟必须由 M0 写一句裁定 |
| L-4 win `.hostcall`/`.hostaddr` 库模式合同 | 承诺只是「一致性证据未齐前不得记产品绿」（`prd.md:58`，`spec.md:831`），由 M8 遵守 |
| 运行桥迁移 | 「运行桥保持旧路」（`spec.md:842` LB-4） |
| 2c2 的三笔债（sourcefacts 解耦、seedparse2 UNVERIFIED、stack×3 UNVERIFIED） | 2c2 默认不开，只在 `--librarytyped` 构建里有（e020920c）；M5 表里如实写 UNVERIFIED |
| 十六进制 A′（P4c） | 「已有缺陷，非本版回归；待派」，且「排在 W/S 之后，除非阻塞发布」。实测：它是拒绝，不是错码，所以不阻塞 |
| §2b 指针↔算术；extent ≥2^31 | 原文分别是「仍待裁」和「建议维持登记 not covered」 |
| 经典参考 sizeof 缺口，以及经典 13 WRONG | 「待定界（登记，未修）」（`plans/v0.0.43.md:55`）；产品与 gcc 一致；由 tdescprobe-classic 做 ratchet |
| plan-proposal 第 3–6 步 | 「后续（未授，列出待定）」 |
| R3 TDESC 的 S1 及之后、state/cell 各刀、stagechain 入闸 | 原文是「只是设计」；改字节的刀必然要 reseal e3，和 C6「不 seal」冲突 |
| 超时债余项（selfcheck/compilercheck 冷缓存、stagechain 冷跑 >60 s） | 原文是「代裁可动手、不放宽限时」，没有截止日期。但如果某个闸作业因此变红，就会通过 M7 挡住发布 |
| prd §3.0d ①–⑤（边级冻结原件等） | 进度登记，「有封≠已冻」 |
| `.cx` 隐式前奏（cx.h prelude） | 实验实测：前奏**不是闭包中性的**，60 个 .cx 里有 3 个镜像会变（`research/c43-cx-paradigm-experiments.md` E1）。草案状态，没有书面承诺 |

---

## (B) 发布前的红闸：宿主问题与真实缺陷分开

### B1 宿主或准备工作问题（不是代码缺陷，用宿主配方消掉）

| 套件 | 处置 |
|---|---|
| c99precheck | 设 `LC_ALL=C`。bash 的 glob 和 Python 的 sorted 排序规则不同，实测 `LC_ALL=C` 下 rc=0。**但它也暴露了一个潜在缺陷**：difftest_o 的分片划分依赖宿主 locale。建议在 gate.sh 里钉 `LC_ALL=C`（只改 tests/） |
| draftguard | 缺 PyYAML。用 `/usr/bin/python3` 实测 rc=0，或者给 venv 装 pyyaml |
| ape-version | 缺 `llvm-readobj`，装 llvm |
| latebind-p6 | 缺 `ffi.h`，装 libffi-dev |
| reviewed-trees | 宿主 umask 0002，检出权限是 664。实测在 umask 022 下的 `git archive` 树上 rc=0。可以重检出，或者在 umask 022 下跑 |
| middle-flow-ledger | `MODEL_COM` 要指向带 `compiler.pkg` 的完整构建目录，比如 `out/model-com/` |
| resourceflow | `MODEL_COM` 的默认值是相对路径（`gate.sh:856`），被检查脚本 cd 之后就找不到了。实测改传 `$PWD/unisacc.com` 后 13/13 PASS。**这是 gate 默认值的潜在缺陷**，建议在 gate.sh 里把它绝对化（只改 tests/） |
| tools-2/3「clone failed」 | `tests/tools.sh:58-73` 并发 clone 没有加锁。先单独预取 corpus |
| tools-2、tapebin-roundtrip-structure-1/2 超时 | 新宿主单线程 Python 慢，实测；系统 Python 3.10 也超时。属于超时债：先串行、在低负载下复测；仍超时就拆片（不放宽限时） |
| bound rc=77 | 设计如此（Darwin 专用），不算失败 |
| 发布前置：缺 `corpus/c-testsuite`、oras、zip、ld.lld；没有 csmith | `FETCH=1 tests/corpus.sh`；安装 oras、zip、lld。csmithdiff×41 预计记为 TOOL_MISSING，走出口表裁定 |

**宿主配方（推断，一次写成脚本）**：`umask 022; export LC_ALL=C; export PATH=/usr/bin:$PATH`（或 venv 里装 pyyaml）；`apt install llvm libffi-dev zip lld`；oras 静态二进制；`MODEL_COM=$PWD/out/model-com/unisacc.com`。

### B2 真实问题（台账或流程没跟上，或产物过期；都不是编译器行为错误）

| 套件 | 修法 | 归入 |
|---|---|---|
| identity-product-negative | `make com` 重建根产品（M4） | M4 |
| fresh-order-0/1（推断 2..7 同样红） | 重新记录 `tests/freshorder.tsv`：parse2 标签数 4372→4378，`--locations` 下 4407→4413。漂移来自 10-20 到 10-21 的 parse2 修复，不是今天引入的（实测） | M2 |
| revivedscan | 从 `tests/pyfront.knownfail:18` 和 `tests/product-refusals.knownfail:6` 删掉 declsnapshot。修复已生效，三方 md5 一致（实测） | M2 |
| subtract-safety | 修 63 处悬空引用：`tests/gatedeps.json:41164/41165`、`tests/refresh_gatedeps.py:211` 改指 archive；research c43-flow-reuse-step3..6；`tests/exittablecheck.py`。会话开始前就是红的 | M2 |
| freezecheck | 见 M1 | M1 |
| spec TP-9 的下划线整数句子 | 只改文档，顺手关掉 | M2 |

**真实的编译器缺陷**在 CONTRACT 层没有表现为红闸，但已经被 tdescprobe 测出来了：2 个 WRONG，加 e05 的 SIGSEGV（e05 不在 7ca340d5 的 106 个探针里，它是运行时用法，不是 sizeof）。处置见 M3。

**未验证项（如实列出）**：fresh-order-2..7 没有单独跑，红是推断的；3 个超时套件没有绿证据；所有绿结果都是在旧产品上得到的，M4 之后要重跑。

---

## (C) 需要主人授权的发布步骤（按顺序）

步骤编号沿用 RELEASE CHAIN 审计。

| 步 | 动作 | 授权依据 | 备注 |
|---|---|---|---|
| S0 | 一次报裁：M0 + M1 的处置 + H1 16 核 + 授权建候选与封存 + 授权整闸 + 写明 Draft/Latest/签名各自的边界 | `plans/v0.0.43.md:5,60`；`AGENTS.md:70` | 用 `release/ruling-requests.tsv` 记一行；先例见 `research/c42-ruling-request.md` |
| S5 | 建候选：`release/tools/build_candidate.sh`（13 步，每步 ≤58 s） | 「候选封存…另授」（plan:60） | 构造和打包已经全走 C；`h1source.py` 仍会被调用 |
| S6 | comboot 定点：用 `exec/c/comboot.sh`，**不要用** `candidate_chain.sh`（写死 `/private/tmp`，macOS 专用） | 随 S5 | 证据：stage2 = stage3 = 候选 sha |
| S7 | 封存到 GHCR：`seal_candidate.sh`，然后提交 candidate.json，跑 gatedeps，推送 | plan:60（0.0.42 先例：政委授） | 要先装 oras |
| S8 | 推 `rc/v0.0.43` 标签 | 随 S7 | |
| S9 | 本地全量队列，1044 项，**推断约 3 小时**，放后台跑 | plan:5「无故整闸」未授，需要 S0 授权；没有 env-pin 不准 resume | 预期红：csmith×41（TOOL_MISSING）、Darwin×8（UNVERIFIED_HOST）、lib-callable-catalog 和 lib-lifecycle（已有 H2 裁定） |
| S10 | 出口表 NEEDS_RULING 一次送裁 | 裁定 | 证据：NEEDS_RULING=0、product_red=0 |
| S11 | CI release-check（push 后自动触发）；手动补跑 darwin-gate.yml | 不需要 | 证据是 run id/attempt |
| S13 | `install_root.sh`（写根目录里 gitignored 的文件） | 主会话的 owner 自己执行 | §16 规定无论最终公开与否都要做 |
| S14 | **Draft**：`release_prep.sh` 在没有 Draft 时会**隐式执行** `gh release create --draft`（`:84`），所以必须授权后才能跑 | `AGENTS.md:38`；§29:384 | 要先装 zip |
| S15 | Windows 签名（可选，0.0.42 推迟了） | 「签名」另授，并要过 release-signing 环境审批 | |
| S16 | Apple 签名（可选） | 另授 | **这台 Linux 做不了** |
| S18 | 公开与 Latest：signed 路走 publish.sh；或按 unsigned 先例 `gh release edit --draft=false --latest` | 「Latest 仍须另授」（`AGENTS.md:38`），并要主人确认（§16:197） | 之后跑 release-smoke 六格 |

建议的授权打包方式：
- **第一包**（S0）：裁定 + 候选 + 封存 + 整闸。
- **第二包**（S10 结束后）：出口表裁定 + Draft。
- **第三包**：Latest/公开（签名与否）。

---

## (D) 流/网范式下的重构方案

### D.1 推荐的组合设计：TDESC 做主干，state 做护栏，cell 只取统一的链运行器

两位评审都把 tdesc 排第一（7.5 和 8.5 分），state 第二（6.5），cell 第三（4.5）。组合方式如下：

- **主干：TDESC（E3 内部的规范类型存储，不新增第八条流）。**
  - 类型以「从名字往外读的派生串 + 基类型」的形式只追加不改写，例如 `A2PA3i32`，存成 hash-cons 后的 id。
  - sizeof、解引用、下标、步长、签名相等、晚绑定的 LB-8 比较，都改成对同一个描述符做操作。
  - 金网（type/tyinfo/pfconv/binsel）只看有限的 `TYCLS`，它们的域和封存都不变。
- **护栏：state 的 statecheck（构造期的类型化状态声明）。**
  - 真相源是**构造出来的 delta JSON**，不是手写的 `tests/regs-e3.tsv`。
  - 它能补上已确认的覆盖洞：`regcheck.cx:79` 只解析 `NAME vpush r1,…` 这种形式，JSON 形式的 membercontrol 和 initializers 栈帧被整个跳过（两位评审都复核过）。
  - 只取它的 S0–S3。S4 视情况做。**S5/S6 不做**：那是 12–14 天的纯重排，不改任何行为。
- **运行器：cell 的 S0。**
  - 把 `stagechain.cx` 泛化成读链表的 `cellchain.cx`，同时把构造（--construct）和运行（--run）拆开。
  - 拆开后，每个 E3 变体的构造（24–36 s）和 650 文件差分（每文件约 0.5 s，每批约 80 个）各自都能放进 58 s 的一步。
  - cell 的 UREC1 记录协议、流内拒绝记录、e3.parse/sema/emit 拆分**推到 0.0.45 以后**。理由：中间流没有外部参考；拒绝顺序在有前瞻的地方有风险；拆分期间状态数会上涨。
- **贯穿各刀的规则：生成器先知 + ratchet。**
  - 已经由 7ca340d5 落地。
  - 任何 WRONG 只能通过「让描述符成为唯一来源」变成 OK，不允许再加路径补丁。
  - ratchet 对任何方向的判定变化都报红，必须显式记录。

### D.2 为什么它能消除「每条路径一个补丁」这类缺陷

| 病根（实测或已读到的） | 组合设计里的对策 |
|---|---|
| 类型被压平成「指针深度 + 基类型 + ≤8 维 + 标志」，分散在 td/tb/type_rank/type_base_depth/fp_more 等寄存器，以及 PTR/DIM/TDIM/SHAPE 等内存区里。`SH.ONEPTR` 甚至要用 `td - type_base_depth == 1` 去反推数组的位置 | 改成只追加的派生串，每个星号、每个后缀都是一条独立追加的操作。「少一层指针」「丢一维」**在表示里写不出来**，sizeof 只是读 `TYSZ[vty]` |
| 值记录有 9 个独立寄存器，235 处只写 `vt,vb,value_rank`、不动 `rkok,rk,vid,szrow*`（state M12）。szrow 那类缺陷（e02d924a）就是这么来的 | 用 `vty`（不衰减的类型 id）加 `vcat` 取代。衰减由消费方显式调用 `RV()`；在过渡期，VALUE 记录的帧由声明推导出来 |
| 保存和恢复有 4 种手写语法：结果行里内联 STX、manifest 的 mapseq、两种 `-stack.tsv`、recipes；检查器只看得到其中一种 | statecheck 读构造产物，规则包括：寄存器必须已声明、只读不写的必须是 init0、存取基址必须是已声明的 bank、帧必须等于某个已声明帧的展开。漏存一个字段在**构造期就红**，不用等探针撞上 |
| 每修一处，都要人工同步 freshorder、knownfail、abi.manifest、gatedeps 等派生台账。本会话的 13 个红里有 4 个就是这样来的 | R4 并行做：把派生台账接进 `release/derived-map.tsv`，自动重算加 `--check`。这不在三份设计之内，两位评审都指出要单独跑 |
| 三个实现（产品、经典、Python seed）各自按路径补同一个缺陷类：经典 13 WRONG，产品 2 WRONG | 三者共享同一个 TDESC 格式和同一套探针域，但不共享代码。经典参考单独有 `src/tdesc.c`（TDESC S7） |

### D.3 有序切片（每刀都有见证与闸；天数是设计估计，评审已标为未测）

**通用闸 G**：清洁 HEAD 构建的产品对比候选产品，在 650 个 .c（tests/ 与 examples/）上逐字节比较 `-S` tape、rc、stderr，分批，每批 ≤58 s。外加 tdescprobe 的 ratchet、`regcheck`（之后换成 statecheck）、每个变体的 `--check-net`。

**改 e3.net 的刀额外要付的固定成本**：e3-seal reseal（`tests/reseal-e3.sh`，case 字节不变）、stage-contract 重录、facts export 与 abi.manifest 在**同一提交**里刷新（f8f45cb6 的教训）、8 个变体每个都要过 ≤58 s 的构造。

| 序 | 刀 | 字节 | 见证实验 | 闸 | 估时 |
|---|---|---|---|---|---|
| R0 | **已落（7ca340d5，待推送）**：tdescprobe 窄版，106 探针 | 0 | oracle 对 cc 106/106 | tdescprobe-oracle、-classic、com-tdescprobe-1..3 | 已完成 |
| R0b | tdescprobe 扩域：派生加入 A2 和多维（覆盖 d13/d14）；基类型加 char、struct；上下文加 global/local/param/member/typedef/type-name；用法加衰减后使用、调用、**运行时下标**（覆盖 e05） | 0 | 基线 BAD 集必须包含 d04、e05、d07、d13、d14、d16、e02（tdesc 审计实测的形状） | ratchet 只允许单调变化；每个分片 ≤40 文件、≤58 s | 0.5–1 天 |
| R0c | **state S0：statecheck.cx 读构造出的 delta JSON**；声明 regcheck 漏掉的 5 个部分保存（M9）；挂成 stagechain 的构造后置条件 | 0 | 变异测试：从某个帧里删掉 `szcomma`，或从 membercontrol stack8 删掉 `rk`，必须变红 | regcheck 加每个变体一个 statecheck 叶。**先测 .cx 读 61–67 MB JSON 的时间和内存**（没测过，历史上有 5.25 GB OOM 的 waiver） | 0.5–1 天 |
| R0d | **cellchain S0**：stagechain 泛化为读链表的运行器，拆成 `--construct` 和 `--run` | 0 | 在 308 个文件上，判定行和 stagechain 完全一致 | 新增 cellchain-e3 作业（contract 层），给 stagechain 冷跑 >60 s 的超时债销账 | 0.5–1 天 |
| R1 | state S1–S2：寄存器普查（owner/lifetime/kind）加 312 个 bank 行；规则 1–4 生效 | 0 | 改错一个寄存器名，或删掉 `diag_multi` 的 init0 行，必须红；改一个基址立即数，必须红 | statecheck | 1.5 天 |
| R2 | state S3：删除 5 个死 `-stack.tsv`（vla、callcontrol、constexpr、printfcontrol、unarycontrol）。R0c 复核（2026-10-23，tests/statecheck.cx 读构造出的 delta）：`exec/parse2/control-stack.tsv` 与 `exec/parse2/callcontrol-stack.tsv` 都没有活的消费方（构造器与 export.py 均不读；regcheck 读 control-stack.tsv 只是文本），control-stack.tsv 一并列入候选 | 0（推断） | **8 个变体全部**做 delta cmp。目前只做了 2 个（M11） | 每个变体一个 ≤58 s 的 cmp 步；G | 0.5–1 天 |
| R3a | **已做（2026-10-23，`research/c43-r3a/REPORT.txt`，原型 `prototype.diff`）**：(1) **GO**——一条 25 动作的 `SBINTERN` cons 序列（类型串以 0x01 打头，与标识符不撞），等型同 id、异型异 id；只写新寄存器 `tyx*` 与新区 2.000e15–2.004e15，650 个 `.c` 的 tape/rc/stderr 全同（实测）。缺口即 R3 见证集：`int (*p)[3]` 与 `int *p` 同投影；函数指针的 `tb` 带逐声明序号，需函数签名描述符。(2) **NO-GO（一般情形）**——`fpback` 重扫经 INITCOUNT→常量表达式→`sizeof`→类型/表达式解析可达 BIND/tag/lookup（写 `usp v tagusp tg sid cur`）；实测缺陷：`int (*fps[])(int) = { [sizeof(enum E{EA=1,EB})] = f, f };` 被拒 "not covered: enum tag redefinition"（gcc 收）。**结论：R4 不走影子重扫，改在主遍每个声明符事件处用 cons 行建描述符。** | 仓外 | — | — | 已结 |
| R3 | TDESC S1（R3a 后重估 2–2.5 天：加函数签名描述符、`(*p)[N]` 形状、六种声明上下文、作用域存取帧带每符号类型库）：TY 区与 cons 行；在提交点把旧寄存器投影成 TDESC，写入 `STY`；只有显式资源 `e3/tdesc-trace` 才输出 trace，版本不认识就返回 rc 2，不做回退 | tape 不变，e3.net 改变 | 投影出的 TDESC 和生成器期望之间的不一致，必须**恰好等于**有损形状的集合 | G + 固定成本 | 1.5 天 |
| R4 | TDESC S2：**主遍**声明符事件处 cons 出 `dty2`（R3a：不做影子重扫），只往 trace 里写 | tape 不变 | 所有探针上 `dty2` 等于期望；650 个语料文件上 `dty == dty2` | G + statecheck（影子路径没有新增写入） | 2 天 |
| R5a–e | TDESC S3 **按上下文拆成 5 刀**：object、param、member、typedef、type-name。DECL 成为唯一的声明符，旧段落按上下文删除，COV 负责投影或具名拒绝 | 只在探针上新增具名拒绝 | 每一条现有类型形状拒绝都有一个负例，stderr 字节和位置都相等；d04、e05 从 BAD 变成 not-covered | G + exec-r21-e3、edge-e3-parse seal/ablate、edge-e3-pfconv、stage-contract | 约 3 天 |
| R6 | TDESC S4a：`vty` 加 `TYSZ`，删除 szrow*（也就是 state S7 的「VALUE 携带 szrow」折叠，只做一次） | 不变 | `b_sizeof_rows.c` 和经典缺口文件都与 cc 一致；部分保存从 8 降到 ≤5 | G + statecheck | 1 天 |
| R7 | TDESC S4b：用 OPK 表驱动解引用、`[]`、`+`/`-`、`&`、cast；rk/rkok/vid 退役。**WRONG 变 OK** | 探针改变（声明过的） | ratchet 的 BAD 集严格缩小，语料不变 | G | 2 天 |
| R8 | TDESC S5（成员 MTY、初始化行字节）；S6（F 签名 id，`us_type_descriptor` 成为投影，USLCON1 字节不变，USLCON2 只在版本资源下出现） | USLCON1 不变 | com-bindings-neg、com-extent-neg、libraryexports parity | G | 3 天 |
| R9 | **从 R3 起并行**：TDESC S7，经典参考 `src/tdesc.c` 加不稳定的 `-dump-tdesc` 开关 | 经典 tape 不变 | tdescprobe-classic 的 13 WRONG 归零 | 经典 `-S` 跑 650 文件，除迁走的探针外全部相同 | 2 天 |
| R10 | TDESC S8：删除已死的投影寄存器；视情况做 state S4（frame/move 作为唯一源） | 不变 | statecheck；部分保存降到 0 | G | 1–2 天 |
| 之后 | cell S1（`regcheck --partition`，只做测量）。到 0.0.45 以后再做 UREC1 和 e3.parse，让它直接产出 TDESC 派生串 | | | | |

**估时与版本归属**
- 关键路径约 15–18 个工作日（设计估计；R9 并行时可以压缩约 2 天）。
- R0b–R2 都是零字节，可以在 0.0.43 等待队列时做，不算收口条件。
- R3 及以后进 0.0.44。

**仍然留给其他工作的**：非类型类的 E3 缺陷；pp、lex、lower、enc 的点修；E3 24–36 s 的构造时间；每刀都要 reseal 的成本。

---

## (E) Python 仍在关键路径上的位置，以及 C/.cx 替换顺序

**已经是 C 或 .cx 并且默认启用的（来自 RELEASE CHAIN 审计）**
- 构造和打包：`SEED_C=1 SEED_GEN=1`，即 gen/tbl/net/blob/compilerpack/pack/ape.c；
- 来源身份 `ident.c`；种子 `build_seed.sh`；定点 `exec/c/comboot.sh`；参考构建 `build_ref.sh`；看门狗 `tests/bound`；静态镜像 `staticimagecheck.cx`；以及 regcheck.cx、stagechain.cx、tdescprobe.cx。

**仍然只能走 Python 的（关键路径）**

| 位置 | 用在哪里 | 替换顺序（全部是设计） |
|---|---|---|
| `rc_tag.sh:10-11`、`seal_candidate.sh:7-8`、`publish.sh:11` 的 JSON 读写；`installpair.sh:31` 的 same_origin | S7、S8、S18、S9 | **1**：写一个小的 `release/tools/jsonfield.cx`（读写字段），代码量最小 |
| `h1source.py`（`build_candidate.sh:94` 无条件调用） | S5 | **2**：换成 .cx，从此建候选零 Python |
| precheck 的廉价段：`freezecheck.py`、`subtractsafety.py`、`ledgercheck.py` | S2、S13 | **3**：freezecheck 和 M1 的口径修改**一起**改写成 .cx；另外两个随后 |
| `exec/facts/export.py`，以及 `parse2base.py:slots()` 里的 rank 伴随展开 | 每一刀 E3 重构（R3 起 k2 常量会变） | **4**：在 state S4 的 stategen.cx 里直接产出 k2-stack 事实，export.py 只保留做 `--check` 对拍 |
| `refresh_gatedeps.py` | S3、S7 稳定点 | **5** |
| `exittable.py`、`unsigned_receipt.py`（后者无条件写 `local_gate_complete:true`，r42 已记为 caveat） | S10、S14 | **6** |
| 整个门禁队列：`gatequeue.py`（68 KB）、`release.sh`、`gate.sh`（386 处 python3）、queuestart、gqalive、stagelog | S2、S9 | **7（最大，放最后，0.0.45 以后）**。先让 cellchain 和 statecheck 这类 .cx 作业替掉各自的套件，再考虑替队列本身 |
| tools-2 分片的 `python3 -m unisa compile`（`tests/tools.sh:92`） | 闸作业，正在超时 | 改成用产品跑；同时能给超时债销账 |
| CI runner 上的 `unisa` 参考、comdemo、winsuite；`macosbundle.py`；`draftfetch.py` | S11、S16、S17 | 不在本机的约束范围内，排在最后 |

**目标是 100% 零 Python 依赖（主人 2026-10-23 重申硬目标；原「保留当 oracle」作废）：** 当前基线：`python3` 调用点 **2253**（`tests/gate.sh` 占 183）、`import unisa` 的文件 **86**、剩余 `.py` **452**。Python seed（`unisa/`）只是过渡参考物，这三项同时清零后才移入 `archive/`（不删，仅人工参考，不参与构建、闸与验证链，也不充当“裁判”）。现存 86 个文件仍 `import unisa`（`abi_audit.py`、`gold_audit.py`、`consts_check.py`、`layout_check.py`、`modelsignature3check.py` 等），这是 py2cx 的真正尾巴：逐个改为由产品自身判定或迁 `.cx`。`export.py` 在 R3 期间的临时用途按替换顺序第 4 项消掉；原文“R3 改 k2 常量时仍会运行现有的 `export.py`”仅限于 R3 那一刀，不构成保留理由。

---

## (F) 主会话接下来的 3 个具体动作

1. **推送 7ca340d5，然后发一份 S0 裁定请求包。**
   - 先 `git push`，用 `git ls-remote` 核对。
   - 写 `research/c43-ruling-request.md`，并在 `release/ruling-requests.tsv` 加一行。内容只包括：
     - M0 的范围裁定与 A.2 推迟表；
     - M1 的处置，推荐 (a) 改口径；
     - H1 16 核是否接受；
     - M3 选 (i) 还是 (ii)，推荐 (i)；
     - 授权建候选、封存、整闸（S5、S7、S9）；
     - 写明 Draft、Latest、签名各自的边界。
   - 在裁定回来之前，不碰产品闭包。
2. **一笔只改 tests/ 和文档的修复提交，加上宿主配方。**
   - 提交内容：
     - 重记 freshorder.tsv；
     - 从两个 knownfail 删掉 declsnapshot；
     - 修 subtract-safety 的 63 处；
     - gate.sh 里绝对化 MODEL_COM、钉 `LC_ALL=C`；
     - 补 spec TP-9 的下划线句。
   - 宿主一侧：执行 B1 的配方（装包、umask、corpus 预取包括 c-testsuite）。
   - 然后 `make com`，`ident . check unisacc.com` 要显示 verified。
   - 在新产品上重跑 CONTRACT 层（每批 ≤58 s）和 com-tdescprobe-1..3。
   - 验收：除 B1 已登记的宿主项外，零红；tdescprobe 基线在新产品上复核通过。
3. **补齐收口文档，然后按授权进入发布链；等队列时并行做零字节重构刀。**
   - 在新产品上写 M5 的晚绑定分类状态表和 M6 的第 4 项基线复核（只改文档）。
   - 裁定落地后：如果 M3 选了 (i)，先做那一刀 `fix:`；然后按 S5→S6→S7→S8→S9 走。
   - S9 队列在后台跑的约 3 小时里，在**独立工作树**中做 R0b（tdescprobe 扩域）、R0c（statecheck，先测 JSON 读取的时间和内存）、R0d（cellchain 拆分）。这几刀都是零字节，不进 0.0.43 的收口条件，提交前缀用 `tests:`。

---

**主要不确定性**
- 7ca340d5 不在闭包内是推断。
- fresh-order-2..7 的红是推断。
- 3 个超时套件在低负载下能否转绿，未测。
- M3(i) 能否一刀覆盖所有上下文，未测。
- 1044 项队列约 3 小时是按比例推断。
- statecheck 读 JSON 的时间和内存未测。
- TDESC 的 SBINTERN cons 与影子重扫可行性未测（由 R3a 决定）。
- 所有行数、状态数、天数都是设计估计。
