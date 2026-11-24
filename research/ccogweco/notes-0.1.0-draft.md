# unisacc 0.1.0 发布说明(草案)

整体裁决 UNMEASURED。本页是封版说明草案:七条准入逐条给当前判定与证据入口,候选数据(冻结提交、APE sha、
strict 汇总、CI run、起链到公开墙钟)在 0.1.0 封版车上最后填。未测一律写 UNMEASURED,不写成通过;
豁免只按 R37 已裁定条目写,并保留原始计数。

## 真实身份(封版车填)

| 对象 | 身份 |
|---|---|
| 冻结提交及提交 X | 待填 |
| 发布 APE SHA256(unisacc.com) | 待填 |
| 源快照 SID | 待填 |
| pkg.bin / 六目标切片 | 待填 |
| unisacc.com.build.json / build.json | 待填 |
| SHA256SUMS | 待填 |
| CI run ID、attempt 与 Y1 | 待填 |

## 准入七条逐条(R37 读法,R38 计数口径)

| # | 条件 | 当前判定 | 依据与原行 | 证据路径 | 封版前还缺什么 |
|---|---|---|---|---|---|
| 1 | 六目标自举不动点连续 3 版 | 部分满足(106 已归档;107/0.1.0 待) | 106 已实测并归档:release/evidence/0.0.106/ape-receipt.txt 六切片、fixedpoint-6targets.tsv 六行 SAME、INDEX 两行、notes 原句;链 compare `slices=same count=6`、`objects=same count=102`、`kernels=same count=2`、`product=same`(chain-run-249.stdout:161-164) | release/evidence/0.0.106/;research/ccogweco/fp-106-extraction.md | 107 与 0.1.0 两趟各自归档同四项 |
| 2 | CI 六格 QUALIFIED 且严格证据 FAIL/UNKNOWN 为 0 | 部分满足(106 strict FAIL=0;CI 六格沿用 0.0.105 记录) | 106 预跑原 merge(503 批)为 PASS=495 FAIL=6 UNKNOWN=2;同候选后补七条原 FAIL 行后合并 SUMMARY 为 batches=505 PASS=503 FAIL=0 UNKNOWN=2 INVALID=0;UNKNOWN=2 为已登记无生产者(cx-booter-ape、tools-default-regression,0.0.105 同形);CI 六格 QUALIFIED 引 0.0.105 ci-45347740771.log(pass=42 fail=0) | release/evidence/0.0.106/strict.tsv、expected-unmeasured.tsv;release/evidence/0.0.105/ci-45347740771.log | 107/0.1.0 各自 strict FAIL=0 与当版 CI QUALIFIED |
| 3 | 零已知 P0 | 未测(待 107 候选实跑) | 账本 5 行:2 个 P0 由 107 F1 关闭(P0-126、P0-F1);P0-chain-step-reaped-kill 已随 107 追加集(chain-step 修复与夹具),待候选实跑关闭;U33 判 P1 随 0.0.125;POT-f0r 潜在非 P0 | research/ccogweco/p0-ledger.tsv;research/2026-11-24/R38-ruling.md 第 1 节 | 107 候选上 F1 与 chain-step 的实跑原行;账本关行 |
| 4 | state-unverified 不超过 3 行(只计声称类) | 不满足(预期 3 到 4) | 表内 23 个数据行(22 UNVERIFIED + U33 OPEN);声称类基数 10 = U05/U06/U08/U09/U11/U30/U33/U21/E01/E02;缺口与债 15 行不计数 | build/state-unverified.tsv;research/ccogweco/unverified-classification.md;seal-0.1.0-evidence-map.md 条 4 三值 | 三值见下节;E02 保留则多为 4 行,如实记未满足 |
| 5 | 前端覆盖三期完成 | 不满足,豁免(仅本条) | A 在 0.0.100;B1 在 0.0.107 只有部分 2/3 与 3/3 引擎行与 module-units/cxmod-tri 分片,完整重放待候选;B2=0.0.109 未开工 | R37 第 5 条(b)(c)(research/2026-11-24/R37-seal.md:64-66);plans/v0.0.107.md、plans/v0.0.109.md | B1 四批实跑(c99-tentative-array-incomplete、c99-array-param-typedef、c99-extern-array-incomplete、c99-global-redecl-conflict);B2 顺延写进说明 |
| 6 | 整链起到公开连续 3 版不超过 120 分钟 | 不满足(原文) / 按 R6 重述待第三样本 | 原文口径 98=96.5 分钟 PASS,100=175.5、101=163.7、105=209.9 均超;R6 重述只取封版车"起链到 READY_TO_EXECUTE"为第三样本,106/107 也须在 120 内才够三样本 | research/agy-h/chain-to-public.tsv;R37 第 6 条(:69-73,须 fable 裁) | 106/107/0.1.0 的起链到 READY_TO_EXECUTE 墙钟 |
| 7 | 能用清单 30 过 25 | 未冻结(草案 29/30 就绪) | 30 项已按类配额(8/8/6/4/4)绑定现有批次;29 项就绪,唯一保留缺口是 release-chain 批本身(不换项);清单未冻结,strict.tsv 批次行机械映射未出 | research/ccogweco/usable-list-draft.md;R37 第 7 条 | 107 公开前冻结清单并勾 strict 批次行;release-chain 缺口登记 0.1.x |

