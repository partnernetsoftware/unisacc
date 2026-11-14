# unisacc 0.0.54 -- 发布说明

发布提交 X `78e9ca46abbeef5f162a6cba3d0be551ca0e93b9`,构造源(冻结)提交 `0add718ff515efb02b4fe05f8028e92d9cd1a284`,候选记录 `unisacc.com.build.json feb64ec5bec8ce234cba4ff611d47ac4ba14875836122852fc4e1534ad54f0cf`。git 范围:`v0.0.53..78e9ca46ab`(`v0.0.53..origin/main` 共 20 条提交)。

## 是什么

面向 C99 的编译器,自己写出 ELF、Mach-O、PE,可编译自身;产品形态为单文件 `unisacc.com`。0.0.54 是纯工具版:产品输入相对 0.0.53 零改动(账本 VER054-PURE-TOOLS),主题为梳理加提速。上一发布版见 `plans/release-notes-0.0.53.md`。0.0.55 的主题为 cx 流网设计实现(账本 VER054-055-SPLIT)。

## 身份与证据

| 对象 | 身份 | 说明 |
|---|---|---|
| APE `unisacc.com` | sha256 `0ca238092e803b82bc30b3dffbea3dfa29e2bb7c4df903afe8d755feade419ef`,6529460 字节 | `--version` 输出 `unisacc 0.0.54`。比 0.0.53(6529476 字节)小 16 字节:版本槽写入片后,gzip 成员长度差 1 字节并伴随填充变化,各片解出大小相同(com-tree 已核) |
| 源 SID(sources_sha256) | `4001d3e26a177be391571b9341665af1c67af1dc27c121069396fb29a13652fd`(短 4001d3e2) | SOURCES_SID VERIFIED,与 0.0.52、0.0.53 相同 |
| 链产物 | 未复用 0.0.52/0.0.53 链产物,整链从干净状态在冻结树 0add718ff5 上重跑 | 两代不动点:gen1/g2/g3 四个 lnx 目标键、osx/win 各片均完成,compare 结果 objects 85、kernels 2、slices 6、product 全 same;lnx 候选 g2 与 gen1、g3 与 g2 同 sha(x86_64 `e9d18ef4...`,arm64 `39737e45...`) |
| 复现性 | 未注入版本前 APE 与 0.0.53 同字节(`0842748d`) | 证明冻结树干净重跑可逐字节复现 0.0.53 产品 |
| P4 包 `pkg.bin` | `56efea531e104b4114b314d5bdb4e5efc2a6debb96f974fa5d6e32675a637db8`,1763636 字节 | 与 compare 的 package 一致 |
| `slice-win-arm64` | `24c3f83712980d7c6d09370827927842d3186fb6eceb5802ec4e491392eabe72`,3485184 字节 | |
| `unisacc-0.0.54-win-arm64.zip` | `30e47116456bbb87964ff347e470428117018beb437181a703247ea84d39d94f` | |
| 记录 `unisacc.com.build.json`(含别名 `build.json`) | `feb64ec5bec8ce234cba4ff611d47ac4ba14875836122852fc4e1534ad54f0cf` | 记录内 commit 为冻结源 0add718ff5,product_digest 等于 APE sha256 |
| `SHA256SUMS` | `7dfe1389bc37a107c06f6f279db2c593310c2ce9907c4a4678b8517f542ee1df` | 自校 6/6 通过 |
| s9 | commit 档(候选 APE 0ca23809):selected 132,PASS 130,FAIL 0,UNKNOWN 2(source-ident-helper 55s 看门狗、s9-nightly-run),NOT_FULL;release 档首跑:15 批全 FAIL,均为 full-* 全域矩阵存在 OPEN 格(如 full-stream-e1 required 44、satisfied 39、open 5,OPEN 格要求 lnx/arm64、osx、win 原生宿主,本机不可关闭),非产品回归;daily 档首跑结果见账本后续行 | 未称 QUALIFIED |
| 原生 CI | 同候选运行 42976232937(检出提交 X 78e9ca46ab,ape_sha256 0ca23809):QUALIFIED pass=42 fail=0 not_run=0 invalid=0 | 六目标原生单元格实跑 |

七资产:APE、pkg.bin、slice-win-arm64、unisacc.com.build.json、build.json(同内容别名)、win-arm64 zip、SHA256SUMS;sha 由 `release-run --pack` 现算。

## 行为变化(升级前请读)

产品输入零改动:编译器、运行库、驱动的语义相对 0.0.53 不变。本版变更全部在工具、检查、构造链辅助与文档侧。

## 本版已落地(git log `v0.0.53..origin/main`,共 20 条)

