# 记忆宫殿(唯一协同入口)

项目所有者 2026-11-14 裁定:agent 协同只用本文件。不再用 `agentwork/` 的任务表、账本、看板记过程;`agentwork/` 退役,现存文件只作工具读取与历史,不再追加。本文件由 prd.md §9 索引;每轮先读本文件,事实、阻塞、下一动作原位更新,旧进度删掉或改写,不追加流水。

## 写法规则

- 只写当前事实:谁在做什么、卡在哪、下一步命令。过程、重跑记录、单个批次 PASS 不写。
- 三类才写进来:别的 agent 必须遵守的决定、不可逆的对外动作、阻塞发布的未验证项。
- 每条事实带证据入口(命令或路径),过期就改,不留"曾经"。
- 速度数字只来自 tools/run-startup-bench.cx、coldcompile.cx、check-bench-gencode.cx、speed-probe.cx。

## 当前发布与版本

- 已装根:0.0.59(APE sha256 前缀 e43ef368,本机备份 ROOT/unisacc.com.059,未入库)。
- 进行中:0.0.61(合并原 0.0.60 范围,不单独发 0.0.60)。已在 main:qsort introsort、stdout 缓冲(SPEED-17)、safedel 的 chains 根白名单、sy_atspawn、cx.stream/cx.os/cx.sys 一批实现、E2 padding 补丁(directive-scan-byte 的 P3BLANK 行)、链工具修复(entry-build RT_UNIT/RT_EXTERN 按对象键选;whole-front 的 worker 以 RT_UNIT 编译;release-build 暂存目录用 sy_rmdir;tlib kit 缓存命中后置 0755)。
- 已证实(实测):私有整链 g3fix8 全相位走完,g2 与 g3 candidate 在 lnx-x86_64 和 lnx-arm64 的 sha256 逐位相同;打包产物对 0.0.59 的冷编译(coldcompile,单样本)ledger-add 9382 -> 6504 ms,entry-build 18427 -> 13607,whole-front 16556 -> 12532,coldcompile 9128 -> 6471(不含 E2 补丁)。
- 在跑:对 main 03c4d5df9d 的树的真链 real4(私有构造根 /tmp/unisacc-session-build-lr0060/src,输出 chain-real4,日志 log/real4.out)。出包后:装机(install-ape)、单跑 tsv-lint 与 tlib-prep-refuse 看 BATCH_RUN、coldcompile 实测、对比 build/speed-baseline.tsv、再发布。

## 起整链的做法(已验证的配方)

1. 私有根的工作树必须检出被构造的 REV(否则驱动工具与 REV 内工具拼法不一致,wf-prep-g2 收据 root 失配)。
2. BOOTSTRAP_FROM 要求祖先 2a46e1a4,而它不是 main 的祖先:用 `git commit-tree <main 树> -p fbdc99ca -p <main HEAD>` 造私有提交,先 `git fetch` main 到私有根。
3. `sh ./unisacc.com tools/candidate-chain.cx -- --rev REV --out 全新目录 --parallel 12`(输出目录必须全新,不能续跑;超时给 5400 s)。
4. 打包:同命令加 `--resume --cache-manager-osx X64 ARM64`(借用 0.0.59 的 osx cache-manager,Mach-O)。

## 阻塞与未验证(发布前清零或如实写进发布说明)

