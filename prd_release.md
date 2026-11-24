# prd_release.md —— 发布与 CI 手册（自 0.0.48、0.0.49 发布经验归纳）

本文件是 [prd.md](prd.md) 的子文档：只写**怎么发布、CI 怎么跑、踩过什么坑**。当前版本状态见 prd.md §9.1；单次任务进度看会话日志,共享决定与阻塞只写 `prd_mempalace.md`。

## 1. 发布链（顺序即依赖）

| 步 | 动作 | 工具/命令 | 通过判据 |
|---|---|---|---|
| 1 | 冻结源 SID | `seed/ident ROOT`；`freeze.tsv` + `tools/precommit.cx` 冻结闸 | main 源 SID == 候选 SID |
| 2 | 候选构造 | CTX -> 131 作业 -> 42 网全键验证 -> kernel -> P4 包 -> 六个 whole-o 切片（0.0.49：Mac/Win 由 `tools/whole-front.cx` 分相，Linux 直构）；两代不动点 `chain-step fixedpoint`（发布候选强制 FULL） | 全部 rc=0；全键观察数一致；g2==g3 逐字节（0.0.49：85 对象/2 内核/6 切片/产品） |
| 3 | 候选表改版本并打包 APE | 先 Edit `release/candidate.tsv`（version、tag、commit=冻结源；ci_run_id 暂 0），再 `release-ape pack`（版本由表注入，F-v2 后不在源里）、`release-ape smoke`（含 candidate-version）；`install-ape ROOT APE SHA [--write]` 装本机并立即复测 `check-dev-product`、`phase-timing-stamps` | `--version` 正确；退出码 7 烟测；装上的 APE 版本 == 表里 version（否则三项红） |
| 4 | 提交档 s9 | `tools/all-waves.cx`（外层 `tools/bound 55`；静默窗口，期间不改被跟踪文件含 ledger） | 提交档全绿（0.0.49：96/96，两 step）；daily/release 未跑须如实披露 |
| 5 | 布局与候选表回填哈希 | `tools/release-layout.cx --candidate TABLE SOURCE NEW_OUT`，再 `--check OUT`；回填 ape/package/win_arm64/record 四列（pkg.bin 先 `tail -c16` 核 `UNIPKG1`；slice-win-arm64 须是注入版本槽后的字节） | check 通过；`sha256sum --check SHA256SUMS` |
| 6 | **提交 X** 并建草稿 release | `git commit -- release/candidate.tsv`（0.0.49：`e3fc485793`）；`gh release create TAG --draft --target <commit 列>`，资产分 2–3 批上传；`release-decide --assets DIR` 机查资产 | 资产齐全（见 §2）；X 已推送 |
| 7 | 同候选六目标原生 CI | `gh workflow run candidate-native.yml -f ape_sha256=… -f commit=<X 的 40 位> -f release_tag=…`（CI checkout `inputs.commit` 读候选表版本，必须用 X 而非冻结源） | QUALIFIED N/0（0.0.49：42661106290 42/0） |
| 8 | **提交 Y** 回填 run id，裁决，公开 | Edit 表 ci_run_id=RUN + 发布说明定稿 -> 提交 Y（0.0.49：`35cfdd1e3a`）；`release-decide --candidate … --evidence … --assets …` 须 QUALIFIED unknown=0 fail=0；先写撤回办法再 `gh release edit TAG --draft=false --prerelease=false --latest` | 读回 `SHA256SUMS` 与 `--version`；账本记 PUBLISH 行与回滚办法 |

0.0.49 实际走法（证据：`out/c49-release-runbook.md` 预演表、账本 2026-11-10 `RELEASE-0.0.49 PUBLISH_DECIDED`）：步 3 -> 4 -> 5 -> 6（X）-> 7（CI 首轮 42659892210 四格 rc2，修 `e5b26c5c` 后重跑 42661106290）-> 8（Y）。tag `v0.0.49` 指向 Y；冻结源 979b56e4 与 X/Y 源 SID 相同（版本已移出 SID）。

## 2. 草稿 release 必备资产

- APE `unisacc.com`、`SHA256SUMS`、记录/构造 zip；
- **`slice-win-arm64`**（win/arm64 切片作为独立资产）；
- **`pkg.bin` 必须是带 16 字节尾的 `compiler.p4.pkg`**（`UNIPKG1\n` + LE64 尾），不能是裸 `p3-package.body`。

