# 术语统一（权威版 v2）—— 流 / 网 / 训练段 / 记账阶段 / 发布阶段

> 2026-10-23 主人裁定：概念先统一，否则一切白扯。
> 本文件是全仓唯一术语真源。v1 只覆盖 stage/net/segment；v2 补齐全仓扫描结果，并修正 v1 的一个错误（`class Net` 已被 `unisa/control/net.py` 占用，网规格不能叫 Net）。

## 1. 词频实测（谁在用、用得多狠）

| 词 | 文档 | 代码 | 说明 |
|---|---|---|---|
| stage | **1006 次 / 213 文件** | **1228 次 / 131 文件** | 最高频，且被两处不同含义占用 |
| net | 848 / 116 | 1897 / 184 | 两个含义（ask 网 / 神经网） |
| pipeline | 339 / 120 | 64 / 34 | 整体流水线，含义稳定 |
| stream | 147 / 58 | 143 / 42 | 与 flow 同义 |
| flow | 106 / 33 | 277 / 38 | 与 stream 同义 |
| segment | 93 / 10 | 49 / 19 | 训练合并段，含义稳定 |
| phase | 23 / 8 | 206 / 31 | **三个含义** |
| pipe | 10 / 4 | 124 / 59 | 与 stream 同义 |

## 2. 冲突清单（逐处核实，非推断）

| 词 | 位置 | 实际含义 | 判定 |
|---|---|---|---|
| `stage` | `exec/pipeline/image-stages.tsv` 列名 | **流**（7 个字节管道阶段） | ✅ 正确 |
| `class Stage` | `unisa/gold.py:462` | **网**（pp/lex/parse/… 18 张） | ❌ 抢占「流」 |
| `STAGES` | `unisa/gold.py:628` | 18 张**网** | ❌ 同上 |
| `class Stage` | `ujs/seed/construct/gold.py:264` | **网**（10 张） | ❌ 同上 |
| `STAGES` | `ujs/seed/construct/gold.py:335` | 10 张**网** | ❌ 同上 |
| `STAGE_NETS` | `unisa/gold.py:634` = `("isel","abi")` | 不在 `TABLES` 里的两张网（`TABLES` 15 + `STAGE_NETS` 2 + combo = 18） | ⚠️ 名字第三义，须改名 |
| `class Net` | `unisa/control/net.py:36` | **神经网络模型**（TableNet / StageNet / UnisaNet） | ❌ 抢「网」 |
| `PHASES` | `release/tools/stagelog.py:18`、`stagesum.py:24` = `code/matrix/freeze/candidate/fixedpoint/accept17/seal/queue/draft/public` | **发布流程阶段** | ❌ 抢「phase」 |
| `phase` 行 | `flownet/contract.cx`（`phase NAME...`） | **记账阶段**（一次 run 的 ask 记账） | ✅ 但需限定词 |
| `compiler phase` | `unisa/control/segment.py`（s1/s2/s3） | **训练合并段** | ⚠️ 应统一叫 segment |
| `flow` 行 | `flownet/contract.cx`（`flow FLOW NET...`） | **流** | ✅ 与 stream 同义 |
| `stream segments` | `research/ujs-paper.md` | **流** | ✅ |

## 3. 统一后的五层（此后只有这五层）

| 层 | 唯一名字 | 含义 | 数量 | 真源 |
|---|---|---|---|---|
| L1 | **Stream**（别名 Flow / stream-stage） | 字节管道阶段：输入格式 → 输出格式 | unisacc 7（e2/e1/e3/e4/prune/lower/elf）· m4/ujs 4（lex/front/select/encode）· tdesc 3（declare/context/size） | `exec/pipeline/image-stages.tsv`、`m4/contract.py FLOWS`、`flownet/tdesc/contract.tsv` |
| L2 | **Net** | ask 事实网：一张有限表 + 全键 exact | 18（8770 键）· 10（3677）· 3 | `unisa/gold.py`、`ujs/.../gold.py` |
| L3 | **Segment** | 训练合并段（多网合一的 masked 模型） | 3（s1/s2/s3） | `unisa/control/segment.py` |
| L4 | **Book phase** | 一次推导中 ask 的记账阶段 | 由 contract 声明 | `flownet/contract.cx` |
| L5 | **Release phase** | 发布流程阶段 | 10 | `release/tools/stagelog.py` |

**单词唯一化规则**
- 说「流」只用 **stream**（`flow` 仅作 flownet contract 的字段名保留，文档首次出现写 "flow（= stream）"）
- 说「网」只用 **net**；神经网络一律叫 **model / neural net**
- 说「stage」**必须带限定词**：`stream-stage`；**裸 `stage` 禁止**
- 「phase」单独出现即视为 `book phase`；发布阶段必须写 `release phase`

