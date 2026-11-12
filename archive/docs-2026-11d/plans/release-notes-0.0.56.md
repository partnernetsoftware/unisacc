# unisacc 0.0.56 -- 发布说明(未实跑一律写未跑)

发布提交 X `53293966498b104123582daf14eb7d2d5e55118d`。构造源(候选记录 commit 列)`b2c3ad8d9972b3a55304deea2aab9c5fdb5cb0c9`,源 SID 5ea8dd0c(`5ea8dd0cd9ca3fb673d2644eafbe579bae36e2457baabae14cc56df2744b7e68`)。本版产品输入有改动,整链真实重建;构造源之后 main 上只有构造链工具修复、文档归档与候选表提交,不触产品输入,故沿 0.0.53、0.0.55 先例「构造源与发布提交分离」。git 范围:`v0.0.55..5329396649`,`git log` 现算 10 条(含提交 X 与 0.0.55 公开后的账本提交)。tag 所指提交见 git(提交 Y)。

## 是什么

面向 C99 的编译器,自己写出 ELF、Mach-O、PE,可编译自身;产品形态为单文件 `unisacc.com`。0.0.56 是 0.0.55 顺延的 P1 落地版:唯一一类产品改动是产品内单元前端缓存(P1),外加它引出的一行模块名修复;其余为发布骨架、s9 批次治理与构造链工具修复。上一发布版见 GitHub Release 0.0.55 页。

## 身份与证据

| 对象 | 身份 | 说明 |
|---|---|---|
| APE `unisacc.com` | 088a31c6e73a36b4e66568b76a103670ae5b40a3091c58c3aa6c3bd8ea1b832b,6648004 字节 | `--version` 实测 `unisacc 0.0.56`;release-ape smoke ok=5 failed=0;链 pack 相未注入版本的 APE 为 fd7adaa5,同为 6648004 字节 |
| 记录 `unisacc.com.build.json` | 9eedd0720ac4e2cdbfd79acd249c82ef39a568febb27650a321ab012c855cc98 | SOURCES_SID VERIFIED 5ea8dd0c(identity-helper 对冻结快照现算) |
| `SHA256SUMS` | c4e90f6c0e864603fa72c24374d68463b2958f8d01db78506419513f4c0607e6 | 本地自校 6/6 OK,草稿读回 6/6 OK |
| 其余资产 | pkg.bin 56efea53(与 0.0.55 同字节:链产出的 P4 包 sha 未变),slice-win-arm64 0a7aed29,zip d7f80028 | 见 release/candidate.tsv |
| 链产物 | 本版整链重建,构造源 b2c3ad8d99;链内 compare:objects=same count=86,kernels=same count=2,slices=same count=6,product=same | 两轮重建、三个阻塞、人工介入见下节;不是无人值守 |
| s9 commit 档 | 公开时运行中,公开后(23:24 聚合)补记:候选 APE 088a31c6 加提交 X,selected 143,PASS 141,FAIL 2,UNKNOWN 0,DEFERRED 0;qualification NOT_FULL(12 个 osx 批本机 NOT_APPLICABLE)。FAIL 两批为 s9-nightly-run-pass(检查项 fake-run-pass、index-row)与 s9-nightly-run-fail(检查项 fail-run-fail、fail-index-report-not-dash),都是本版 E 批次治理拆出的夜跑入口自检批,失败原因未定;单独重跑 s9-nightly-run-pass 一次,58 秒内被杀未出结论 | 按集成者裁定不等 s9 公开,公开依据不含 s9;首段加两次 --resume 共 455、201、61 秒;不称 QUALIFIED |
| s9 release / daily 档 | 本版未跑 | 0.0.54 release 档首跑 15 批全域矩阵 OPEN 格 FAIL 未变 |
| 原生 CI | 同候选运行 43045967979(检出提交 X 5329396649,ape_sha256 088a31c6,release_tag v0.0.56):QUALIFIED pass=42 fail=0 not_run=0 invalid=0 | 六目标原生单元格实跑,23:12 触发,23:15 结束 |
| release-decide | 七步 PASS(1 freeze-sid、2 construct、3 ape-pack、5 layout-check、6 draft-assets、7 ci-qualified、8 readback),步 4 s9 UNKNOWN;QUALIFIED 行为 NOT_QUALIFIED unknown=1 fail=0(公开时);s9 出结果后步 4 改记 FAIL,重裁输出存 cc-work/rel056/decide2.txt | 八步原样输出存 cc-work/rel056/decide.txt |
| 六个按目标独立可执行片 | 本地产出六片,release-oneshot --pack 与 --check-pack 均 rc0,逐片 BYTE_IDENTICAL;sha 清单 cc-work/rel056/standalone.sha | 未上传到 release,未在各宿主执行;字节检查不代替宿主执行 |