- `78e9ca46ab` 提交 X:`release/candidate.tsv` 记候选 v0.0.54(构造源 0add718ff5,七资产 sha 现算,ci_run_id=0 待 CI)。
- `874386765d` 账本 FREEZE-054:冻结于 0add718ff5(纯工具版,入版与顺延清单)。账本类提交(此条、`59555223f0`、`ed53148fdd`)合并概述:记录纯工具版裁定、四条违例、0.0.54/0.0.55 分工与冻结。
- `0add718ff5` entry-build `--link-only` 纯链接:逐个核现有 closure 对象身份(源、头闭包、编译器、旗标键、对象 sha),全一致才跳重编,任一不符真实重编或具名拒绝;负例覆盖对象缺失、对象 sha 改、源变更。g2 末相纯链接 27.686s -> 18.555s(x86)/18.283s(arm),两 candidate 与历史 raw 同 SHA。
- `ada4fbf518` source-package verify-jobs 与 capture 路径规范化:新增 `verify-jobs ROOT CTX IDS_FILE` 单次 40s 联合端点;baseline capture 对合法 `seed/../` 路径先规范化再守根边界,越根与 symlink 逃逸仍拒(11 项正负例)。
- `28ff396e15` source-package adopt-job:按网身份接纳未变产物,记为 REUSED-not-executed,不伪造 command.status;全部校验后原子 rename 发布,失败零残留;四类负例。
- `2b35ca5c37` 集成门棘轮缓存行为负例与棘轮读审计:check-integrate-patch 新增 7 批次;新增 tools/ratchet-read-audit.cx 与 ratchet-inputs.tsv,13 棘轮终态 13 SAME。
- `03a756ab7d` check-root-identity 参数按选项名解析,缺值或未知选项 die;调用方兼容。
- `44fa385fc1` 根身份记录同步:install-ape 在候选记录与 APE 匹配时整份替换记录,否则同步三字段并报 RECORD_STALE;check-root-identity 新增四项(缺 helper 则 NOT_RUN);5 处无界 sy_run 改有界。
- `4b802de6b8` pins 锚点修正:s9-shards 零值字面量锚改 :262,check-pins 恢复通过。
- `7d27128e4f` 本机根换 0.0.53:install-ape 装 APE 0842748d,根旁记录改用发布资产 build.json。
- `a88ba2d159` s9 commit 档修复(0.0.53 遗留 2 FAIL 2 UNKNOWN):host-cell-binding 夹具删去 build.json sidecar;check-s9-nightly-run 拆 fail 模式并新增批次;check-real-input-check 工具只编一次;check-s9-shards identity 控制拆分。断言均不删,时限未放宽。
- `59555223f0` 账本 VER054-PURE-TOOLS 与 RULE-VIOLATION-054R1(四条违例)。
- `826a5e9a97` 单元前端缓存键实测:新增 tools/check-cx-unitkey.cx(批次 cx-unitkey);nunit/RT-SHARED、hs_file_note、模型/目标/ABI 标 UNMEASURED。
- `98e30b1bea` 集成门基线棘轮缓存:缓存基线侧棘轮测量输出(键为基线树 id 加产品摘要),命中只现跑 head 侧;缓存的是测量值不是 PASS,14 门不少。
- `3b9054ef23` cx 相位测量与单元前端缓存合同:新增 tools/cx-phase.cx、check-cx-phase、check-cx-unitcache(17 项合同夹具)与 plans/unit-frontcache.md。这是合同与夹具,不是产品内缓存。
- `1be61bd4ef` build-cxcc-host 引导修复:建出 gcc_cx/clang_cx 驱动;tests/cx-host-run.cx(171 PASS)与 cx/check_memory、cx/check_owned 实跑通过。
- `544eaa2dad` stages 单流材料更新:check-stages 红 13 项清零(克隆实跑 failed=0 uncovered=0),断言不变。
- `e5e052d752` 新增 tools/check-s9-worker-smoke.cx(批次 s9-worker-smoke,daily):s9-shards 片工作进程分派冒烟;daily 档门内跳过,门克隆内直跑 failed=0。
- `ed53148fdd` 账本 VER054-055-SPLIT(见上合并条)。
- `939ef9fe7f` prd 系列梳理:prd.md 第 3 节改为结构点唯一权威定义;新增 prd_cx.md;product-map 按 HEAD 重算。

## 整链重跑与人工介入

整链从干净状态在冻结树上重跑(检查点 `~/cc-work/out054/checkpoint.tsv`,日志 `~/cc-work/out054.log`)。这不是一条命令过链(0.0.54 目标 A1 未达成),其间有 9 条人工介入(`~/cc-work/evidence-054/MANUAL.tsv`):