## 4. 落名规则（.cx 为准；.py 是迁移目标，不重命名）

> 本仓是 **.cx 体系**：现有 `.cx` 189 个、`.py` 461 个，后者由 py2cx 波次逐个迁走。
> **因此不给 .py 做重命名**——那是给过渡物抛光，迁走时就消失了。

| 场景 | 做法 |
|---|---|
| 新增 `.cx` 模块 | 一律 `net` / `stream` / `segment` / `book phase`；禁止用 `stage` 指网 |
| 迁移现有 `.py` → `.cx` | **在迁移那一刀定名**：`gold.py STAGES` → `net.cx` 的 `net_*`；`segment.py` → `segment.cx`；`control/net.py`（神经网）→ `model.cx` |
| 已存在 `.cx` 里的错用 | 顺带修：`adapter/ujswasm/ujsgold.cx` 注释 "one per stage" 说的是**网** → 改 "one per net"；`tests/copycontractcheck.cx` 的 `stage` 变量实为构建目标名 → 改 `target` |
| `unisa/` 下的 Python（自举已完成，N22） | **定位是归档参考，不是活 oracle**（主人 2026-10-23）：自举完成后移入 `archive/`，不删，只作人工参考，不参与构建 / 闸 / 验证链。现存 **43 个 `tests/` 文件仍 `import unisa`**，逐个改为由产品自身判定或迁 `.cx`；迁移前不做任何重命名 |
| 文档 | 已落 `prd.md §0` + 本文件；新文档按 §0 用词 |

**不动的**：`image-stages.tsv` 的 `stage` 列（它确实指流）、`flownet` 的 `flow`/`phase` 字段名、C/.cx 里的既有符号语义。

**已废除**（v2 初版里针对 .py 的重命名清单）：`class Stage → NetSpec`、`STAGES → NETS`、`control.Net → NeuralNet`、`SegmentStage → SegmentNet`、`PHASES → RELEASE_PHASES` 一律不再对 .py 执行；它们只在对应文件迁 `.cx` 时以新名字出现。

## 5. 文档重灾区（按 stage 次数，需逐处判定后替换）

`research/ujs-paper.md`(25) · `README.md`(22) · `iterate/construct/README.md`(21) · `tests/k2translate.handoff.md`(16) · `research/unisacc-paper.en.md`(15) · `research/r20-2-reject-samples.md`(15) · `iterate/kernel/README.md`(15) · `exec/c/PACKAGE.md`(15) · `prd.md`(14) · `research/t2-coverage-design.md`(13) · `research/notes/paper-a-gate722-suite-to-stage-model-map-20261013.md`(13)

**替换规则**：每处 `stage` 先判定是 L1（流）还是 L2（网），再替换为 `stream` / `net`；无法判定的**不得盲改**，标 `TODO(term)` 集中裁。

## 6. 闸（防止再退化，以 .cx 为准）

新增 `tests/glossarycheck.cx`：

| 检查 | 判据 |
|---|---|
| G1 | 新增/迁移的 `.cx` 里，`stage` 不得用作**网**的含义（标识符、注释都算）；`tests/sealparadigm.cx` 的 `Stage` 结构体指的是**流**，合法 |
| G2 | 新增 `.cx` 里不出现 `PHASES` 指发布阶段（写 `release phase`） |
| G3 | 新增 `.md` 里不出现裸 `stage`（例外：代码块、文件路径、`stream-stage`/`book phase` 限定形式） |
| G4 | `.cx` 里不出现 `STAGE_NETS` |

## 7. 执行顺序

1. **冻结本文件与 `prd.md §0`**（已完成）
2. 修 `.cx` 侧既有错用：`adapter/ujswasm/ujsgold.cx` 注释 "one per stage" → "one per net"；`tests/copycontractcheck.cx` 的 `stage` 变量（实为构建目标名）→ `target`
3. **`.py` 不重命名**：随 py2cx 波次迁移时按新名落盘（`net.cx` / `segment.cx` / `model.cx`）；长期 oracle（`unisa/gold.py`）只加顶部术语注释
4. 文档替换：按重灾区顺序，逐处判定，不盲改
5. 上闸 `tests/glossarycheck.cx`
6. 每步单独 commit，闸绿；**不改语义、不改字节**

## 8. 一句话

流只有 **stream**，网只有 **net**，训练合并叫 **segment**，记账叫 **book phase**，发布叫 **release phase**；`class Stage` 是把网的类名写错了，`class Net` 是把神经网的名字写宽了。修名字，不修设计。