## 行为变化(升级前请读)

产品输入有改动:
(1) 新增产品内单元前端缓存(P1):e2/e1 结果按单元缓存,只在 lnx/x86_64 启用,默认开。`UNISACC_RUN_CACHE=0` 全关,`UNISACC_UNIT_CACHE=0` 只关单元缓存。命中前逐项复核头文件 sha 与「仍不存在」探测;同单元同路径读到不同字节不存。
(2) `cx/runtime/routestep.cx` 的 `use modelcache` 改为 `use runtime.modelcache`,只是模块名写法统一,无运行语义变化。
(3) 其余五目标(lnx/arm64、osx 两个、win 两个)不启用单元缓存,行为同 0.0.55。

## 本版已落地(`v0.0.55..5329396649`)

产品输入(一类大改 P1 加其修复):
- `9a48b09170` P1 产品内单元前端缓存:新模块 `cx/runtime/unitcache.cx`,`src/` 五文件只留调用点(srcshape 5854 -> 5849)。键 = sha256(UNIUC3 + 产品身份 + 单元路由 + 该单元发布态 `\0cli` 全部键值 + 单元源字节 sha);头文件 sha 在 hs_file 交出字节时就地算;条目整文件 sha 覆盖计数、帧标志、键与依赖清单;回放可叠加 hs_file_note、属性记录与 e2/e1 stderr;tmp 加 rename 发布,预算淘汰与崩溃残留清理。两轮独立审查。实测 u12 换根命中 9.8s -> 6.4s(-36%),26 例 147 次命中字节等价,race/neg/stderr/concurrent/budget 各模式通过。十一批次 daily,材料 unitcache-product(无特性产品上 DEFERRED needs-materials)。
- `b2c3ad8d99` routestep 单行修复(BLOCKER-1 的修复,见下节)。验证:(1) 已装产品 c05ff40b 跑 affected --files:snapshots-cx PASS、trace-tools-only PASS、identity DEFERRED needs-materials;(2) 旧链已备 worker 对补丁快照副本跑 front osx/x86_64 0 32:rc0,32 单元全展开,无 defined twice,同命令对原快照复现原错。

构造链工具(非产品输入,构造源不变):
- `23a873aaa1` candidate-chain 的 whole-front 末片计数不再写死 21,改读已完成 front0 的 part.tsv total 推导(count = total - 64,total 不在 65..96 即拒,不重新分片;45 秒片上限不变)。BLOCKER-2 的修复。提交时真实 86 单元末片未跑;其后链续跑实测 `WF_TAIL total=86 count=22`。
- `d11b06f236` candidate-chain 的 g2_closures 调 chain-step fixedpoint 时 helper 改以 `--identity-helper HELPER` 传入(单行)。BLOCKER-3 的修复。链内实测 g2-lnx-x86_64-closure-m0.o rc0。冻结克隆里实际使用的工具文件与该提交同字节(sha256 前缀 b73746a4)。