结论:七条里当前无一条可写"满足"。第 1、3 条不可豁免(R37 :7):1 要靠三趟归档,3 要靠 107 候选实跑;
第 5 条按已授权豁免明写未满足;B2 顺延 0.1.x 首批。若 1 或 3 为假,当趟不叫 0.1.0,叫 0.0.127。

## R38 声称类三值与最终计数口径

口径(不搬账、不改 kind):基数 10;U05(须 macOS 实跑)与 U33(P1,随 0.0.125)保留;E02 定为已知缺口保留;
缺口与债 15 行不计数但保留行;U21 从严计入。

| 三值 | 计数 | 组成 | 条 4 |
|---|---|---|---|
| 最好 | 3 | U05、U33、E02 保留;U06 定因关闭,U08/U11/E01/U09/U30/U21 全关 | 满足 |
| 预期 | 3 到 4 | 同上;U06 多半无从定因,加入即 4 | 4 则未满足 |
| 最差 | 8 | 仅 U08、U11 关闭;U05、U33、E02、U06、U09、U30、U21、E01 留下 | 未满足 |

封版时以当趟实际关行数写最终值,并逐行给出关闭证据(U08 负控、U11 复审、E01 正负控、U30 107 真实五步原行、
U09 affected-iface2 重跑、U21 分片端到端)。未实跑的行不得记关闭。

## P0 账本与 U33

| id | 等级 | 状态 | 关闭车/证据 |
|---|---|---|---|
| P0-126 | P0 | 107 F1 具名拒绝后关闭(待候选实跑) | plans/v0.0.126.md 根因节;R37:51 |
| P0-F1 | P0 | 同上,是 P0-126 的修复件 | R37:53(d)、:90 |
| P0-chain-step-reaped-kill | P0 | 107 追加集第七笔,补丁+夹具+候选实跑后关闭 | u33-callsite-audit.md;R38 升级条件 |
| P1-U33 | P1 | 保留 OPEN,计入声称类,随 0.0.125 | build/state-unverified.tsv:24;R38 第 1 节 |
| POT-f0r | 潜在(非 P0) | 不修,不并入在途版本 | prd_mempalace.md:44;R37:51 |

## 条 6 实测分钟(agy-h 从链日志与 INDEX 抽)

| 版本 | 起链->公开 | 原文口径 | 备注 |
|---|---|---|---|
| 0.0.98 | 96.5 | PASS | 复用 ci-early 45167911448 |
| 0.0.100 | 175.5 | OVER_120M | CI 首轮超时重派 |
| 0.0.101 | 163.7 | OVER_120M | 链后合 main 形成 F 耗时 107.2 分钟 |
| 0.0.105 | 209.9 | OVER_120M | 链止到 X 161.0 分钟(prd_mempalace:43 笔误为到 F) |
| 0.0.106 | 起链到预跑完 157.2(到公开待填) | 公开后按原文口径填 | 21:31:05 起链,21:58:56 链止(27.9 分);预跑 00:08:17 完成,链起到预跑完 157.2 分钟;公开时刻待填 |

原文条款要求连续三版 ≤120,历史只有 98 一版达标;按 R6 重述(第三样本取封版车到 READY_TO_EXECUTE)仍需
106、107、0.1.0 三样本,当前不足,故本条如实记未满足并注明样本不足。

