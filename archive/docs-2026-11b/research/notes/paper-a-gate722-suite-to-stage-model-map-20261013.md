# Paper A 材料：release gate ≈722 套件族 → 阶段模型映射（非 A seal-before）

- **拍点：** 2026-10-13 ~20:00 Asia/Singapore（UTC+8）；**§6 四缺口补核** ~20:04（tip `20ecaeac`）
- **性质：** 研究笔记／梳 0.0.42 后 → 0.0.43+ 材料；**不发布、不投稿、不改正文**
- **权威（已机核 tip `a940aa08`）：** `tests/gate.sh --list --com` = **722**；`tests/gatelayers.py --check` → contract **79** / stage **377** / pipeline **217** / platform **49**；调度 `tests/release.sh` → `gatequeue.py --jobs 4 --window 55`
- **前缀粗计（与机房主任口径一致，本拍复核；~20:04 补核收口）：** exec≈152、com≈93（`com-` 前缀＝常开3＋仅 `--com` 的90；≠「一切产品路线」）、lib≈85、seed*≈85（`seed*`+`seedgen*`）、rowcov≈50、csmithdiff≈40（端到端另有 difftest／corpus／ccrun／closure／stages 等非 `com-` 前缀）
- **排他／重载：** `release.sh` 具名 exclusive **Linux 基底 15**；Darwin 另加 `bindprep-arm64`／`x86_64`，仅 Darwin/arm64 再加 `exec-memx86-ua-1` → arm64 合计≈**18** exclusive；Linux-only exclusive：**无**；全部 `rowcov*`（≈50）走 `--heavy-suite`（并发上限 1，两宿主同）；窗末未跑完 **rc=75** = PENDING／可续跑，不是失败

---

## 1. 背景（为何统；非 A 封口前闸）

政委经机房主任要一张「产品 release gate 的套件结构」到「Paper A／PRD 已有阶段用语」的对照，方便 0.0.42 封存后梳 0.0.43+ 材料。

两层勿混：

| 层 | 库内用语 | 回答的问题 | 是否 Paper A 阶段 |
|---|---|---|---|
| **诊断层** `gatelayers` | contract / stage / pipeline / platform | 一次失败该先怀疑「声明／单子系统／整条路线／目标宿主」哪一层 | **否**（工程诊断广度，见 `tests/GATE-LAYERS.md`） |
| **管线阶段** Paper A §4.2 | 预处理、词法、解析、优化、剪枝、lowering、编码与镜像；表 1 短名 **pp / lex / parse / enc / …**；库内还见 **parse2、tape、tapebin、seed、image、comboot** | 网络编译器字节流阶段与 IR／产物 | **是**（论文阶段模型） |

本笔记只做映射与缺口，**不**把 GATE／acceptance／真红升成 A seal-before（与 `paper-a-derive-purify-c42-1640-acceptance-failed-true-reds-not-a-gate-20261012-2132.md` 同边界：**A seal ≠ product acceptance PASS**）。

---

## 2. 套件族 → 阶段模型映射表（只用库内已有用语）

**阶段用语来源（禁止自造）：**

- Paper A §4.2 七阶段：预处理 → 词法 → 解析 → 优化 → 剪枝 → lowering → 编码与镜像；中间 IR：**tape**
- 表 1／事实表短名（节选）：**pp、lex、parse、enc、regmap、…**
- 产品／闸名已出现、可对上论文阶段的词：**parse2**（解析侧实现／套件前缀）、**seed**／**seedgen**／**seed-construct**（离线构造／种子链，非七阶段之一）、**image**／**fat**／**elf**／**macho**／**pe**（编码与镜像侧）、**comboot**（产品自举／`make comboot` 与 `com-comboot-*`，属自举／产品路线，不是 §4.2 第七阶段的别名）、**tapebin**（tape 规范二进制，§5.5）