发布工具:
- `77174a9ec6` G4 一条命令发布骨架 `tools/release-oneshot.cx`(0.0.54、0.0.55 实际发布流程编为可续跑 17 相;本版只做骨架与 dry-run,真实 git/gh/CI 相记 PLANNED,不触网)与 check。
- 同提交 L2 按目标独立可执行:`tools/cxlib/releaseoneshotpack.cx`,输出 `unisacc-{lnx,osx,win}64-{x86,arm}[.exe]`(片加 P4 包加 UNIPKG1 尾);五片与 APE 解出片逐字节相同,win/x86_64 取 APE 外层 PE 至末个 raw section;check-releaseoneshotpack 30 项。

批次治理:
- `80ec934bc3` E s9 批次治理:source-ident-helper 拆两批;check-s9-nightly-run 拆 neg/pass/fail 三模式,新增批次 s9-nightly-run-pass,去重后仍原 14 项,各 <=40s;source-job-union 登记对齐测试合同;s9-nightly 新增 `ROOT --resume RUN`(须 session.tsv,只 step/retry/aggregate,报告写 -resume.md),check-s9-nightly 20 -> 24。

文档:
- `a5f562f372` 文档归档:543 个过期文档 git mv 到 `archive/docs-2026-11/`,新增 INDEX.md,修现行文件里指向被移文件的路径(16 文件)。集成门以 smoke-too-many 拒(改名数超冒烟上限),由集成者在干净克隆复核后直接提交;未过集成门,如实记。
- `1f6384c32b` 补落 0.0.50 至 0.0.52 遗留在共享检出未提交的记录(ledger 6 行、items、phase-times、pkg-gaps、.gitignore 加 *.o),纯记录,不触产品输入。

## 整链重建、阻塞与人工介入全表

本版整链跑了两轮,出现三个非超时阻塞,累计人工介入如下。证据:`cc-work/evidence-056/MANUAL.tsv`、`cc-work/evidence-056b/MANUAL.tsv` 及两份 `steps.tsv`。

第一轮(冻结克隆 r056,REV 9a48b09170,20:08 起):

| 编号 | 相位 | 原因 | 做法与核验 |
|---|---|---|---|
| MANUAL-1 | gen1-lnx-x86_64 | 链内整包 entry-build(86 对象约 46s 加链接约 36s)单子进程超 55s;对象与 .cache 已全部落盘,完整构造不复用 .cache,故 --resume 必重超 | 同链环境手工跑 `entry-build ... --target lnx/x86_64 --link-only`(tools/bound 55),再向 checkpoint.tsv 追加 DONE;wall 36366ms;rc0;候选 sha 15879574,3748058 字节;收据头 entry-build-v3 仅 1 次 |
| MANUAL-2 | gen1-lnx-arm64 | 同 MANUAL-1 | 同上,`--target lnx/arm64 --link-only`;wall 35656ms;rc0;候选 sha 63f36ce4 |
| BLOCKER-1 | gen1-osx-x86_64 front0 | 非超时。whole-front worker 44 根整程序展开报 `cx: type mc_candidate from modelcache and runtime.modelcache is defined twice`:routestep.cx 用裸名 `use modelcache`,P1 新模块与 src 文件经 runtime.unitcache 到达 runtime.modelcache,同一单元经两个名字到达同一模块即被拒;单元级 -funit 编译 rc0,故 lnx 86 对象不受影响 | 未做绕行(需改产品源,属新 REV);20:49 停链;修复 `b2c3ad8d99`,整链从头重建 |

第二轮(冻结克隆 r056b,REV b2c3ad8d99,21:03 起):

