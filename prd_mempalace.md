# 记忆宫殿(地图)

本文件是地图,不是账本。每个房间只放一句现状和证据入口;过程、取证、采样、重跑记录不进来,放在提交说明、链日志目录或 archive/。2026-11-14 之前的全文在 archive/prd_mempalace-2026-11-14.md(按节名查)。

## 写法规则

- 一个房间不超过 3 行:现状一句,入口(路径、命令、提交号),下一步一句。
- 只写三类:别人必须遵守的决定、不可逆的对外动作、阻塞发布的未验证项。
- 子代理不提交本文件,结论放回报;主调度只在里程碑(整链结束、公开、装机完成)改一次。
- 版本序只在 build/version-seq.tsv 定义,本文件不手写版本对照。
- 任务表行 `| 部分落地 | ID、ID |` 与 `| 已落地 | ID、ID |` 供 tools/observe-agents.cx 读取。

## 当前位置

- 版本:0.0.65 已公开(2026-11-14T18:02:31Z,Latest,已装,APE sha 889dfe853837d80b)。冻结 REV b7520b8e81,F=c43ba16b01(v0.0.65 与 v0.0.65-src),X=cb1421d99f,Y1=ecd4201d42,Y2=7c219edbc3;整链 real8 一次跑通 rc=0 3694 s;CI run 43620762916 十作业全 success;裁决 NOT_QUALIFIED unknown=1(s9 commit 档按设计)。证据 $CLAUDE_JOB_DIR/tmp/rel65y/。回滚:release/rollback.txt;0.0.63 备份 ~/.cache/unisacc/ape-0.0.63(sha 04b85e6e)。内容:cxmod 三源统一、D1a export、链工具三项(环境白名单、g2 金丝雀、看门狗)。
- 0.0.66 已公开(2026-11-14T23:12:18Z,Latest,已装,APE sha 75c4e940):整链 real9 rc=0 3676 s(REV 24bfc44c70);F=e96881d77d(v0.0.66 与 v0.0.66-src),X=9722945843,Y1=176e10114f,Y2=7870a5256e;CI run 43648734577 十作业全 success;s9 329 批 310/7/12,无产品回归;裁决 NOT_QUALIFIED unknown=1。回滚 release/rollback.txt,备份 ~/.cache/unisacc/ape-0.0.65。证据 $CLAUDE_JOB_DIR/tmp/rel66/、/tmp/s966/。待修(下一版分支):surface-doc-gen 与 dock-audit 树内漂移、s9-native 缓存键不含依赖模块、candidate-chain 材料未接线连带同片 6 批不执行。会话根 /tmp/unisacc-session-build-lr0060 曾于 11-15 02:14 至 05:20 被未查明者整删,已按配方重建。
- 上一版 0.0.63:2026-11-14T14:12:06Z 公开,F=9f3d367e44,APE sha 04b85e6e;0.0.62 备份 ~/.cache/unisacc/ape-0.0.62。
- 0.0.67(分支 v0.0.67,领先 main 14 提交,8ac1789b39):R3 与随行小修全部入库(s9-native 缓存键、candidate-chain 材料、两处漂移、speed-probe 落盘、release-sweep、s9-shards 符号链接根、no-out-snap、affected 平台过滤)、加速方案 A 第 1 至 3 步;已冻结:v0.0.67 合入 main(e880e880a4),冻结 REV 12d52b93f3,chain-ready READY yes(4-p1 修 cbf417bec8);整链 real10 起于 2026-11-15 09:49:05 +0800,私有提交 59b6a91dcc,本体 PID 1779027,PGID 1779026/1779025,硬限 11:19:05,输出 /tmp/unisacc-session-build-lr0060/chain-real10,日志 log/real10.out;会话根勿删。未验:R3 的 9 败只凭文案判定、commit-own 批 UNKNOWN、加速待放开 chain-run width=1 互斥。
- 0.0.68(分支 v0.0.68,进行中):L4 零解码实现中(omp-cdx;首步实测布局段 36.8 ms);库包改进 A 派生表一条命令闭环(子代理);check-lower-fnreuse 已入 4423d9bccf。随行顺序与验收见 prd.md 版本表。
- 编排:omp-go、omp-cdx 两窗口经 bin/mux envelope 派工,子代理至多 2 个;写域互不相交,版本工作目录根放 unisacc.com 符号链接。
- 0.0.65 已知缺口(发布说明已写):s9 325 批 309/12/4,7 项为 s9 根内候选缺可执行位的准备失误(单跑通过);tlib-prep-refuse 为看门狗提交引入的工具依赖缺陷;candidate-chain 批超 55 s 预算待拆批;看门狗未给基线致 44 次误判;已装编译器对大型二维自动数组报 not covered(最小复现 $CLAUDE_JOB_DIR/tmp/repro-large-2d-auto-array.cx)。