| 套件族／前缀（约数） | gatelayers 主落点 | 主要挂靠的论文／库内阶段用语 | 材料一句话 |
|---|---|---|---|
| **exec-***（≈152） | 多为 **stage**（≈123）；另有 pipeline／platform 切片 | 名中可见 **pp / lex / parse2 / lower / prune / chain**；大量为驱动、告警、内存、自编译切片，**未挂名单阶段** | 单子系统或参考对拍的主仓；能点名阶段的是少数，其余是「执行器／宿主／产品路线」碎片 |
| **com-***（≈93） | 几乎全 **pipeline** | 整包产品路线：`com-difftest*`、`com-forward*`、`com-comboot-*`、`com-tapebin*`、`com-seedgen` 等 | 「已装运 `.com` 走完整编译／运行／自举」——跨多阶段，失败时难直接钉到 pp 或 enc |
| **lib-***（≈85） | 几乎全 **stage** | 不在 §4.2 七阶段内；属 **宿主 ABI／库／FFI／union／carrier** 等（论文 §4.4「不属于模型的部分」／nativeabi 旁支） | 阶段叙事上应标成「模型外宿主边界」，勿写成第七阶段 |
| **seed\***／**seedgen\***／**seed-construct-***／**seedparse2-***（合计 ≈85） | 多为 **stage**；矩阵目标项偏 **platform** | **seed**＝构造／种子；`seed-construct-parse2-*`、`seedgen-pp*`／`opt*`／`prune*`／`parse2consumer*` 按名挂 **parse2 / pp / opt / prune** | 离线构造与种子消费；对论文是「构造器／种子链」实证，不是运行时七阶段本身 |
| **rowcov-***（50） | **stage** | 显式挂 **pp / lex / parse2 / lower / enc**（含 build／union／分片） | 覆盖率棘轮按阶段切开；见下「覆盖率分片」 |
| **csmithdiff-***（40）＋**difftest\***／**corpus-***／**ccrun-***／**closure-***／**stages-*** 等 | **pipeline** | 整条路线差分／语料；不点名单阶段 | 端到端正确性证据；阶段叙事需二次分诊 |
| **unisa-***（≈31） | **stage** | `unisa-gold-*-label` 挂 **pp / lex / parse / peep / …**；`unisa-image-*`／`unisa-tape*` 挂 **image / tape** | C↔Python 点采样；已有 derive-purify「K6 ≠ A gate」叶，材料可引用，勿升 seal |
| **manifest-entries-{pp,lex,parse2,enc,lower,…}** | **contract** | 直接点名阶段清单合同 | 声明／清单健全性，不是运行时逐步对拍 |
| **bigclosure-***／**nativeboot-***／**fat-***／**winposix**／**ccinterop**／部分 **exec-*-self** | **platform** | **image**／目标三元组／原生启动 | 目标镜像与宿主矩阵；论文 §8.1#3 矩阵义务的工程侧，≠ T1 |
| **gate-***／**queue***／**envpin**／**warmup**／**freezecheck**／**ledgercheck** 等 | **contract** | 无阶段挂靠 | 闸与队列元合同；失败多为工程卫生／宿主撞车 |

**短对照（七阶段 ← 闸名里能直接看见的例子）：**

| §4.2 阶段 | 库内短名／近义 | 闸名举例（非穷尽） |
|---|---|---|
| 预处理 | pp | `exec-pp-*`、`rowcov-pp*`、`manifest-entries-pp`、`seedgen-pp*`、`unisa-gold-pp-label` |
| 词法 | lex | `exec-lex*`、`rowcov-lex`、`manifest-entries-lex`、`unisa-gold-lex-label` |
| 解析 | parse／**parse2** | `manifest-entries-parse2`、`rowcov-parse2-*`、`seed-construct-parse2-*`、`seedparse2-*`、`unisa-gold-parse-label` |
| 优化 | opt／peep（表侧） | `opt-run-*`、`seedgen-opt*`、`unisa-gold-peep-label` 等 |
| 剪枝 | prune | `exec-prune-*`、`seedgen-pruneconsumer-*` |
| lowering | lower | `exec-lower`、`rowcov-lower-*`、`manifest-entries-lower` |
| 编码与镜像 | enc／image | `rowcov-enc-*`、`manifest-entries-enc*`、`unisa-image-*`、`fat-*`、`bigclosure-*` |
| （IR，非单独第七阶段） | tape／tapebin | `tapebin-*`、`com-tapebin-*`、`unisa-tape*` |
| （构造／自举旁路） | seed／comboot | `seed-*`、`seedgen*`、`com-comboot-*`、`com-seedgen` |

---

## 3. 三分类：阶段合同／宿主平台噪声／覆盖率分片

下面按**材料梳妆**用，与 `gatelayers` 四层正交（一层套件可同时带两色）。

### 3.1 「阶段合同」

**含义：** 失败时，材料上能较稳地写到「某一论文／库内阶段（或单子系统）与参考不一致」，而不是「整包红了」。

**典型：**