## 3. 往期发布教训提炼(0.0.48 至 0.0.51;逐表原文已于 2026-11-24 迁入 prd_hist.md「prd_release.md 第 3、3b、3c、3d 节」)

下列规则提炼自四张教训表,编号供其它章节引用;现象、根因、证据与提交号以 [prd_hist.md](prd_hist.md) 逐字迁入的原表为准。

1. 触发 CI 前先对照第 2 节清单机查草稿 release 资产;偶发超时只在重跑通过时记偶发,不改时限(原 3 规则;0.0.48 三轮)。
2. 移走任何"第二真相源"(如版本串)时,先用 `refactor.cx sites` 或 `cx-lint --rules R1-version` 列全读取方,一次改完(原 3b 第 3 条)。
3. 发布提交 X、Y 推送后立即在共享检出 `git merge --ff-only origin/main` 并直接检查返回码(原 3b 第 4 条,第 6 节引用)。
4. 等价证据只用同源小规模 oracle、两代不动点与 CI;跨版本结构对照只作信息记录;cmp 判据分层:帧与收据允许依赖物理根,切片、内核、P4、APE 绝不允许(原 3b 第 7、8 条)。
5. 分相模板按相位名选;冻结一次、各相位复用同一快照(原 3b 第 5、6 条)。
6. 改公开标签只用 `gh release edit TAG --tag NEW --target <完整 40 位>`,不先删远端标签(删标签会使 release 退回 Draft;原 3c 第 1 条)。
7. 候选表与记录的 `commit` 永远写真实构造源;main 头只出现在提交 X、Y 与 CI 的 `-f commit`(原 3c 第 3 条)。
8. 身份值一律由快照现算并校验,不手抄;ROOT 必须是带目标版本表的私有克隆,`--pack` 开头比较 ROOT 表与 CANDIDATE_TSV 的 version(原 3d 第 1、2 条)。
9. 顺序:草稿创建与七资产上传 -> 提交 X -> 触发 CI;记录文件有误时取消运行再重做(原 3d 第 3 条)。
10. 集成门预算不足就转入 pending 由 `--until-done` 续跑,不缩短子进程超时;门与长链错峰;凡声称"一条命令"的链,必须用从未冻结过的 rev 真起一次(原 3d 第 5、6、7 条)。
11. 停摆必须通知集成者;工作目录命名带窗口与任务,完成即清(原 3d 第 8、9 条)。

## 4. 版本串与签名

- **0.0.49 起版本串不在源 SID 内**（提交 `c66fa850e6`，SPEED-049-F source-packages-v2）：唯一版本列为 `release/candidate.tsv`，打包时注入 32 字节槽；源 SID 为十二包摘要组合。改版本不再重建整链。读版本一律 `cxlib.releaseversion`，不再 grep `limits.tsv`。
- 0.0.48 及更早：版本串在产品输入内（`exec/facts/limits.tsv` 共 8 处、`src/version.h`、`src/drive.cx`），换版本即换源 SID，需重建整链（已由上条解耦）。
- 版本补丁历史上共 9 文件，含 `.github/workflows/candidate-native.yml` 的转义正则 `^unisacc 0\.0\.NN\r?\n$`；grep 版本要包含转义形态与 `check-productpkg`。
- Mac 切片是 ad-hoc 签名：版本串变化必牵动覆盖该页的 32 字节页哈希槽。判据：差异集 == 版本字节 ∪ 该槽，且槽 == SHA256(该页)。Linux/Win 仅版本字节。

## 5. 时限与耗时

0.0.48 实测：

| 项 | 实测 | 上限 |
|---|---|---|
| Linux whole-o | 36–41 s | 60 s |
| Win whole-o | 70–83 s | 120 s（`/usr/bin/timeout --signal=KILL 120`；`tools/bound` 只接受 1..60） |
| Mac A+B | 97–100 s（A 51–57 s） | 贴 60 s 线 |
| 候选构造总计 | 约 2 小时 | 0.0.49 目标压缩 |

0.0.49 实测（`build/phase-times.tsv`，提交 `022caf53b1`；120 秒例外已撤销，单子进程 ≤60s、分相子相 ≤45s）：

