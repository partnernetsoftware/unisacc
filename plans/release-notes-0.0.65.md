# unisacc 0.0.65 -- 发布说明

状态:公开前草稿,公开读回后补记。沿用 plans/release-notes-0.0.63.md 的口径:只写已入库事实;未实跑的项写未跑,未证实的项标 UNVERIFIED,不写 PASS;本文件不宣称 QUALIFIED。标"待填"的字段尚无事实,由发布流程填写,不得臆测。

- 冻结提交(REV):b7520b8e81;源标签 v0.0.65-src 指向 F:c43ba16b01dd6ba1e0f6be633c37648f971d62c8;候选表提交(X,仅改 release/candidate.tsv):cb1421d99f382027369e96b10b5d3ea328d2bfdb。整链所用私有提交:de2df6171e。
- 候选 APE(unisacc.com)sha256:889dfe853837d80b495b3835b9272700384df4b4facfa54e4c27f11bc4291bdb(7091300 字节)。
- 整链 real8:一次跑通,rc=0,墙钟 3694 s,compare 通过,产物 sha 前缀 2e7ec678aaca5ebf(出自记忆宫殿里程碑记载)。
- 同候选 CI:candidate-native.yml run 43620762916,completed success,10 个作业全 success,含 6 个平台候选作业。osx 与 win 的证据范围以该 run 作业为准,不扩大解读。
- release v0.0.65 资产 6 件:unisacc.com、pkg.bin、slice-win-arm64、unisacc.com.build.json、build.json、SHA256SUMS;无 zip。
- 公开时间:待填(gh release view 的 publishedAt)。release/decision.txt 裁决(公开前,tools/release-decide.cx 实跑):NOT_QUALIFIED unknown=2 fail=0(第 4 步 s9 与第 8 步读回为 UNKNOWN;读回在公开后补)。本版不宣称 QUALIFIED。
- 公开后读回:待填(6 件 sha256 与 SHA256SUMS 逐件核对、`--version` 输出、标签指向、Latest)。
- 速度数字只引 build/speed-baseline.tsv 已有行;装机后的 0.0.65 对 0.0.63 同口径对比尚未测,全部待填,未测不写数。

## 是什么

面向 C99 的编译器,自己写出 ELF、Mach-O、PE,可编译自身;产品形态为单文件 `unisacc.com`。0.0.65 的主题是 cx 模块实现的三源统一、模块公开名单语法 D1a,以及起链工具的三项加固。上一发布版见 plans/release-notes-0.0.63.md。

## 本版内容(按入库提交,范围 707e6f562e..b7520b8e81,共 19 个提交)

### (1) cxmod 三源统一

t6t7、checkcalls(cxm_checkcalls 误拒短名函数指针调用的修复,即 0.0.62 缺口 (9))、T8 三份补丁入库;cx/compiler/cxmod.cx 改为 cx0-extract 生成物。入库提交 2ce17760c0 记载的检查:check-cxmod-tri passed=15 failed=0 gaps=0(gaps 2 -> 0);cx0-extract --check --all 两行 EXACT;derive --check drift=0;ratchets ok=13 fail=0。cx.compile 升至 0.2.8。冻结 HEAD 上实跑:check-cxmod-tri passed=36 failed=0 gaps=0;cx0-extract 两行 EXACT。

### (2) D1a:#pragma unisa export NAME[,NAME...]

声明本模块自有非 static 函数的公开名单(规范见 prd_cx.md 10.7);11 条拒绝文案,每单元名单上限 128 个名。入库提交 90507688c8,测试提交 10ac1a7d40(按产品能力验证 export 语义拒绝)。限定:试点用法只在仓外证明;仓内模块改用 export 需装 0.0.65 之后做,本版未做。cx.plan 为此 bump 0.1.5(接口 api_sha 不变)。

### (3) 链工具三项

(a) 起链环境白名单 build/chain-env.tsv:IDENT_HELPER 必需,RUN_CACHE 与 CACHE_DIR 拒绝,未登记的 UNISACC_* 默认拒绝;candidate-chain 起链门在构造前 REFUSE chain-env,rc 2;冻结前一提交 b7520b8e81 登记 tools/bound 设置的 UNISACC_BOUND_DEADLINE_MONO(allow)。(b) g2 对象批次先测单键金丝雀再恢复预算宽度(提交 64a6387504)。(c) 起链看门狗 chain-pace(提交 8d3ccdd475,tools/chain-pace.cx,只读挂载)。

看门狗限定:本次整链未给基线,按默认 60 s 把 44 次长相误判为 SLOW/STALLED;已记为发现,不是整链故障(整链 rc=0)。基线口径待下一次起链补。

## 未达项与已知缺口(必须如实写)