- `gatelayers=stage` 且名中带 **pp／lex／parse2／lower／enc／prune／opt／tape／gold-***：如 `exec-pp-directives`、`exec-lower`、`exec-prune-*`、`unisa-gold-*-label`、`seed-construct-parse2-*`（按片）
- `gatelayers=contract` 且点名阶段清单：`manifest-entries-{pp,lex,parse2,enc,lower,…}`、`tapebin-shape`／`tapebin-roundtrip*`、`dsl-ops`、`decision-ledger`
- `lib-*` 中明确子系统合同（ABI／carrier／union…）：对论文应记为 **模型外宿主边界合同**，勿塞进七阶段表

**注意：** `com-*`／`csmithdiff-*`／`difftest-*`／`exec-chain-*` 等虽重要，但是 **pipeline 合同**——证明整条路线，默认**不能**直接写成「pp 阶段合同失败」。

### 3.2 「宿主／平台噪声」

**含义：** 结果强依赖本机工具链、内存、超时、目标是否可跑、另一队列是否撞车；材料上应先分诊为宿主／资源，再谈产品阶段。

**典型：**

- `gatelayers=platform`：`bigclosure-*`、`nativeboot-*`、`fat-*`、`winposix`、`ccinterop`、部分 `seed-matrix-{lnx,osx,win}-*`、跨目标 `exec-*-self`／绑定准备
- 调度强制串行的胖项：`lib-carrier-import-model`、`com-seedgen`、`seedparse2-*`、`seedgen`／`seedgen-2`、Darwin 上的 `exec-bindprep-*` 等（Linux exclusive 基底 15；Darwin/arm64 至≈18，见 §6 补核①）
- 桶回执已示范的 rc 类（见 `research/c42-1640-bucket-receipt.md`）：**rc77** 宿主不适用、**rc142** 超时、**rc2** 资源未知、队列元套件撞车——记噪声／债，不直接写成阶段语义红
- 缺宿主工具（zig／跨 ld／仿真）导致的 `windows-resolver-host`、`elfobj`／`com-elfobj` 等

### 3.3 「覆盖率分片」

**含义：** 同一阶段的覆盖义务被切成多 job，为过 55s 窗与内存；绿／红要连同 **build／union／前驱** 一起读。

**典型：**

- **rowcov**：`rowcov-pp*`、`rowcov-lex`、`rowcov-parse2-{1..24}`＋`build`/`union24`、`rowcov-{lower,enc}-{1..8}`＋`build`/`union8`（共 50；`--heavy-suite` 并发 1）
- **seed-construct-parse2-*** 长串分片、**seedgen-\*consumer-*** 按阶段消费分片
- **csmithdiff-***／部分 **difftest-***／**corpus-***／**stages-***：语料／差分分片（pipeline 色，但调度形态同「分片」）

**材料纪律：** `rowcov-parse2-build` 超时 → 全部 `rowcov-parse2-*` 前驱连锁红（c42-1640 已见）——叙事上应写「parse2 覆盖率构建／分片链」，不要写成「24 个独立 parse2 阶段合同同时破产」。

---

## 4. 缺口：增量测试结构缺失如何伤阶段叙事（供 0.0.42 后梳 43+）

1. **指纹可续跑，但「新候选／新状态目录」常逼整表重开。**  
   `gatequeue` 按 job 指纹（声明输入、设置、可执行物、env-pin 等）决定能否复用；`release.sh` 在树变时仍可复用指纹未变的结果。然而：**whitelist 空时 stamp 含 `MODEL_COM` sha**——仅 cand 字节变 → 逻辑上 722 stamp 全失效；**新空 `GATE_STATE`** → carry 0／≈722。`GATE_STATE` 路径本身一般不进指纹（极少 `LOCATION_BOUND` 例外）。材料上「相对上一候选哪一阶段变红」往往丢失，只剩一整窗 722 的平面结果（实据与边界见 §6 补核②）。

2. **失败难附着到 §4.2 阶段。**  
   约一半以上 suite 名不携带 pp／lex／parse2／… 标签（驱动、告警、lib ABI、com 整包、队列元合同）。`gatelayers` 的 stage／pipeline／platform 是**诊断广度**，不能自动翻译成「预处理／lowering」叙事。库内另有 **`tests/stagemap.c`**（E2 pp…E6 image＋chain／lib／build／meta；≠ gatelayers）：exec≈152 中 paperish≈91、chain+build≈61；tip 上 `stagemap --check` 对全 722 **未绿**（新 suite 未入表）——阶段索引仍不完整，论文侧仍易被整包红淹没（见 §6 补核③）。