- 已装 0.0.59 落后于工作区:tsv-lint 在 affected 下报 `_u_so_put` 未定义(FTRIM-bootstrap 类),装新产品后应转绿,未验证。
- 约 80 个批次此前红,大半属 FTRIM-bootstrap(装新产品后复验);27 + 36 个批次因窗口 DEFERRED 未跑;osx/win 平台批次需对应主机,无证据。
- check-stdio-buffer 不覆盖 --link-only 与子集编译的缓存 identity 一致性;tty/pty 路径未测;六目标未跑。
- build/speed-baseline.tsv 未更新;run-startup-bench、check-bench-gencode、speed-probe 对新产物未跑。
- 整链 77 分钟(4625 s);g2/g3 的 objects 相按目标串行约 2480 s。fable 正在改相并发;预期节省未实跑。
- 既有红 wt-gate(5/8 断言失败,与退役和 ext.net 无关,干净工作树也红):tools/all-waves.cx 的 wave 在 `-run` 方式下要求自身可执行文件同目录有原生 `run-batch`(`executor-run-batch-required /run-batch`,tools/all-waves.cx:339),check-wt-gate 以 `sh ./unisacc.com tools/all-waves.cx` 起它,不满足。需改 check 的起法或 all-waves 的执行器发现,未做。slice-migration-gen 另有 source-missing cx/compiler/frames.cx(该文件 HEAD 未跟踪)。
- 仓库根 out/:五个工具已迁到 $TMPDIR;仍写 out/ 的:tools/gen-model-decode-material.cx(依赖 extract-p3-models 要求 ROOT/out)、tools/csih-ape.cx(csih 车道,须由其维护者迁)、tools/chain-run.cx(--out 须在 ROOT/out 之下)。

## 下一动作(按序)

1. real4 出包 -> 装机复验 -> 速度对比 -> 公开发布 0.0.61 -> 缓存与临时目录大扫除(含 ~/.cache/unisacc/tlib/kit 内 31 个无可执行位的旧镜像)。
2. 整链并发化落地并实测整链墙钟。
3. 收尾 out/ 写入点。
4. 核心库包由内往外封装(cx.stream、tlib、S13 复评);速度项:E2 之后的冷启动与生成代码两档新实测。
5. agentwork/ 归档:分批进行(分类表见提交 b10474a8f0 的说明)。第 1 批已做(devtime 工具与其检查移入 archive/agentwork-retire-2026-11/tools/)。仍有 48 个文件引用 agentwork/:第 2 批已做(patch-queue、unfreeze、state-audit 三个叶子工具与检查已归档,批次/覆盖/编译清单/pins 行已撤),第 3 批 archive-records、aw 检查、cursor-fleet 后 dag,第 4 批起改读写单表的通用工具并定表的去向,最后 git mv agentwork/。每批都要同步撤 tests/tools-compile-sources.tsv 与 prd.md 能力速查行。已知既有红:slice-migration-gen 报 source-missing cx/compiler/frames.cx(该文件 HEAD 未跟踪,与退役无关)。

## 记忆宫殿(索引)

| 房间 | 当前事实 | 入口 |
|---|---|---|
| 当前协同 | 本文件;agentwork/ 退役 | 本文件 |
| 当前发布 | 已装 0.0.59;0.0.61 进行中 | 本文件上节、prd_release.md |
| 结构总表 | 五层、六片、7 + 1 流、网、执行器、格式、事实表、构造链、测试 | prd.md §3;cx 侧 prd_cx.md |
| 新入口 | src/unisacc.cx 六层;src/core、src/runtime 指向 cx/ | plans/cx-modules.md、prd.md §5 |
| 测试入口 | `sh ./unisacc.com tests/run.cx -- --smoke/--all/--list` | prd.md §7、tests/batches.tsv、tests/coverage.tsv |
| 分层 | 术语层 T、结构层 L0-L4、模块层 cx-L0..3 | prd.md §0、plans/cx-layers.md |
| 棘轮 | 13 项,以 tools/ratchets.cx 末行为准 | tools/ratchets.cx、prd.md §2.2 |
| 事实表 | 事实 -> 生成器(带 --check)-> 产物 | exec/facts/、prd.md §2.4 |
| 构造图与相位 | build/graph.tsv、build/chain-phases.tsv | prd.md §3.7、§2.2 |
| .cx lint 与静态预检 | tools/cx-lint.cx、tools/tsv-lint.cx、tools/chain-precheck.cx | prd.md §2.2 |
| 产品全景表 | 生成块,tools/product-map.cx -- --write 重算 | prd.md §9.6 |
| 往期发布与历史 | 各版发布说明与历史索引 | prd_release.md、prd_hist.md、plans/release-notes-*.md |
| 归档 | 旧 C/Python 实现、旧 1333 项套件、历史计划与 PRD | prd.md §10、archive/ |