(1) first_e2e 500 ms 启动门未过。0.0.63 已装件首次启动 664.9 ms(build/speed-baseline.tsv 2026-11-14 末尾行,ABAB 交错 n=9);0.0.65 装机后的同口径数:待填。本版不宣称启动变化。
(2) 已知编译缺口,未修:已装 0.0.63 编译器对大型二维自动数组报 `not covered: an op outside the first encoder slice`。最小复现在仓外(repro-large-2d-auto-array.cx,入库位置待定)。0.0.65 是否同样复现:UNVERIFIED。
(3) tests/cx-namespace.cx 新增两条产品语义断言,待装 0.0.65 后实跑:未跑。
(4) s9 commit 档(主机范围 lnx/x86_64):选中 325 批,309 PASS、12 FAIL、4 UNKNOWN,工具裁决 `S9_VERDICT 4 UNKNOWN reason=tier-commit-not-full`(commit 档按设计不给全批资格),decision.txt STEP 4 记 UNKNOWN,不写 PASS。定性:无产品回归。逐项:
- qsort-intro、stdio-buffer、xs-bound-width、include-shadow、safedel-chains、entry-receipt-head、release-ctor-fast 共 7 项 FAIL:s9 根内候选缺可执行位(权限 0600)所致;补位后在对照根逐项单跑全过。这 7 项为 s9 准备失误,单跑通过,非 s9 执行器整批证据,s9 内的 FAIL 记录保留不改。
- tlib-prep-refuse FAIL:提交 8d3ccdd475 引入的工具依赖缺陷(candidate-chain 经 cxlib.runnerprep 触达 tlibpkg),属工具不进产品,未修。
- candidate-chain:超出 55 s 预算,记 UNKNOWN,待拆批。
- tlib-prep:s9 内超时;单跑 12.9 s 通过(单跑证据,非 s9 整批证据)。
- chain-batchkeys、preflight-tool、release-usage、report-verify-tool:0.0.63 已有,冻结根环境所致,照旧,未在其他根复跑证实(UNVERIFIED)。
- fixedpoint-sample、prechain-gate:watchdog-deadline,0.0.63 已有,照旧,未复测。
daily 与 release 档未跑。0.0.63 s9 失败批次中未修项(release-usage 等,见 plans/release-notes-0.0.63.md 未达项 (3))照实沿用,本版未新增修复证据,UNVERIFIED。
(5) 本机全批资格:未取得(s9 commit 档 309 PASS、12 FAIL、4 UNKNOWN,见 (4))。
(6) osx 与 win 平台证据:沿用 0.0.62 缺口,CI run 作业之外未新增平台实跑,UNVERIFIED。
(7) D1a 在仓内模块的落地、export 对既有模块的影响:未做。

## 0.0.63 已知缺口里仍开的项

沿用 plans/release-notes-0.0.63.md 编号;本版未修或未验证修复,不得写成已解决。已处理:0.0.62 缺口 (9) 的 checkcalls 修复随本版入库(见内容 (1)),其余仍开:
(1) DNS 未做(功能边界):sy_resolve/sy_resolve4 只认数字字面量与 localhost。
(2) std-surface 的 new 行、sy_unix_connect 越过截止时间、sy_tcp_accept_from 对 unix 监听返回 -EAFNOSUPPORT、poll 后阻塞调用截止时间、EINTR 上抛、netmsg salen 为负、sy_sig_taken 非原子:本版未重核,未修。
(3) cx 模块软链被当两个模块、osx/win 的 -ENOSYS 点未全进 exec/facts/refusals.tsv:未修。
(4) arm64 msghdr 布局未实跑;osx/win 无主机实跑证据;osx cache-manager 自产件在 macOS 上的运行与签名未验证:本版未新增证据。
(5) 批次全集复验面不完整,s9 daily 与 release 层未跑通一次。
(6) check-stdio-buffer 覆盖范围、dock-audit 全部 UNAUDITED、derive 总闸登记项偏少:未变。

## 撤回与回滚

按运行手册(release/rollback.txt,0.0.65 版,首行含 0.0.65,随 Y1 入库):撤回公开用 `gh release edit v0.0.65 --draft=true` 并把 v0.0.63 重设为 Latest;main 上提交只用 `git revert`,不 force push;本机根回退到 ~/.cache/unisacc/ape-0.0.63(sha 前缀 04b85e6e)用 tools/install-ape.cx。回滚核对项:资产件数按 6 件。

规则:发布页正文必须与本文件逐字相同,公开后读回核对;读回结果记账本。

## 待核对

(1) 整链产物 sha 前缀 2e7ec678aaca5ebf 与公开件 sha 的关系(公开件为 APE 889dfe85...,两者口径不同,读回时分别核)。
(2) 装机后补:启动、冷编译、生成代码对 0.0.63 的同口径对比,入 build/speed-baseline.tsv 后再引。
(3) 公开时间、读回结果(公开后补)。
