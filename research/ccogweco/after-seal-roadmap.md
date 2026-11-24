# 0.1.0 之后的 0.1.x 路线(ccogweco,写作,2026-11-24)

前提:R37 已把 0.0.108 移出封版关键路径(诊断件归 0.1.x 首个产品车);0.1.0 之后的第一批装车按本页。
判定词只用:就绪/待测量/有补丁/待补丁/阻塞;预计冷态凡未实测一律写 UNMEASURED,推断值标"推断"。

## 1 车队规则:一车一类继续,工具线例外

- 版本车继续"一版一类"(集成车试行规则):每车一个产品改动类别,单一写者,单车批数控制在 5 到 6 件。
- 工具线与测试材料类(WP1-WP8、S5、doc-links 棘轮、提交钩子按层跑批、precommit cap 后续、check-split-parents)
  不是产品改动,按 CLAUDE.md 的 devsvc/工具例外直接落 main:tool-only 提交,带 check-*.cx 与批次登记,
  `--keyed` 只带自己行;不占版本车,不跟冻结一起停;在飞冲突按 tests/batches.tsv / batch-product-inputs.tsv 串行。
- 证据类(准入七条)不占车,R37 已定。

## 2 件清单(依据/依赖/预计冷态/车次)

### A 诊断类(0.1.x 首个产品车,原 0.0.108 件)

| # | 件 | 依据 | 依赖 | 预计冷态 | 车次 |
|---|---|---|---|---|---|
| A1 | c15 未定义函数前移编译期 | tests/diag-contract.tsv c15 x1_undef.cx:4 KNOWN-GAP 到期 0.0.97;version-seq 第 50 行 | 改 exec/parse2/control 与事实表,需 derive | 单批 diag-contract 约 20 秒(推断) | 0.1.1 诊断车 |
| A2 | c16 no file 候选路径(searched/package-root) | 同上 c16 k_use.cx/x3_modseg.cx;2644f21 只是准备提交(UNVERIFIED) | 按现行树重做 2644f21,过 gatekeeper | 约 20 秒(推断) | 0.1.1 同车 |
| A3 | c17 does not export 带 use 行 | 同上 c17 q_sym.cx:1 | 可能依赖 c16 的模块诊断基础 | 约 20 秒(推断) | 0.1.1 同车 |
| A4 | -o 误覆盖源文件警告 | version-seq 第 50 行(从 0.0.113 并入) | 需新 diag-contract group 合同行 | 10-20 秒(推断) | 0.1.1 同车 |
| A5 | diag-contract 基线消减 | 每转一行 KNOWN-GAP 才降 DC_GAP_BASELINE(现 15) | 与 A1-A4 同批,不得先降基线 | 随 A1-A4 | 0.1.1 同车 |
| A6 | 103 新判据落地 | cc 决定 103 归 0.1.x;v0103-new-criteria.md | one.cx 已找回,新夹具待写 | 每轮 5x2x5 次,空机数分钟(UNMEASURED) | 0.1.1 或 0.1.2,取决于夹具进度 |

### B 材料/拆批补丁栈(工具线,直接落 main,不占车)

