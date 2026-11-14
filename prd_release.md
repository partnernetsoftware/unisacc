# prd_release.md —— 发布与 CI 手册（自 0.0.48、0.0.49 发布经验归纳）

本文件是 [prd.md](prd.md) 的子文档：只写**怎么发布、CI 怎么跑、踩过什么坑**。当前版本状态见 prd.md §9.1；单次任务进度写 `agentwork/ledger.tsv`。

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

## 3. 0.0.48 的 CI 三轮教训

| 轮 | 现象 | 根因 | 处置 |
|---|---|---|---|
| 1 | 失败 | 草稿 release 缺 `slice-win-arm64` 与 `pkg.bin` | 补资产 |
| 2 | `bad or missing package footer` | `pkg.bin` 用了无尾裸正文 | 换 `compiler.p4.pkg` |
| 3 | QUALIFIED 42/0 | Mac Intel `stream-pipe` 第 2 轮超时为偶发，第 3 轮通过 | 运行 42545543777 |

规则：触发 CI 前先对照 §2 清单机查资产；偶发超时只在重跑通过时记偶发，不改时限。

## 3b. 0.0.49 发布教训

| # | 现象 | 根因 | 规则 | 证据 |
|---|---|---|---|---|
| 1 | 首轮 CI 42659892210 四个 POSIX 格 rc2（Windows 两格通过） | `tests/candidate-qualify.cx` 仍从 `limits.tsv` 读 `PRODUCT_VERSION`，F-v2 后该行已不存在 | 资格工具改读 `cxlib.releaseversion`（候选表） | 提交 `e5b26c5c1c`；发布说明「原生 CI」行 |
| 2 | `host-cell-binding`（commit 档）发布前 4 FAIL，与候选表无关 | 同类：`tests/check-host-cell.cx` 读旧版本源 | 迁到 `cxlib.releaseversion`（19/0） | 提交 `e3fc485793`（X）；`out/c49-release-runbook.md` §0 表 |
| 3 | 以上两例为同一缺陷的两次返工 | F-v2 移走版本串时只改了打包注入方，没有一次搜全所有 `PRODUCT_VERSION` 读取方 | 移走任何「第二真相源」时先 `refactor.cx sites`/`cx-lint --rules R1-version` 列全读取方，一次迁完；`build/hardwire.tsv` 登记模式 | 提交 `c66fa850e6`（F-v2）、`84df77f4b2`（hardwire.tsv）、`87016b122d`（cx-lint R1-version） |
| 4 | 提交 X 后共享检出未快进，后续补丁的集成门基线过期 | 集成者在私有克隆推了 X，共享检出仍在旧 HEAD | 发布提交 X/Y 推送后立即在共享检出 `git merge --ff-only origin/main` 并检查返回码（§6 已有规则，本版再犯） | 账本 2026-11-10 `C49-TOOLS-READY-CDX HANDOFF`（baseline 以 35cfdd1e 重算）；具体返工次数未测 |
| 5 | Win 分相 worker 静默 rc=2 | worker 模板选错：span 相必须用 `whole-win-span` 模板，不是 `whole-front` | 分相模板按相位名选，`whole-front.cx` 对模板/相位不匹配应具名拒绝 | 提交 `ba945f3238`（模板 whole-front/whole-win-span）；具体失败日志在 `out/c49-slow-cdx-979b56e4/`（本文未逐条核，未测） |
| 6 | P3 相位贴 60s（计划 §1b 记 P3 59s）、gen1 osx/x86_64 首次超时 76.558s | 每相位重复 freeze 拷贝 6.5MB 已装 APE（`release-build` 冻结 16.827s 为其中一例） | 冻结一次、各相位复用同一快照；0.0.50 G0 持久检查点 + `--resume` | `agentwork/phase-times.tsv` 第 39、54 行；计划 §1b；「每相位拷 6.5MB」的逐相位耗时未单测 |
| 7 | G0 在不同物理根复现：Linux 片 SHA 相同，Mac 合并帧差 6643B | 帧/收据依赖物理根（疑 `#line` 路径），产品产物不依赖 | cmp 判据分层：帧/收据允许依赖根，切片/内核/P4/APE 绝不允许；`build/graph.tsv` 增 `root_dependent` 列，新增 `check-root-independent` | 计划 [archive/docs-2026-11b/plans/v0.0.50-unisacc.md](archive/docs-2026-11b/plans/v0.0.50-unisacc.md) §1b |
| 8 | 与 0.0.48 镜像结构对照 28 处差异 | 源码不同，跨版本结构对照无判别力 | 等价证据只用同源小规模 oracle（中等程序默认 vs 分相镜像 cmp0）+ 两代不动点 + CI；跨版本对照只作信息记录 | 发布说明「已知限制」；计划 §1 |
| 9 | 冻结 -> 公开十余小时，计算之和约 31 分钟 | 相位间人工/代理间隙、临时设计 Mac/Win 分相方案 X、工具缺陷返工 | 0.0.50 §0：一条命令到草稿 release ≤20 分钟 | 发布说明「已知限制」；计划 §0 |

