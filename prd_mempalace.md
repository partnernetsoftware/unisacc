# 记忆宫殿(地图)

本文件是地图,不是账本。每个房间只放一句现状和证据入口;过程、取证、采样、重跑记录不进来,放在提交说明、链日志目录或 archive/。2026-11-14 之前的全文在 archive/prd_mempalace-2026-11-14.md(按节名查)。

## 写法规则

- 一个房间不超过 3 行:现状一句,入口(路径、命令、提交号),下一步一句。
- 只写三类:别人必须遵守的决定、不可逆的对外动作、阻塞发布的未验证项。
- 子代理不提交本文件,结论放回报;主调度只在里程碑(整链结束、公开、装机完成)改一次。
- 版本序只在 build/version-seq.tsv 定义,本文件不手写版本对照。
- 任务表行 `| 部分落地 | ID、ID |` 与 `| 已落地 | ID、ID |` 供 tools/observe-agents.cx 读取。

## 当前位置

- 版本:0.0.63 已公开(2026-11-14T14:12:06Z,Latest,已装,APE sha 04b85e6e4ec3ddcb)。F=9f3d367e44(v0.0.63 与 v0.0.63-src),X=86e85e60c8,Y1=d57b5be605,Y2=707e6f562e,CI run 43600577542 十作业全 success,裁决 NOT_QUALIFIED unknown=1(s9 commit 档按设计)。回滚:release/rollback.txt;0.0.62 备份 ~/.cache/unisacc/ape-0.0.62(sha 83b82c8d)。
- 整链 real7:2026-11-14 21:29 完成 rc=0(三段,第三段 20:50:57 起),compare 通过;产物 /tmp/unisacc-session-build-lr0060/chain-real7/ape-final/unisacc-next.com(sha256 前 16 位 8312372d7af1e23d,未注入版本),收据 ape-receipt.txt。该目录保留到下一版公开并读回为止。
- 装后门已过(2026-11-14):K3 硬模式 assertions=76 failed=0、cx-namespace 18/0、速度无变慢项(build/speed-baseline.tsv 末尾);大扫除第一遍已做(/tmp 9.6G -> 7.6G,处置表 $CLAUDE_JOB_DIR/tmp/sweep63.txt);chain-real6 与 0.0.62 两份快照按 R6-H65 留到 0.0.65 公开后删。
- 下一步:0.0.65(cxmod 三源类)在 v0.0.65 分支推进,t6t7、checkcalls、T8 已落(2ce17760c0),待补 cx.plan 的 tlib bump。

## 决定(别人必须遵守)

- 版本号跳过 0.0.64:0.0.63 之后是 0.0.65。入口:build/version-seq.tsv。
- 起整链的环境只设 UNISACC_IDENT_HELPER,不设 UNISACC_RUN_CACHE 与 UNISACC_CACHE_DIR(RUN_CACHE=0 经 cx/runtime/modelcache.cx mc_off() 关掉模型缓存,real7 因此在 g2 两次超 90 s)。
- 起链配方与 0.0.62 发布 12 步:见 archive/prd_mempalace-2026-11-14.md 的"起整链的做法"与"0.0.62 发布实际做法"两节;发布说明占位由 tools/notes-fill.cx 填。
- own-leak 补丁不与 cxmod 同候选,顺延 0.0.68;s2 补丁先过 F11 预演再入 main。
- 版本分支(项目所有者 2026-11-14 试行):main 只做集成与发布;开发中版本各一条分支 v0.0.NN,工作目录 .claude/worktrees/v0.0.NN;一条分支只放一类大改,冻结时合入 main。0.0.65(cxmod 三源类)分支在建。规则全文见 AGENTS.md"版本分支"条。
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
| 发布 | 0.0.62 公开;0.0.63 冻结中 | release/、plans/release-notes-0.0.63.md、prd_release.md |
| 版本序 | 0.0.63 -> 0.0.65 -> 0.0.66 -> 0.0.67 -> 0.0.68 | build/version-seq.tsv |
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