| 编号 | 相位 | 原因 | 做法与核验 |
|---|---|---|---|
| MANUAL-1 | gen1-lnx-x86_64 | 同首轮 MANUAL-1(整包超 55s,86 对象与 86 .cache 已落盘) | 同首轮做法;wall 19832ms;rc0;候选 sha 15879574,3748058 字节,与首轮同字节 |
| MANUAL-2 | gen1-lnx-arm64 | 同上 | wall 19292ms;rc0;候选 sha 63f36ce4,3760298 字节,与首轮同字节 |
| BLOCKER-2 | gen1-osx-x86_64 merge | 非超时。front0/1/2 均 rc0,merge 报 `whole-front: REFUSE incomplete merge partition`:P1 新增单元后整程序 86 单元,candidate-chain 两处写死分片 32/32/21 = 85,whole-front 判 next != total 即拒 | 未做绕行;修复 `23a873aaa1` |
| MANUAL-3 | gen1-osx-x86_64 merge 起 | 构造工具修复,非产品绕行:冻结克隆只把 tools/candidate-chain.cx 更新为 main 23a873aaa1 版;快照与全部产品输入不动,构造源仍为 b2c3ad8d99。手册「第 3 次介入即停」由协调者决策豁免,理由是修的是已过门落地的工具缺陷 | 覆盖工具文件(修前副本留证),带 --resume 续跑 |
| MANUAL-3a | gen1-osx-x86_64 front2 | MANUAL-3 的状态修复:旧工具以 64/21 产出的 front2 片不覆盖第 86 单元,属无效材料;该路径续跑不自清 control | front2、front2.control、merged.control 三目录移到 cc-work/trash;checkpoint.tsv 删去 DONE gen1-osx-x86_64-front2 一行(修前副本留证);front0、front1 保留(分片不变);续跑日志 `WF_TAIL total=86 count=22` |
| BLOCKER-3 | g2-lnx-x86_64-closure-m0.o | 非超时。`chain-step: REFUSE unknown/duplicate option: .../ctx/identity-helper`:g2_closures 把 helper 当第 7 个位置参数传给 chain-step fixedpoint,应为 `--identity-helper HELPER`;自 53dc832999 起即如此,动态闭包相经 candidate-chain 走到此处必拒 | 修复 `d11b06f236` |
| MANUAL-4 | g2-lnx-x86_64-closure 起 | 构造工具修复,非产品绕行(与 MANUAL-3 同类):单行参数位置修正,快照与产品输入不动 | 冻结克隆工具文件更新为修复版(sha b73746a4,修前副本留证),--resume 续跑;m0.o rc0,wall 10307ms |
| CHECK-44 | lnx_obj[44] 审计风险实测 | 审计提出「g2 lnx 逐对象相写死 44 个 .o,P1 新单元可能漏」 | 实测不漏对象:gen1-lnx-x86_64 落盘 86 对象 = 44 根对象加 42 闭包对象(m0..m41);闭包清单 42 行,含 m13.o = modelcache、m14.o = unitcache(P1 新模块是闭包对象不是根,根数 44 未变);44 + 42 = 86 与 part.tsv total=86 一致;gen1 两个 lnx 候选 strings 均含 UNIUC3。无需修工具,不改产品字节 |

MANUAL-4 之后到 pack 无新增介入(MANUAL.tsv 止于 CHECK-44)。第二轮链 21:03 起,23:07 pack 完成,墙钟约 125 分钟,超过 120 分钟线;加上作废的第一轮 41 分钟,构造共约 166 分钟。

发布打包阶段的人工处理(cc-work/rel056/pack.log、pack2.log):
- release-run --pack 首跑停在 layout-gen:`RELEASE_LAYOUT REFUSE ape: sha256sum output or digest mismatch`。原因是 release-layout 先按候选表核 sha,而初表的 sha 列还不是本次产物的值。处理:用同一工具对已暂存的四件做一次探测生成(输出在 cc-work/rel056/layout-probe,不作发布物),取七件实际 sha 填表后整条 --pack 重跑通过;重跑的 APE 与首跑同 sha(088a31c6)。表内七个 sha 全部来自文件现算,回填表与提交 X 的表逐字节相同。
- release-run 没有 --resume 入口(发布手册写有,实际报 usage),所以是整条重跑而非续跑。