## 3c. 0.0.50 发布教训

| # | 现象 | 根因 | 规则 | 证据 |
|---|---|---|---|---|
| 1 | 已公开的 release 退回 Draft 约 5 分钟 | 改公开标签时先 `git push origin :refs/tags/旧标签`,GitHub 把失去标签的 release 视为草稿 | 改标签只用 `gh release edit TAG --tag NEW --target <完整 40 位>`(缩写提交号被拒 `target_commitish is invalid`),之后再 `--draft=false --latest`;旧标签由 GitHub 自己替换,不手删 | 账本 2026-11-10 `REL050-FALLBACK RETAG` |
| 2 | 候选链 worker.o 出现 `setpgid` libc 外部引用,六片 g1 链接不过,先被判为产品回归 | 链准备的 worker 源与 0.0.49 不同(模板 `st_includebase(0,0)` 改 `include`、编译带 `-I include`),编译器本身无差异(同一 min.cx 两版输出逐字节相同) | 判「产品回归」前先做同源最小对照;链在 wf-prepare 后断言 worker.o 外部引用只许 `sy_*`;模板按 SHA 钉入收据 | `out/c050-y0/y0-setpgid-regression.md`;items `Y0-SETPGID-REGRESSION` |
| 3 | 后备候选的记录 `commit` 该写什么 | 产品字节来自 979b56e4,打包时 main 头已前进且含未进产品的 Y-0 | 候选表/记录 `commit` 永远写真实构造源;main 头只出现在提交 X/Y 与 CI 的 `-f commit`;发布说明写清两者关系 | 0.0.50 发布说明「身份与证据」 |
| 4 | 独立资产 slice-win-arm64 没有注入工具,0.0.49 时靠临时程序 | 注入只在 ape-pack 内存里做 | `tools/version-inject.cx` + `check-version-inject`(批次 version-inject) | 提交 `114ef751be` |
| 5 | 后备路径从接手到公开 50 分钟(CI 10 分钟),主路径 Y-0 链自 11-09 起多次停摆未完成 | 停摆无人察觉、隐性前提、门后回归(0.0.51 计划 1b) | 0.0.51 第零批 W/PF/RA/LD 先于任何产品候选 | [archive/docs-2026-11b/plans/v0.0.51-unisacc.md](archive/docs-2026-11b/plans/v0.0.51-unisacc.md) 1b |

## 3d. 0.0.51 发布教训

| # | 现象 | 根因 | 规则 | 证据 |
|---|---|---|---|---|
| 1 | 打包输入 sources.tsv 写了 0.0.50 的旧源 SID e764a816,若发布则新版记录带别人的身份 | sources.tsv 由人手从已装根的纪元记录抄来,没有从构造源现算 | 身份值一律由快照现算并校验(identity-helper 对冻结快照复算);release-run --pack 的 sid 现算校验(rr-sid) | 账本 REL051;提交 X(7a964ec8b5)作废、X'(30d1c3cd67) |
| 2 | 在仓根跑 release-run --pack 静默得到 0.0.50 的 APE | ape-pack 读 ROOT/release/candidate.tsv 的 version,仓根表仍是旧版 | ROOT 必须是带目标版本表的私有克隆;--pack 开头比较 ROOT 表与 CANDIDATE_TSV 的 version(rr-ver) | 提交 bb82fe386d |
| 3 | 草稿 release 尚未创建就触发 CI,fetch 作业失败 | CI 的 fetch 从 release_tag 下载草稿资产 | 顺序:草稿创建与七资产上传 -> 提交 X -> 触发 CI;记录文件有误时取消运行再重做 | CI 运行 42809224934、42809291021 作废 |
| 4 | 补丁落地后 chain-step.cx 编不过 | 作者在集成者冻结后覆盖了同名补丁文件补全声明,落地前没核 sha 也没编译 | 门核冻结 sha 与作者路径内容一致、对涉及的 tools 编译验证(H6 ld2);交出后改补丁一律换新文件名 | 提交 65a4cd7aa3 增量修复;ld2 de419e8569 |
| 5 | 集成门屡屡 rc=124 / transport=-1,要手工反复续跑 | 门的 call() 在整体预算剩余不足时把子进程超时缩短到剩余预算(precommit 需约 48s 只剩约 25s 被杀) | 预算不足就转入 pending 由 --until-done 续跑,不缩短硬跑;--land 的 git commit 用 -F;夹具用真 git | 提交 4b919b20ea |
| 6 | 全新 rev 一条命令起链,0.15s 即死 | chain_work 在冻结前 mkdir;修复补丁只加了冻结后的 mkdir,没删冻结前的 | 凡声称「一条命令」的链必须用从未冻结过的 rev 真起一次;--bootstrap 无默认,且不安装旁边的纪元记录(ctx 记录其 sha,事后补则 REFUSE context-record-changed) | 不间断从零复现(0.0.52 A1 基线) |
| 7 | 集成门与长链争 CPU,链的 45-60s 相位超时 | 门并行 13 项棘轮把 loadavg 抬到 4-6 | 门与长链错峰;链运行期间门排队 | 账本 |
| 8 | 磁盘写满(/tmp 153GB)中断所有窗口 | /tmp 下工作目录无清理规则 | 工作目录命名带窗口与任务,完成即清;定期清理工具(待建) | 2026-11-10 |
| 9 | 决策守卫连续违规后窗口无声停摆 | 守卫把协议文本失败合成 block 并转外部等待,无通知 | 停摆必须通知集成者;chain-watch 类机制覆盖窗口 | tmp-cdx 修复线 |

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