3. **分片与前驱掩盖阶段信号。**  
   覆盖率／seed 分片在 build 失败或超时后批量连锁；exclusive／heavy 又改变谁先跑完。阶段叙事需要「根因 job＋阶段标签＋噪声类」，现状队列默认不产出该三维收据。

4. **窗末 rc=75 与验收语气易混。**  
   PENDING 可续，不等于阶段合同未证；acceptance FAILED／真红记债（c42-1640）是产品发布面，**不得**读成「某阶段 T1 或实证封口失败」。

5. **对 0.0.43+ 的梳妆建议（笔记级，不改闸）：**  
   - 材料表维持本页「族 → 阶段用语」列，新增失败时只增行，不改键数／产品码；  
   - 区分三色记账（阶段合同／宿主噪声／覆盖率分片），继承桶回执 rc 分类；  
   - 若工程侧日后做增量结构，优先「同指纹可跨候选目录迁移的阶段子集视图」——本笔记不设计实现，只登记需求痛点。

---

## 5. 与 Paper A 的关系（挂回 A；不进 seal-before）

| 主张 | 本笔记立场 |
|---|---|
| 722／gatelayers／release 窗 | **产品／release 工程**实证载体卫生 |
| 映射到 §4.2／表 1／parse2／seed／tape／image／comboot | **挂回 A** 作「载体如何被测」的材料索引 |
| 须先 GATE PASS／啃绿真红／结清 H1 才许投 A | **禁止**（已有 derive-purify 叶） |
| 须先补齐增量测试结构才许投 A | **禁止**升格；仅 43+ 工程缺口 |
| Softguess／8509·8769／§8.1#3／Ο1\|Ο2 | **不动**；本笔记不叠卡、不重钉 |

**一句话：** 这是产品 release 旁支的阶段挂图，父节点仍是 Paper A（unisacc 实证载体），**不是** A 的 seal-before 闸门。

---

## 6. 缺口 1–4 补核收口（原「未核清」已关）

原四条「需机房主任补文件／口径」已由机房主任在 tip `20ecaeac` 只读核清；回执：`/tmp/unisacc-cdx2/observe-10m/20261013-2004-paper-a-gate722-four-gaps-receipt.md`（方法：`tests/release.sh`、`tests/gatequeue.py`、`tests/stagemap.c`、`tests/gatelayers.py`、`tests/gate.sh --list[--com]`、`~/.unisacc/queue-backup/*/state/results.json` stamp diffs）。**未改** product／gate／本笔记以外文件。

### 机房主任补核（2026-10-13 ~20:04 Asia/Singapore）

① **Darwin exclusive／重载差集**  
Linux 基底 **15**；Darwin 另加 `bindprep-arm64`／`bindprep-x86_64`；仅 Darwin/arm64 再加 `exec-memx86-ua-1` → arm64 合计≈**18** exclusive。`rowcov*`≈50 为 heavy（并发 1），两宿主同。**Linux-only exclusive：无。**

② **换 cand／新 `GATE_STATE` 与 stamp 复用**  
whitelist 空时 stamp 含 `MODEL_COM` sha；**新空 state → carry 0／≈722**。仅 cand 字节变 → 逻辑上 **722 stamp 全失效**。实据：两份不同 artifact 备份 **common720 same_stamp0**。`GATE_STATE` 路径本身一般**不**进指纹（极少 `LOCATION_BOUND` 例外）。

③ **exec 阶段标签表（≠ gatelayers）**  
有库内 `tests/stagemap.c`（E2 pp…E6 image + chain／lib／build／meta）。exec≈152 中 **paperish≈91**、**chain+build≈61**。tip 上 `stagemap --check` 对全 722 **未绿**（新 suite 未入表）。**≠** `gatelayers`（诊断广度）。

④ **com≈93 边界（对外口径）**  
`--list`＝**632**；`--list --com`＝**722**（+90）。`com-` 前缀 **93**＝常开 3＋仅 `--com` 的 90。对外**勿**把 93 写成「一切产品路线」；端到端还有 `csmithdiff`／`difftest`／`corpus`／`ccrun`／`closure`／`stages` 等非 `com-` 前缀。

---

## 7. 变更范围确认

- **只增**本笔记（及可选 `paper-notes-20261009.md` 一行登记）  
- **不改** product／gate.sh／release.sh／gatelayers 分类／version.h／Draft／Latest／键数／Softguess  