| 项 | 实测 | 出处 |
|---|---|---|
| 131 作业波 | 276.89 s | 022caf53b1 |
| 42 网全键验证 | 7.18 s | 022caf53b1 |
| Linux gen1 whole-o | lnx/x86_64 61.338 s（内 slice 45.408 s）、lnx/arm64 61.079 s | phase-times 第 51、53 行 |
| Mac/Win 分相 | Mac Intel B 相构造 43.138 s（贴 45 s）；Win 三相（如 g3 win/x86_64 front2 6.510 s） | 发布说明「已知限制」；phase-times 第 147 行 |
| S9 提交档 | 255.13 s + 61.94 s（两 step），聚合 31.01 s | 022caf53b1 |
| 两代不动点并行端到端 | UNMEASURED | phase-times 第 11、12 行 |
| 冻结 -> 公开 | 十余小时（计算之和约 31 分钟） | 发布说明；计划 §0 |

## 5b. 沿用的流程条目(2026-11-22 自 plans/v0.0.83、v0.0.84、v0.0.85、v0.0.86、v0.0.87、v0.0.88 摘出;原文逐字,前三份计划已归档到 archive/plans-pre086/,后三份已归档到 archive/plans-pre090/)

条文为原计划条款原文,一字未改;摘出只为避免随计划归档而失效。方括号内是出处:计划、节、条与原文件行号。现行承接情况另列。

[0.0.85 (d)(3),plans/v0.0.85.md 行 122] (3) 布局冒烟(预跑前 10 分钟):整链出口后先在候选布局只跑本版新增或改过的批次加清扫清单批次,全绿才发全量预跑。
[0.0.85 (d)(1),plans/v0.0.85.md 行 120] (1) 新增批次自测门:S1 落库后任何新批次不经单提交布局自测不得登记;S1 落库前新增的批次由登记人在布局内手跑一次并把 RAW 行贴进提交说明。
[0.0.85 (d)(8),plans/v0.0.85.md 行 127] (8) 时限:批次 55 秒界限不放宽;新批次自测超 40 秒即拆。
[0.0.85 (d)(7),plans/v0.0.85.md 行 126] (7) 字节线:合法程序 -o 任一变化即为缺陷;镜像变化逐格登记原因。
[0.0.85 (f)(4),plans/v0.0.85.md 行 155] (4) 诊断改动只许改变拒绝与告警路径:合法程序 -o 与 netregen 60 格对 0.0.84 公开件必须逐字节 SAME,变化即缺陷,不登记指纹(主调度裁定)。
[0.0.85 (d)(5),plans/v0.0.85.md 行 124] (5) 预跑规程:只经 S2 入库驱动发车,每窗口单独 --out,窗口间并行;不手工 --resume 同一文件;同一批补跑两次即停手定因。目标预跑 1 次。
[0.0.84 (d)(4),plans/v0.0.84.md 行 113] (4) stream-pipe 55 秒界限:不放宽;靠 S3 拆部件取余量;拆分后每版记各部件 CI 耗时,任一部件超 40 秒即登记整改。
[0.0.83 (3),plans/v0.0.83.md 行 93] (3) 推测整链复用若身份键漏掉链工具或宿主工具链身份,会复用到不同构造条件下的候选;键必须含链工具哈希与 chain-ready 九条前置的输入身份,首版强制双跑对拍。
[0.0.86 发布前硬阻塞 (C),plans/v0.0.86.md 行 117] (C) 冻结前须把 v0.0.86-fix、v0.0.86-l1、v0.0.86-w2d、v0.0.86-d2 子分支合入 v0.0.86,并向 1:cdx、1:agyneon、1:agy-flow、1:ccapi、1:cdx2、1:agy-cage 广播冻结停写令,防止 X -> Y1 -> Y2 期间有人推 main 触发 RELEASE_EXEC_GATE_MAIN_ADVANCED。
[0.0.87 起链前必须为真,plans/v0.0.87.md 行 49] 消费者清扫:候选前端编过全仓 tools 与 include,新增拒绝数为 0。
[0.0.88 四,plans/v0.0.88.md 行 62-65] 起链命令与 `UNISACC_IDENT_HELPER`:
  - 默认路径: `~/.cache/unisacc/ident-helper/identity-helper-real5`(来源: 由仓内 `seed/ident.c` 宿主编出, `chain-ready 20-ident-helper` 强制检查已设且可执行)。
  - 显式 cache-manager: `~/.cache/unisacc/release-v073/cm/x64` 与 `~/.cache/unisacc/release-v073/cm/arm64`。
  - 启动命令: `UNISACC_IDENT_HELPER=~/.cache/unisacc/ident-helper/identity-helper-real5 setsid sh ./unisacc.com tools/chain-run.cx -- --rev <REV40> --out /tmp/unisacc-session-v088-chain1/<REV40> --deadline-s 5400 --segment-s 270 --max-segments 40 --supervise -- --cache-manager-osx ~/.cache/unisacc/release-v073/cm/x64 ~/.cache/unisacc/release-v073/cm/arm64`。