| # | 件 | 依据 | 依赖 | 预计冷态 | 顺序 |
|---|---|---|---|---|---|
| B1 | WP2 precommit-gates/commit-own 材料化 + toolsloc 分离(9 批) | margin-packages.md WP2;现测 precommit-toolsloc* 46.2-46.3 s KILLED、gates-na 53.1 s | 与 107 追加集的 precommit list-cap 改动同文件,须等其落库;cdx 主刀 | 15-24 秒(推断);precommit-toolsloc* 须 <=22 秒 | B1 首件 |
| B2 | WP4 表/棘轮/计时/安装校验材料与分片(5 批) | 同上 WP4;tsv-lint 55.8 s KILLED、install-ape 46.4 s KILLED、batch-timing 55.5 s 首冷 | 与 E8 affected-canary 追踪同表;WP4 材料与拆片同批 | batch-timing <10 秒;其余 12-23 秒(推断) | 与 B1 并行(不同文件域) |
| B3 | WP8 slice-inventory/flow/skill/signal-targets(4 批) | WP8;已有 wp8-slice-inventory.patch(fdfb175b) | gatekeeper 过 + 一次冷测;slice-inventory 与 skill-cx 要改源码 | 12-20 秒(推断);flow-tool 可能已 <25 秒,先复测 | B1 之后 |
| B4 | WP6 cx-booter-ape/cx-npfault/s8c-bind/wt-gate-clean/dev-product(5 批) | WP6;已有 wp6-rest.patch(3c8d890f)与 wp6-wt-gate-clean-b.patch(5c7eae90) | wt-gate-clean 需 run-batch 材料路径(备选 A/B 二选一,cc 定) | 12-24 秒(推断) | B3 之后 |
| B5 | WP1 devsvc 核心/冻结/Git 材料包(9 批) | WP1;devsvc-core-b 50.6 s、devsvc-remain 51.2 s | 与 cdx g35-followup-v2 同域;ompgpt 主刀;落库串行 --keyed | 12-22 秒(推断) | B4 之后 |
| B6 | WP3 Git 重放/落库/自检(4 批) | WP3;push-replay-commit-push 53.1 s FAIL、land-patch 46.4 s KILLED | 与 B5 的 devsvc-git 同域,顺序在 B5 后 | 16-24 秒(推断) | B5 之后 |
| B7 | WP5 module-units 既有拆分补丁(1 批 3 片) | 107 已合 a7b2dd86c5;余下为冷测与断言并集核对 | 无 | 已入车,补测 | 随 B2 |
| B8 | WP7 编译器/运行时跨宿主工具链(6 批) | wp7-plan.md;cx-procguard 40.1 s、unit-boundary 39.2 s、source-bom 39.1 s(commit-tier 窗) | unit-boundary 依赖 107 的 FRONT_UNIT_MAX 256 落库 | 复测后定;分片每片 <=27.5 秒 | B4 之间并行(不同文件域,注意 procguard 与 0.0.125 冲突) |
| B9 | S5 零记录 commit 批测量工具 + 三波冷态 | margin-packages.md 第 3 节;S5 459 批/381 入口 | agy-h 的 s5-measure-tool.patch 待 gatekeeper;须 UNISACC_RUN_CACHE=0 与隔离表 | 单批 <=60 秒,按波次 | B1 前先落(测量先于优化) |

### C 今晚新增的工具线件(直接落 main)

| # | 件 | 依据 | 依赖 | 预计冷态 | 车次 |
|---|---|---|---|---|---|
| C1 | 提交钩子按层跑批(设计到 MVP) | precommit-run-tier-design.md:32 个 <=10 s 批串行 11.8 s,2 路并发 | 策略表 tests/precommit-runtier.tsv + check 批;须在 107 落库后动 precommit.cx | 增量 8-15 秒,上限 30 秒(实测为底) | 工具线,107 公开后第一件之一 |
| C2 | doc-links 纳入棘轮/chain-ready | doclinks-81-investigation.md 末节(cc 已决定登记工具线缺口);+1 已由第一车修掉(plans/v0.0.126.md 行内链接改反引号并 --write,私有树复跑 result=OK broken=80) | 在第一车上复跑 --check 确认 broken=80;再在 ratchets 加 NEW_BROKEN/STALE_BASELINE | doc-links-gen 全扫耗时 UNMEASURED(未测),先测再定位置(棘轮 vs chain-ready) | 工具线,与 C1 可并行 |
| C3 | precommit 读上限(list-cap)落 107 后的后续 | cdx notes v2 补丁清单「list-cap 提交门大输出按上限具名拒绝」 | 107 落库后统一各 gate 输出上限事实源;补超限负控(toolsreg/ratchets/toolsloc) | 随既有门,无新增批(推断) | 工具线,107 后 |
| C4 | check-split-parents 贴限 | 实测 35.0 s(最近一条 batch-times:total 35014 ms),超 27.5 秒线 | 拆 --cases 或减扫描面;不得放宽时限 | 目标 <=27.5 秒 | 工具线,随 B3 |
| C5 | 预期失败表 + 零 UNKNOWN 生产者件 | cx-booter-ape 的 booter-raw-elf、tools-default-regression 的 old-checker 两处无生产者;release/evidence/<V>/expected-fail.tsv | 生产者补丁(已有 producer-tools-default-regression.patch、cx-booter-ape 材料件) | 随 B4/B1 | 工具线,107 后退 UNKNOWN 两项 |