- MANUAL-1/2:gen1 lnx x86_64 与 arm64 用 `entry-build ... --o1-drivers --link-only` 手工带 Root41-bootstrap 作 CTOR 在 bound 55 下运行(36.6s/36.0s),产物 sha 分别 e9d18ef4(3706794B)、39737e45(3714938B),以 sha 记入检查点核身份。
- MANUAL-3:link-only 在 gen1 lnx 两个 receipt.txt 追加第二个头(entry-build-v3/root/p3/p3_sha),使 whole-front 报 receipt root mismatch;删去追加段 4 行重复头,原文保留为 receipt.txt.raw-appended。
- MANUAL-4/5:g2 lnx 两目标改为逐键分波(`chain-step fixedpoint --object KEY` x85,再 `--resume --link-only`,经 fpslice.sh);键用时约 1844s/1924s,链接约 41.6s/39.6s;候选 sha 与 gen1 相同,`--resume` 无 `--object` 时 reused=1 rc0。
- MANUAL-6/7:g3 lnx 两目标同法(键约 2449s/2450s,链接约 41.1s/40.5s);候选与 g2 相同,reused=1 rc0。
- MANUAL-8:试图规整 g2 lnx 的 86 个收据头失败(规整会破坏 chain-step resume 身份,因 artifact_sha 覆盖收据),已按 raw-appended 逐字节还原,规整副本留作 receipt.txt.normalized-try。
- MANUAL-9:whole-front 的 wf-prep/wf-span-prep(g2、g3)以 g2/g3 candidate 作 CTOR、gen1 对象为 OBJECTS(85 个 .o 与 g2/g3 目录 sha 相同,见 wf-prep-inputs.tsv),4 次 prepare rc0 约 13.6s;备选方案 8(带 g2 ctor 全量 entry-build)55s 超时,未重试。

0.0.55 整改项(原因):链每次重写 Root41-bootstrap 致约 22s 冷编;`--link-only` 追加收据头;逐键多头收据与 whole-front 不兼容;chain-step 无批量键;cx_rmtree 白名单。

各相实测耗时:ctx 51.6s、routes 32.0s、p3 45.0s、kernels-g2 35.9s、kernels-g3 10.9s、osx g2/g3 约 127s/片、win 约 100s/片、linux g2 逐键约 1885-1964s/片(g3 约 2490s)、compare 3.6s、pack 28.3s。

## 冻结验证

- identity 批次 PASS(22s,312/0);根身份 13/0/0。
- source-job-union、source-recorded-jobs、origin-package 仍未通过或未跑(登记与测试参数合同不一致、需旧源 ctx、缺 4 个材料),属 0.0.53 遗留。

## s9

- commit 档:132 选中,130 PASS,0 FAIL,2 UNKNOWN(source-ident-helper 在并行 wave 下 55s 看门狗超时,系 adopt 补丁新增四类负例使检查变重,0.0.55 D7 拆分;s9-nightly-run 未出结论,0.0.55 查明),qualification NOT_FULL。相比 0.0.53(124/128,FAIL 2)两项 FAIL 已在真实 s9 环境转 PASS。
- release 档首跑(0.0.51-0.0.53 均未跑):15 批 FAIL,全部为 full-* 全域矩阵(stream-e1/e3/e4/prune/lower/elf、negative、optimization、formats、hosts、bootstrap、interfaces、ri、properties、resources),原因是矩阵中需要 lnx/arm64、osx、win 原生宿主的格为 OPEN(NEEDS_FOREIGN_HOST,禁止模拟),本机无法关闭;同候选原生证据在 CI 42976232937,但尚无把 CI 证据回填全域矩阵的接线(0.0.55 立项)。不称 QUALIFIED。
- daily 档首跑:发布时仍在运行,红单完成后追记账本,不作为本版资格。
- 上述未填项未实跑前不称 QUALIFIED。

## 已知限制与未声称的性质

- T3 主补丁(相位表、增量网接进候选链、g2 拆分接线)顺延 0.0.55;本版仅有 adopt-job、verify-jobs 与 `--link-only` 基础件,未接入候选链。
- p3 细相、chain-step 整改顺延 0.0.55;S3 gcc_cx/clang_cx HOSTCX 批次未入版(wt-gate 基线红阻塞)。
- 产品内单元前端缓存顺延 0.0.55:缓存键不全(见 `826a5e9a97` 的 UNMEASURED 项);本版只有合同夹具与键实测。
- wt-gate 与 slice-inventory 在干净 main 上基线红(slice-inventory killed-at-deadline,53.9s),修复补丁待 0.0.55。
- BASE6 新 42 网字节 oracle 因 /tmp 清理丢失,未重建。
- 0.0.52/0.0.53 本地发布证据目录丢失(公开资产仍在 GitHub),本地无法复核其原始证据。
- 整链不是一条命令完成(见上节人工介入);未声明 A1 达成。
- cx 流网设计实现在 0.0.55,本版不含。

## 未跑范围

s9 daily/release 档与原生 CI 见上文待填项,未填一律视为未跑;source-job-union、source-recorded-jobs、origin-package 未通过或未跑(见冻结验证)。

## 验证入口

1. 核哈希:`sha256sum unisacc.com` 应为 `0ca238092e803b82bc30b3dffbea3dfa29e2bb7c4df903afe8d755feade419ef`(6529460 字节);`SHA256SUMS` 自校 6/6 已实测。
2. `sh ./unisacc.com --version` 应输出 `unisacc 0.0.54`(出包时已核)。
3. 冒烟:发布后下载件的冒烟未跑。
4. 复现:`release-run --pack` 已实跑,七资产 sha 现算并写入 `release/candidate.tsv`。
5. 原生 CI:https://github.com/partnernetsoftware/unisacc/actions/runs/42976232937(QUALIFIED pass=42 fail=0)。