## 决定(别人必须遵守)

- 版本号跳过 0.0.64:0.0.63 之后是 0.0.65。入口:build/version-seq.tsv。
- 起整链的环境只设 UNISACC_IDENT_HELPER,不设 UNISACC_RUN_CACHE 与 UNISACC_CACHE_DIR(RUN_CACHE=0 经 cx/runtime/modelcache.cx mc_off() 关掉模型缓存,real7 因此在 g2 两次超 90 s)。
- 起链配方与 0.0.62 发布 12 步:见 archive/prd_mempalace-2026-11-14.md 的"起整链的做法"与"0.0.62 发布实际做法"两节;发布说明占位由 tools/notes-fill.cx 填。
- own-leak 补丁不与 cxmod 同候选,顺延 0.0.68;s2 补丁先过 F11 预演再入 main。
- 版本分支(项目所有者 2026-11-14 试行):main 只做集成与发布;开发中版本各一条分支 v0.0.NN,工作目录 .claude/worktrees/v0.0.NN;一条分支只放一类大改,冻结时合入 main。0.0.65(cxmod 三源类)分支在建。规则全文见 AGENTS.md"版本分支"条。
- 整链会话根 /tmp/unisacc-session-build-lr0060 在链运行与发布期间勿删(11-15 曾被未查明者整删);清理只由主调度在公开后做,再犯即上钩子拦截。
- 子代理用 sed -i、printf >> 等写仓内文件属违例;guard-edit.ts 已补 `cd 目录 && sed -i` 与 `sh -c` 漏洞。

## 阻塞发布

- 0.0.63 已公开;s9 commit 档 321 批 313 PASS、6 FAIL、2 UNKNOWN(derive、tlib-frozen 已在 main 41a15ffc7f 修;release-usage 待单跑;其余三个为冻结根环境造成),发布说明已如实写。
- s9 commit 档按设计恒为 UNKNOWN,裁决预计 NOT_QUALIFIED unknown=1;daily 与 release 档未跑。入口:release/decision.txt。

### 仍未验证(缺什么 / 清零动作)

- 新产品验证:cx-namespace 三缺口具名拒绝,需新产品复跑 tests/cx-namespace.cx。
- check-stdio-buffer:不覆盖 --link-only 与子集编译的缓存一致性,tty 路径未测。
- 速度基线:冻结候选与装机后产品的同口径重测未做;装机后各跑一次入表。
- 整链耗时:real7 三段累计,与 real6 的 3752 s 不可直接比。
- osx cache-manager:借用副本来源未证实,候选上首次 osx 实跑核来源。
- tsv-lint 不稳:同日三次实跑 PASS、FAIL、PASS,未定因。
- wt-gate-probe 为 PLANNED:链路超 55 s 批次上限,6 条断言未验证。
- slice-migration-gen:source-missing cx/compiler/frames.cx 旧记述,待复核。
- affected-iface2 批次未取得实跑结果:待跑。
- -c 对象复用:装机产品上 UNISACC_REQUIRE_N15C=1 硬模式断言未跑。
- 已入库工具中 tools/status.cx 的测试夹具曾被 sed -i 改过,内容未重审。
- cx 模块语法缺口:`use m::sym as alias`,先修别名(cxmod 三源产品输入)。
- cx 模块身份缺口:软链 tools/cxlib/sys -> cx/sys 被当两个模块,排在别名修复之后。
- 仓库根 out/:逐项迁移到 sy_tmpdir 并出信封,清单见归档同名条。
- 候选审查 cxmod 三源:checkcalls 误拒等 5 项,补丁验证后并入 0.0.65。
- 候选审查 cx/sys+ext:unix connect 截止、accept_from unix 等 7 项,补丁见 patches-0.0.66。

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