现行承接核对:0.0.83 (3) 推测整链复用身份键含链工具哈希一条,现行文档与代码里的承接未核;0.0.85 (d)(1) 的单提交布局自测现由 `tools/register.cx` 的 layout 自检执行,但 40 秒拆批界限是否被它覆盖未核。
现行承接核对(0.0.86 至 0.0.88 三条):冻结停写令只有 `tools/release-exec.cx` 的 RELEASE_EXEC_GATE_MAIN_ADVANCED 门在代码里把关,停写令广播本身现行文档无承接;`UNISACC_IDENT_HELPER` 默认路径由 chain-ready 20-ident-helper 检查已设且可执行,但路径与 cache-manager 路径的文字只在此条;消费者清扫的现行承接未核。
注(cc 2026-11-22 裁定):0.0.85 (f)(4) 的"不登记指纹"适用于仅改诊断拒绝与告警路径的版本(合法程序 -o 与 netregen 对公开件必须逐字节相同,变化即缺陷),该条仍有效,不放宽;prd_mempalace.md:175 的 netregen 字节栅栏是另一类,适用于有意改镜像的版本,经 G1 expected-fail 登记精确指纹并带到期。两者并存。
## 6. 集成操作纪律

- 共享工作区只有集成者写 main；其他窗口出补丁，集成者在私有克隆 `cherry-pick/rebase` 后推送。
- 同步本地用 `git merge --ff-only origin/main`，**直接检查返回码**，不接 `| tail`（0.0.48 曾因此漏报合并失败）。
- s9 会话期间，被跟踪工作树文件（含 `archive/agentwork-retire-2026-11/ledger.tsv`）变化会使整波作废：用静默窗口；0.0.49 起 child 用 `--shared-wt` wave 级共享扫描（6fccc10255），封条不符仍整 slice refused。
- 发布提交 X/Y 推送后立即快进共享检出(本文第 3 节规则 3,原 §3b 第 4 条)。
- 公开发布后在 `prd_mempalace.md` 登记不可逆动作（含撤回办法），并读回 `SHA256SUMS` 与 `--version`。
- 其他窗口的工具补丁经 `tools/integrate-patch.cx ROOT PATCH NEW_CLONE` 到 READY_TO_COMMIT 再由集成者带入（0.0.49 工具总补丁 `ba945f3238`，账本 `C49-TOOLS-READY-CDX`）。

## 7. 往期发布

版本序、各版状态与公开日期只看 `build/version-seq.tsv`;发布证据在 `release/evidence/<版本>/`,发布说明在 `release/notes-<版本>.md`(0.0.72 及更早的已归档到 `archive/pre080/release/`)。现行近期说明:[archive/plans/release-notes-0.0.59.md](archive/plans/release-notes-0.0.59.md)、[archive/plans/release-notes-0.0.61.md](archive/plans/release-notes-0.0.61.md);0.0.48、0.0.57、0.0.58 的身份摘要与已知缺口见 [prd_hist.md 往期发布](prd_hist.md)。

0.0.43 至 0.0.50 的逐版入口表(0.0.43、0.0.45、0.0.47、0.0.48、0.0.48.1、0.0.49、0.0.50 七行,含 APE sha256、CI 运行号与提交 X、Y)已于 2026-11-24 逐字迁入 [prd_hist.md](prd_hist.md)「prd_release.md 第 7 节往期发布表」。
