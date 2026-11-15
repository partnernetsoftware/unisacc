# 记忆宫殿(地图)

本文件是地图,不是账本。每个房间只放一句现状和证据入口;过程、取证、采样、重跑记录不进来,放在提交说明、链日志目录或 archive/。2026-11-14 之前的全文在 archive/prd_mempalace-2026-11-14.md(按节名查)。

## 写法规则

- 一个房间不超过 3 行:现状一句,入口(路径、命令、提交号),下一步一句。
- 只写三类:别人必须遵守的决定、不可逆的对外动作、阻塞发布的未验证项。
- 子代理不提交本文件,结论放回报;主调度只在里程碑(整链结束、公开、装机完成)改一次。
- 版本序只在 build/version-seq.tsv 定义,本文件不手写版本对照。
- 任务表行 `| 部分落地 | ID、ID |` 与 `| 已落地 | ID、ID |` 供 tools/observe-agents.cx 读取。

## 当前位置

- 已公开:0.0.66 为 Latest 且已装(APE 75c4e940);各版提交号、CI、裁决与缺口见 release/ 与 plans/release-notes-0.0.6x.md;回滚 release/rollback.txt。
- 0.0.67:已冻结(REV 855ce83aa9),整链 real11 在跑;入口 /tmp/unisacc-session-build-lr0060/(KEEP-README.txt 有 PID 与硬限),会话根勿删。
- 0.0.68:分支 v0.0.68 开发中;内容与验收见 prd.md 版本表。
- 编排:omp-go、omp-cdx 经 bin/mux envelope 派工,子代理至多 2 个,写域互不相交。

## 决定(别人必须遵守)

- 版本号跳过 0.0.64:0.0.63 之后是 0.0.65。入口:build/version-seq.tsv。
- 起整链的环境只设 UNISACC_IDENT_HELPER,不设 UNISACC_RUN_CACHE 与 UNISACC_CACHE_DIR(前者会关模型缓存)。
- 起链配方与发布 12 步:archive/prd_mempalace-2026-11-14.md 对应两节。
- 版本分支:main 只做集成与发布,开发版各一条分支,工作目录 .claude/worktrees/v0.0.NN,一分支一类大改。全文 AGENTS.md。
- 整链会话根在链运行与发布期间勿删,清理只由主调度在公开后做。
- 改产品源的验收必须含引导编译器(bootstrap-0.0.59)多单元编译,不只已装产品 -c(real10 教训)。
- 写仓内文件只用 Edit/Write 或 .cx 工具;guard-edit.ts 拦 sed -i 等。

## 阻塞发布

- 发布裁决恒为 NOT_QUALIFIED unknown=1:s9 commit 档按设计不给全批资格,daily 与 release 档未跑通(0.0.68 资格面)。入口:release/decision.txt。

### 仍未验证(缺什么 / 清零动作)

- 编译器缺口:多单元合并下跨单元函数指针形参含结构体指针被拒(not covered: incompatible semantic declaration type),R3 去强转待它补上。
- 已装编译器对大型二维自动数组报 not covered(复现 $CLAUDE_JOB_DIR/tmp/repro-large-2d-auto-array.cx)。
- check-stdio-buffer:不覆盖 --link-only 与子集编译的缓存一致性,tty 路径未测。
- osx cache-manager:借用副本来源未证实,候选上首次 osx 实跑核来源。
- tsv-lint 不稳:同日三次实跑 PASS、FAIL、PASS,未定因。
- wt-gate-probe 为 PLANNED:链路超 55 s 批次上限,6 条断言未验证。
- slice-migration-gen:source-missing cx/compiler/frames.cx 旧记述,待复核。
- affected-iface2 批次未取得实跑结果:待跑。
- 已入库工具中 tools/status.cx 的测试夹具曾被 sed -i 改过,内容未重审。
- cx 模块语法缺口 use as alias 与软链模块身份:排 0.0.70。
- 仓库根 out/:逐项迁移到 sy_tmpdir,清单见归档同名条。

## 房间索引

| 房间 | 现状 | 入口 |
|---|---|---|
| 发布 | 0.0.66 公开为 Latest;0.0.67 在分支 v0.0.67 | release/、plans/release-notes-0.0.63.md、prd_release.md |
| 版本序 | 0.0.66(已公开)-> 0.0.67 -> 0.0.68 -> 0.0.69 | build/version-seq.tsv |
| 起链与发布做法 | 配方 5 条、发布 12 步 | archive/prd_mempalace-2026-11-14.md 对应节 |
| 规划 | R6、R7 与第五轮规划、流程精简方案 | archive/prd_mempalace-2026-11-14.md 的"下一动作总表 R6"与"流程精简"节 |
| 补丁 | 仓外补丁按版本分目录 | ~/.cache/unisacc/patches-0.0.63 至 0.0.67 |
| 速度 | 只认四个工具的实测 | build/speed-baseline.tsv |
| 未验证表 | 与上节逐条对拍 | build/state-unverified.tsv、tools/state-gen.cx |
| cx 设计 | 模块、内存安全、nn | prd_cx.md |
| 结构总表 | 五层、六片、流、网、事实表、构造链 | prd.md |
| 测试入口 | `sh ./unisacc.com tests/run.cx -- --smoke/--all/--list` | tests/batches.tsv、tests/coverage.tsv |
| 棘轮与派生物 | 13 项棘轮;derive 9 项 | tools/ratchets.cx、tools/derive.cx |
| 产品全景 | 生成块 | prd_productmap.md |
| 历史 | 往期发布与归档 | prd_hist.md、archive/ |