## 六目标不动点三版归档进度(106/107/0.1.0)

- 106:已实测并归档。链目录 bg-b1/2e8a19a342 的 `CHAIN fixedpoint slices=same count=6`、`objects=same count=102`、`kernels=same count=2`、`product=same`、`package=present sha=50c2333e...`;六目标 g2/g3 终片逐对 sha 相同(lnx x86_64/arm64、osx x86_64/arm64、win x86_64/arm64);归档四项已在 release/evidence/0.0.106/:ape-receipt.txt(六切片 path/sha/bytes)、fixedpoint-6targets.tsv(六行 SAME)、INDEX.tsv、notes 原句。
- 107:UNMEASURED,待其链的 fixedpoint 相位与 apex 回执。
- 0.1.0:UNMEASURED,待封版车自身链。
- 历史参考(不计入三版):0.0.100/notes:26 与 0.0.105/notes:28 的"固定点"句只有两代 gen1/整链 compare 四同,
  无 six-target 回执行,按 R37 读法不替代归档证据。

## 107 追加集与 0.1.0 零变化槽边界

- 107 追加集(cc 记十二份):WP4 材料、WP4 install-ape、两个无生产者(cx-booter-ape 的 booter-raw-elf、tools-default-regression 的 old-checker)、选择器复用、提交门 list-cap、U08 源缺失负控、ompgpt 夹具修复、seal-precheck、P0 chain-step,以及第一车合入后的材料宏检查;逐笔 tools-only,预跑完成后、第一车公开前落车。
- 0.1.0 是零代码生成变化的封版槽:只装封版机械工具(seal-precheck)与证据,不装材料/拆批补丁栈;材料栈(B1-B9)与工具线 C1-C5 在 0.1.0 之后落 main。
- 边界:107 的追加集不进 0.1.0;0.1.0 失败时不改叫 0.0.127(R37),两者证据目录与 strict 表各自独立。

## E02 已知缺口(产品键不改)

单元缓存键未纳入导入模块接口:结构体扩字段等接口布局变动后,旧对象可能被复用。决定不改产品键,只加检查例
作已知红复现证据、不入常规批次;按已知缺口写入 0.0.107 与 0.1.0 说明。证据:research/ccogweco/e02-closure.md、
research/ccogweco/unverified-extra-rows.md。

## B2 顺延

前端第三期 B2(0.0.109)未开工,24 小时封版窗口内不做。按 R37 第 5 条(c) 已授权豁免,本条明写"未满足,
B2 顺延 0.1.x 首批",并按记忆宫殿登记所有者需求顺延。回滚:不发 0.1.0,当趟叫 0.0.127。

## 已知缺口汇总

(1) 第 1、3 条依赖 106/107 归档与候选实跑;未测前整体裁决 UNMEASURED。
(2) 第 2 条 strict FAIL/UNKNOWN 以当次 strict.tsv 汇总为准;UNKNOWN 两项无生产者(cx-booter-ape 的
booter-raw-elf、old-checker)按已登记项处置,不冒充 PASS。
(3) 第 4 条按上表三值,若为 4 行如实记未满足,不改 kind、不搬账。
(4) 第 6 条按原文记未满足;R6 重述须 fable 裁。
(5) E02、U33(P1)、余量包里 release-chain 冷态 KILLED 68 秒与未拆批次在 0.1.x 收口。

## 公开前须再核(封版车)

1. 106 六目标回执四件归档(ape-receipt 12 行、fixedpoint-6targets 六行、INDEX 两行、notes 原句),107 同口径。
2. 106/107/0.1.0 strict.tsv 汇总 FAIL=0 与 UNKNOWN 处置;各版 CI QUALIFIED。
3. 声称类最终计数与逐行关闭证据。
4. chain-step P0 关闭原行入账。
5. 能用清单冻结(30 项、29 就绪、release-chain 缺口登记 0.1.x)。

## 回滚方案(封版车填具体值)

(1) 把 v0.1.0 release 转回 draft,Latest 指回已核前一公开版并读回。
(2) 按发布分支状态前向 revert Y2/Y1/X,保留 tag 与 release。
(3) 从已核前一版备份回装本机 Root,复核版本与 APE SHA。