如实说明:
- 第二轮链产物是在同一冻结克隆里先后用三个版本的 tools/candidate-chain.cx 产出的(原版、23a873aaa1 版、d11b06f236 版);两次更换都只改构造工具,不改快照与产品输入,但这不是「一条命令无人值守」的链。
- 第二轮 gen1 的 lnx 两目标候选 sha 与第一轮相同(MANUAL 行实测记录;两轮构造源只差 routestep 一行模块名写法)。
- 链内强制 `UNISACC_UNIT_CACHE=0`(带守卫断言),即构造过程不使用 P1 缓存;P1 缓存的正确性证据来自提交 9a48b09170 的专项批次,不来自整链。

## 已知缺口(如实)

- P1 单元缓存只在 lnx/x86_64 启用,其余五目标未启用,未测收益;u12 的 -36% 是单场景实测,不是全量基准。
- P1 提交门缺 whole-front 整程序展开检查,漏检 routestep 的重复模块名(BLOCKER-1),代价是整链重建一轮;门内左移检查(wfcheck)尚未落地,顺延 0.0.57。
- 构造链工具仍有写死计数未治理(只读审计 cc-work/hardnum-audit.md):whole-front 分片偏移 0/32/64 与每片 32 仍写死,容量上限 96 单元,当前 86,再增 10 个以上单元会再次断链;gen1 osx-x86_64 另有一份重复的手写分区;根数 44 写死十余处。本版只修了末片计数与 helper 参数两处。
- gen1-lnx 的 entry-build 整包超 55 秒,两轮共 4 次手工 `--link-only`;对象相与链接相拆分(gen1split)未落地,顺延 0.0.57。
- 基线红未在本版处理(产品输入,留 0.0.57 的 P57):module-units 的 `UNIT_LIMIT violations=1`(`src/runcache.cx:629` bare unit limit);cx-layers 的 `FAIL UP cx/runtime/holdframe.cx:5 use compiler.frames`。两项在干净 main 80ec934bc3 独立克隆实跑同样 FAIL,与本版改动无关。
- 门内批次 candidate-chain 在补丁树与干净 main 上均 killed-at-deadline 53.9s,属基线超时,集成者显式放行,未记 PASS。
- 顺延未入本版:G1 门本体(D2/D4/D5 main-health)、G2、G3、D6 槽位仲裁、ib55 三件、t57-1(runnerprep 冷编)、D1 selfcheck 03+04。
- release-oneshot 的真实 git/gh/CI 相均为 PLANNED;本版发布仍按手册逐步人工执行,发布尾部并行由人工编排。
- 六个按目标独立可执行片未上传、未在各宿主执行;候选表仍是七资产。
- s9 commit 档在公开时尚未出结果(运行中),公开不以 s9 为依据;公开后补记为 141/143,两批夜跑入口自检 FAIL 原因未查明,未修;s9 克隆里 install-ape 重写了 plans/root-identity.md 的身份块,该克隆工作区与提交 X 相差这一份文档;release 档与 daily 档未跑;全域矩阵 release 档 15 红未回填。s9-nightly 的 `--resume` 在真实会话上本版首次使用。
- 沿 0.0.55 说明、本版未处理也未复核:BASE6 字节 oracle 未重建;p3 的 28 个 release-ctor case 材料未重建(DEFERRED),未记 PASS。
- 整版超 2 小时铁律:P1 落地 20:05,pack 23:07,公开在其后(约 3 小时以上);超时原因是缺陷晚发现与整链无增量复用,已登记整改(cc-work/plan-forward-057.md,0.0.57 零接触链)。
- 两代不动点、跨平台原生资格、s9 与 CI 结论以上表实填为准;表中仍是占位或写未跑的项,不称 QUALIFIED。

## 撤回办法(公开前写定)

`gh -R partnernetsoftware/unisacc release edit v0.0.56 --draft=true` 撤为草稿;`gh -R partnernetsoftware/unisacc release edit v0.0.55 --latest` 恢复上一版 Latest;tag 不删。彻底撤回才 `gh release delete v0.0.56 --cleanup-tag` 并在账本记 REL056-WITHDRAWN。本机根回滚:install-ape 装回 v0.0.55 公开 APE c05ff40bf99de9b1ece8310ecc2d7c5bb83f7a4ccf620162ca46c26a1e851fde。