0.0.49 实测（`agentwork/phase-times.tsv`，提交 `022caf53b1`；120 秒例外已撤销，单子进程 ≤60s、分相子相 ≤45s）：

| 项 | 实测 | 出处 |
|---|---|---|
| 131 作业波 | 276.89 s | 022caf53b1 |
| 42 网全键验证 | 7.18 s | 022caf53b1 |
| Linux gen1 whole-o | lnx/x86_64 61.338 s（内 slice 45.408 s）、lnx/arm64 61.079 s | phase-times 第 51、53 行 |
| Mac/Win 分相 | Mac Intel B 相构造 43.138 s（贴 45 s）；Win 三相（如 g3 win/x86_64 front2 6.510 s） | 发布说明「已知限制」；phase-times 第 147 行 |
| S9 提交档 | 255.13 s + 61.94 s（两 step），聚合 31.01 s | 022caf53b1 |
| 两代不动点并行端到端 | UNMEASURED | phase-times 第 11、12 行 |
| 冻结 -> 公开 | 十余小时（计算之和约 31 分钟） | 发布说明；计划 §0 |

## 6. 集成操作纪律

- 共享工作区只有集成者写 main；其他窗口出补丁，集成者在私有克隆 `cherry-pick/rebase` 后推送。
- 同步本地用 `git merge --ff-only origin/main`，**直接检查返回码**，不接 `| tail`（0.0.48 曾因此漏报合并失败）。
- s9 会话期间，被跟踪工作树文件（含 `agentwork/ledger.tsv`）变化会使整波作废：用静默窗口；0.0.49 起 child 用 `--shared-wt` wave 级共享扫描（6fccc10255），封条不符仍整 slice refused。
- 发布提交 X/Y 推送后立即快进共享检出（§3b 第 4 条）。
- 公开发布后写 `agentwork/ledger.tsv`（含撤回办法），并读回 `SHA256SUMS` 与 `--version`。
- 其他窗口的工具补丁经 `tools/integrate-patch.cx ROOT PATCH NEW_CLONE` 到 READY_TO_COMMIT 再由集成者带入（0.0.49 工具总补丁 `ba945f3238`，账本 `C49-TOOLS-READY-CDX`）。

## 7. 往期发布

现行发布说明全文:[plans/release-notes-0.0.59.md](plans/release-notes-0.0.59.md)、[plans/release-notes-0.0.61.md](plans/release-notes-0.0.61.md)(0.0.48/0.0.57/0.0.58 的身份摘要与已知缺口并入 [prd_hist.md 往期发布](prd_hist.md))。

| 版本 | 状态 | 入口 |
|---|---|---|
| 0.0.43 | 历史 | [plans/release-0.0.43.md](plans/release-0.0.43.md) |
| 0.0.45 | 公开预发布 | GitHub Release 页 v0.0.45(发布说明文件已不在 plans/) |
| 0.0.47 | 已发布 | [archive/docs-2026-11b/plans/release-notes-0.0.47.md](archive/docs-2026-11b/plans/release-notes-0.0.47.md) |
| 0.0.48 | 正式版，CI 42/0（运行 42545543777），APE sha256 `559705097427cf8f…` | [plans/release-notes-0.0.48.md](plans/release-notes-0.0.48.md);摘要见 [prd_hist.md 往期发布](prd_hist.md) |
| 0.0.48.1 | macOS 签名/公证草稿（v0.0.48.1-osx-launch-rc1，CI 42555350465 42/0，DMG 6e25e7c9…）；未作为正式版公开 | 发布说明已不在 plans/、[prd_hist.md](prd_hist.md)「0.0.49 收尾」 |
| 0.0.50 | 正式版（2026-11-10，tag `v0.0.50` 指向提交 Y `31c5886469`），CI 42/0（运行 42767415154，提交 X `114ef751be`），APE sha256 `76ed4200aaaf71b2…`；后备候选：产品字节同 0.0.49 源 `979b56e4` 六片只换版本串，Y-0 因 setpgid 回归留 0.0.51；s9 三档未跑 | GitHub Release 页 v0.0.50(发布说明文件已不在 plans/) |
| 0.0.49 | 正式版（2026-11-10，tag 指向 `35cfdd1e`），CI 42/0（运行 42661106290），APE sha256 `2d3852a8404e1ba3…`，s9 仅提交档 96/96 | GitHub Release 页 v0.0.49(发布说明文件已不在 plans/) |