### D 产品件(0.1.x 产品车,不与工具线混车)

| # | 件 | 依据 | 依赖 | 预计冷态 | 车次 |
|---|---|---|---|---|---|
| D1 | U33 随 0.0.125 收口(进程句柄回收后 pid 复用) | R38 判 P1 保留;state-unverified:24;0.0.125 是 sy_child_t 生命周期版 | 0.0.125 必须先发;与 WP7 cx-procguard 同文件冲突 | 夹具单跑目标 20 秒内 | 0.0.125(先于 0.1.1?见排序) |
| D2 | E02 产品键(单元缓存键纳入导入模块接口) | e02-closure.md;已决定 107 不改键、留已知红 | 需要"接口变更源码不变"负控夹具与键版本升级 | UNMEASURED,新键全量失效一次 | 0.1.2 或 0.1.3 产品车 |
| D3 | CXM_MAXU 256 的产品级验证与链接层上限 | cxm-maxu-256.patch 已入 107,只到前端/计划层;链接层仍 128 | 107 候选上 CHECK_MAXU/UNIT_BOUNDARY/129/200/256 -o 实跑 | 三批合计 1-2 分钟(推断) | 107 候选验证;若链接层要提上限,立 0.1.3 产品车 |
| D4 | B2 前端第三期(0.0.109) | R37 第 5 条豁免;B2 未开工 | agy-flow 整件;封版后第一批 | 按 109 计划(UNMEASURED) | 0.1.1 之后的独立产品车,不与诊断车混 |
| D5 | mutation gate 超时杀进程组(G38) | 0.0.125 验收;G38 判旧静态件假绿 | ompgpt mutation-gate-timeout-v2.patch + 动态断言 | 夹具单跑 <=20 秒 | 随 0.0.125 验收 |

## 3 车次与顺序建议

1. 0.0.125(U33 + 变异门)先行:它是 P1 与 G38 的既有版本槽,工具线 B8 的 procguard 与 D1 都等它。
2. 0.1.1 诊断车(A1-A5,+A6 若夹具就绪):一车一类,产品输入只动诊断合同与 exec/parse2 表。
3. 0.1.2 产品键车(D2):单独一车,升缓存键版本;不与诊断混。
4. 0.1.3 链接层/CXM 车(D3 若需提上限):单独一车。
5. 0.1.4 B2 车(D4):前端第三期整件。
6. 工具线 B1-B9/C1-C5 与上述车并行,按写域串行:登记表(tests/batches.tsv、coverage、batch-product-inputs)
   同一时刻只允许一个窗口 --keyed 落库;其余文件域可并行。

## 4 依赖图(摘要)

- B1 等 107 的 list-cap 落库(同文件 precommit.cx/check-precommit-gates.cx)。
- B8 unit-boundary 等 107 的 FRONT_UNIT_MAX 256;procguard 等 0.0.125 的 proc.cx。
- B4 wt-gate-clean 等 run-batch 材料路径二选一(改通用规则 vs 拆批)。
- C1 等 107 落库与 C3(同一 precommit.cx 文件,顺序 C3 -> C1)。
- C4/C5 都是工具线自含;可先做。
- D1 等 0.0.125;D2 独立;D4 独立;D5 随 0.0.125。

## 5 风险与回滚

- 每车一类若被工具线挤占,版本车会退化成混合车:R37 后 108 移出关键路径,允许 0.1.1 只装诊断件;
  工具线单独提交不得借版本车搭车。
- 拆批验收:断言并集等于原批 100%、无重复断言、保持预算与 serial 域、每片冷态 3 次峰值 <=27.5 秒
  (wp7-plan 验收判据);不满足就回退批次行与入口改动。
- 回滚:工具线按提交回退(tool-only 提交独立);产品车按版本分支回退;0.1.0 已公开则按发布回滚方案。
- 未测量项:doc-links-gen 全扫、E02 新键、B2、A6 基准、D3 产品级三批均为 UNMEASURED,落地前先各测一次,
  不得用推断值记 PASS。
