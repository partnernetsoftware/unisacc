# unisacc 0.0.67 -- 发布说明

状态:已公开,公开读回后补记(Y2)。沿用 archive/plans-2026-11f/release-notes-0.0.66.md 的口径:只写已入库事实;未实跑的项写未跑,未证实的项标 UNVERIFIED,不写 PASS;本文件不宣称 QUALIFIED。标"待填"的字段尚无事实,由发布流程填写,不得臆测。

- 冻结提交(REV):855ce83aa9ed2fbf3823d5054048a851fb14485f;源标签 v0.0.67-src 指向 F:e02a8f8099c1214f028b10efb1ab55f1510ea7de(树取 REV,只差 release/candidate.tsv,父为 fbdc99ca 与 REV);候选表提交(X,仅改 release/candidate.tsv):9e54f512a8b06e3ea890920ce8a9428690767d55。整链所用私有提交:a67e7e2d11(树同 REV)。
- 候选 APE(unisacc.com)sha256:a99ea71de7edf8b21db804839e65dd130aff430c6d57cfb627a26e84a7b49e8b(7091428 字节)。pkg.bin sha256:c7cb3a8c3a6d88779bafc7aa0686ce6fe413c4477be502db4fbddf2cb092beae(与 0.0.66、0.0.65、0.0.63 的 pkg.bin 相同)。
- 整链 real11:rc=0,墙钟 3697 s,fixedpoint 的 objects、kernels、slices、product 四行均为 same,package sha 前缀 c7cb3a8c(出自整链日志与派工记载);链末状态行为 PREPARED,不是 PASS。此前的 real10 失败,见已知缺口 (1)。源身份 sources_sha256:b975f76fa3ae0ee19892928447908837d1657524bd97bd55480024ed7528c563(链内 identity-helper 对整链源快照实算;release-run --pack 对它的核对结果为 UNVERIFIED,因 F 无对应快照目录)。
- 本地打包(tools/release-run.cx --pack):步 4 至 9 全 DONE,结果 PACK_OK。tools/release-ape.cx smoke 对打包件:未跑。
- 同候选 CI:candidate-native.yml run 43672079447,2026-11-15T03:22:36Z 触发,输入 commit 为 X、release_tag 为 v0.0.67;结果:completed success(2026-11-15T03:25:43Z 结束),10 个作业全 success:fetch、ffi-lnx、cell-tools、report,以及 6 个平台候选作业(lnx x86_64、lnx arm64、osx arm64、osx x86_64、win x86_64、win arm64;其中 win arm64 的矩阵标记为 file,其余为 yes)。osx 与 win 的证据范围以该 run 作业为准,不扩大解读。
- release v0.0.67 资产 6 件:unisacc.com、pkg.bin、slice-win-arm64、unisacc.com.build.json、build.json、SHA256SUMS;无 zip。草稿上传后下载读回,5 件 sha256 与 SHA256SUMS 逐件一致,SHA256SUMS 与本地打包件逐字节相同(sha256 69e46e46f16c6e6300e23a930bce27dd09fece994a5981dc0b5638e271adf4a9)。
- 公开时间:2026-11-15T04:04:14Z(gh release view 的 publishedAt),Latest。release/decision.txt 裁决(tools/release-decide.cx 实跑):公开前 NOT_QUALIFIED unknown=2 fail=0,公开读回后 NOT_QUALIFIED unknown=1 fail=0(只剩第 4 步 s9 为 UNKNOWN)。本版不宣称 QUALIFIED。
- 公开后读回:下载 6 件,5 件 sha256 与 SHA256SUMS 逐件一致,unisacc.com sha256 同上;标签 v0.0.67 与 v0.0.67-src 均指 F。本机装机:公开后已装入本机根,--version 输出 unisacc 0.0.67(回执提交 7f64760505)。
- 速度数字只引 build/speed-baseline.tsv 已有行。0.0.67 的启动、冷编译、生成代码对 0.0.66 的对比:公开时未测;装机后实测(已装 0.0.67 对 0.0.66,两边同走包装根,ABAB 交错 n=9,build/speed-baseline.tsv 2026-11-15 的 0.0.67 行)。测时同机有其他作业,load1 3.00 至 8.35,高于 2,绝对值不可与 0.0.66 那批(load1 0.60 至 1.27)直接比:首次启动中位 717.8 对 677.4 ms(差 +40.4 ms,样本范围 655.5 至 777.2 对 654.4 至 766.6,最小值只差 1.1 ms,不下结论)、重复启动 29.6 对 30.6 ms(run-startup-bench)、冷编译 edit-span 2726 对 2768 ms、ratchets 5120 对 5162 ms(coldcompile),后三项差值在噪声内;gencode 8MB 两边各 2 轮,sha 1023.9、999.5 对 1033.4、1029.6 ms,sort 1120.3、1134.0 对 1115.2、1222.8 ms,memcpy 12.00、12.34 对 11.97、12.14 ms;speed-probe(hello,fresh-o 默认口径,全阶段,各 3 次)wall 中位 660.3 对 647.7 ms。本版不宣称任何速度变化;首次启动一项需在低负载下复测。
- 装后门实测(已装 0.0.67,APE a99ea71de7edf8b2):tests/cx-namespace.cx checks=18 failed=0 unverified=0;K3 硬模式(UNISACC_REQUIRE_N15C=1,tests/phase-timing-stamps.cx)assertions=76 failed=0;tools/check-export.cx passed=13 failed=0 unverified=0。

## 是什么

面向 C99 的编译器,自己写出 ELF、Mach-O、PE,可编译自身;产品形态为单文件 `unisacc.com`。0.0.67 的主题是 cx.stream R3 在驱动侧的 st_span 收敛,另带起链、测试与发布工具的若干小修。上一发布版见 archive/plans-2026-11f/release-notes-0.0.66.md。

## 本版内容(按入库提交,范围 24bfc44c70..855ce83aa9)

### (1) cx.stream R3:st_span 收敛

提交 fa0b41e8f8:drive 与 driveroute 改为 use stages,删去 4 处 st_span 手写声明;fwdincrement 改为 use compiler.fwdmerge,FwiBuf 改为 FwmBuf 的别名;merge 回调形参改为 FwmBuf*(T4)。

提交 f7537dce4d(修门):T4 的回调去强转部分因编译器缺口退回,fwd_incremental 的 merge 回调形参退回 void*,driveroute 的 dv_fw_merge 恢复强转;保留 use compiler.fwdmerge、FwiBuf 为 FwmBuf 别名与 T2、T3。即本版 R3 落地的是 st_span 声明收敛与别名归一,回调去强转未落地。

### (2) 随行工具小修

(a) s9-native 缓存键纳入传递依赖,并抽出无 tlib 的身份模块(9469d6a65f);对应 0.0.66 发布时记下的缓存键缺陷。
(b) candidate-chain 材料接线:s9-nightly 在 init 前准备材料并传给 s9-shards;缺材料只踢本批,同片其余批照跑(1fb541a387)。
(c) surface-doc-gen 再生 ext.procx 与 cx.stream 两文件,dock-audit 补 cx/sys/reslive.cx 一行,消除树内派生物漂移(37b426257f)。
(d) speed-probe:参数按 TAB 拆、计时环境变量真正传给子进程、只计 inference 行(3eec4cc8ee);全阶段落盘与真实墙钟汇总(3f4b04ad19)。
(e) release-sweep 检查在临时仓根误触保护的修复(86c7931a16)。
(f) s9-shards 与 all-waves 接受符号链接根(版本工作目录把 unisacc.com 链到共享检出),外来候选仍拒绝(18cead7873、485bbc9604)。
(g) no-out-snap:check-no-out-snap 先解析根下 unisacc.com 的符号链接,再按解析后的真实路径取产品件及其配套 .build.json;配套件缺失或不可读时具名报错并以 rc 2 退出(3f4b04ad19,与 (d) 的 speed-probe 落盘同一提交)。
(h) affected:选批按平台过滤(NOT_APPLICABLE 单独计数,不计 fail);继承截止已过期时一次具名拒绝;新增 cxlib/platformscope 模块及其检查与批次(8ac1789b39)。
(i) chain-ready 4-p1:去掉 1400 文件处的静默截断,check-cxmod-tri 新增分批并行;提交记载 1433 文件由 280 秒以上降到 13.6 秒,批与单文件输出逐字节对拍一致(cbf417bec8)。
(j) 加速方案 A 第 1 至 3 步:pack 六项输入全取 fixedpoint/g2(a5fa58ec1d),g3 与 compare 移出 pack 关键路径(d13703d6f5)等。提速未兑现:real11 墙钟 3697 s,对 0.0.66 的 real9(3676 s)没有缩短;待放开 width=1 互斥后才可能见效。

其余提交为 0.0.66 的公开记录、版本计划与文档、派生物重生,不属本版产品改动。

## 未达项与已知缺口(必须如实写)

(1) 整链 real10 失败:0.0.67 首次冻结后的整链 real10 未通过;定因后由 f7537dce4d 修门(回调形参退回 void*),重新冻结于 855ce83aa9 并由 real11 跑通。本版候选只对应 real11。
(2) 编译器缺口,未修:多单元下函数指针形参的类型处理有缺口,致使 R3 的 T4 回调去强转无法落地;本版以退回 void* 加强转绕开,缺口本身仍在。
(3) R3 的 affected 复验:大量批次为 deferred,未实跑,不是 PASS。
(4) 同候选 CI run 43672079447 已结束且全 success(见文首);osx、win 平台证据仅限该 run 各作业实际执行的内容,本机未做任何 osx、win 实跑。
(5) s9 commit 档(主机范围 lnx/x86_64,根 /tmp/s967/root 为 F 树快照加候选件,候选 sha 前缀 a99ea71d):选中 331 批,311 PASS、8 FAIL、12 UNKNOWN,另有 12 批 osx/arm64 记 NOT_APPLICABLE 不计入;工具裁决 `S9_VERDICT 4 UNKNOWN reason=tier-commit-not-full`(commit 档按设计不给全批资格),decision.txt STEP 4 记 UNKNOWN,不写 PASS。s9 的 daily、release 档:未跑。本机全批资格:未取得。定性:未发现编译器产品回归;本版新增的红项均落在测试树与选批工具(见下)。s9 内的 FAIL 与 UNKNOWN 记录保留不改,未重跑整档。对照 0.0.66(329 批,310、7、12)逐项:
- testplan-controls、std-surface-v2、sizeof-postfix-positive、sizeof-postfix-refusal、source-package-mixed-profiles 共 5 项 UNKNOWN(本版新增):同在 slice-0-52,整片被 tools/all-waves.cx 第 159 行拒绝(`REFUSE cx-plan tests/plan.cx`),5 批均未执行。根因是树内缺陷:8ac1789b39 让 tools/cxlib/allsession.cx 新增 use platformscope,而 tests/cxlib/ 下只有 allsession.cx 的符号链接,没有 platformscope.cx 的符号链接;同根单跑 `-cx-plan tests/plan.cx` 得 rc 1 与 `module platformscope: no file tests/cxlib/platformscope.cx`,用已装 0.0.66 产品件对同一输入得到同样文案,故不是编译器回归。直接受害的是 testplan-controls,其余 4 项为同片连带;这 4 项未单跑,UNVERIFIED。未修。
- affected-list2(spawn-literal-transitive)、affected-iface(iface-same-selected-plus-deferred-is-full、iface-same-summary-names-full)、affected-iface2(iface-commit-tier-conserves、iface-pkg-other-stale-conserves)、batch-packages(layer-keeps-all、layer-all-subset)共 4 项 FAIL(本版新增):都是 tools/affected.cx 的检查。同根单跑佐证两处:release-ape-replacements 现被平台过滤记为 NOT_APPLICABLE,不再以 AFFECTED 行出现;`--layer base --list-only` 汇总为 batches=826、own=12、all=836,不满足 batches 等于 own 加 all 加 unknown。与 8ac1789b39 的按平台过滤改动相符,属选批工具及其检查的回归,不在产品件内;逐条断言未一一定因,UNVERIFIED。未修。
- state-gen FAIL(本版新增):断言 versions-real 红。tools/check-state-gen.cx 第 74 行写死 `versions rows=6`,同根单跑 `state-gen --emit versions` 输出 `versions rows=7`(rc 0);版本序表加行后检查字面量未跟,属检查漂移。未修。
- tlib-prep、on-change UNKNOWN(本版新增):55 秒看门狗超时(watchdog-deadline,rc 142)。tlib-prep 在 0.0.65 为 UNKNOWN、0.0.66 为 PASS。未单跑复测,UNVERIFIED。
- release-usage、preflight-tool、chain-batchkeys(no-out-dir-in-checkout)3 项 FAIL:0.0.63、0.0.65、0.0.66 已有,冻结根环境所致,照旧;未在其他根复跑证实,UNVERIFIED。
- fixedpoint-sample、prechain-gate UNKNOWN:55 秒与 49 秒处看门狗超时,0.0.63 至 0.0.66 已有,照旧,未复测。
- candidate-chain、candidate-chain-objects、candidate-chain-watch UNKNOWN:`S9 NOT_AVAIL material=candidate-chain-tool`,材料映射为空所致,见 (11);0.0.66 已有。本版"缺材料只踢本批"生效:0.0.66 被同片连带的 root-canon、root-independent、ctor-debt、rechunk、sys-reslive、ext-procx-exec 6 项本次在 s9 内为 PASS。
- 0.0.66 的 surface-doc-gen、dock-audit、release-sweep、report-verify-tool 4 项 FAIL 与 s9-nightly-run-fail UNKNOWN 本次在 s9 内为 PASS。
(6) release-ape smoke:未做。公开后读回、本机装机:已做(见文首)。release-decide 已实跑(见文首)。
(7) first_e2e 500 ms 启动门:未过门,不宣称启动变化;0.0.67 装机后 startup_first 中位 717.8 ms(load1 3.14 至 7.11,最小 655.5 ms),0.0.66 装机后同口径为 665.1 ms(load1 0.60 至 1.09)。
(8) 加速方案 A 提速未兑现(见本版内容 (2)(j));整链墙钟仍高于 2400 s 的门。
(9) sources_sha256 在打包步的核对为 UNVERIFIED(见文首)。
(10) 0.0.66 发布说明"未达项与已知缺口"与"0.0.65 已知缺口里仍开的项"两节所列各项:本版未修或未验证修复,仍开;其中 s9-native 缓存键与 candidate-chain 材料两项有工具改动(本版内容 (2)(a)(b)),s9 commit 档复跑结果见 (5) 与 (11):缺材料只踢本批已见效,材料映射仍为空;s9-native 缓存键一项未单独验证,UNVERIFIED。
(11) candidate-chain 材料接线(本版内容 (2)(b))在真实 s9 init 中未生效,工具缺陷,未修:tools/affected.cx 第 1226 行的 --materials-dump 输出四列 `MATERIAL<TAB>批<TAB>材料名<TAB>相对源路径`,且该模式不构造材料;tools/s9-nightly.cx 的 materials_map 按三列 `MATERIAL<TAB>名<TAB>/绝对路径` 解析,四列行一律落入"REFUSED 行"分支被跳过,MAP 恒为空,init 以 `-` 传给 s9-shards。实测:0.0.67 s9 commit 档 init 后 RUN/materials.tsv 为 0 行(单跑 dump 共 532 行,无一行含绝对路径)。"缺材料只剔本批"不受此影响。

## 撤回与回滚

撤回办法以 release/rollback.txt 的 0.0.67 版为准(随 Y1 入库):(1) `gh -R partnernetsoftware/unisacc release edit v0.0.67 --draft=true`,再 `gh -R partnernetsoftware/unisacc release edit v0.0.66 --latest` 把 Latest 移回 0.0.66;(2) main 上的 X、Y1、Y2 提交只用 `git revert`(新的先退),不 force push;(3) 标签 v0.0.67 与 v0.0.67-src 不删;(4) 本机根回退件为 0.0.66 的 APE(sha 前缀 75c4e940,取自公开的 v0.0.66)。回滚核对项:资产件数按 6 件。

规则:发布页正文必须与本文件逐字相同,公开后读回核对;读回结果记账本。

## 待核对

(1) CI run 43672079447 的结论与各作业结果:已填入文首。
(2) pkg.bin 与 0.0.66 逐字节相同(sha 同为 c7cb3a8c...)而 APE 不同:差异来自六个平台片与注入版本,公开前核一次各片 sha 与整链收据。
(3) 公开时间、裁决、读回结果、装机后速度对比与装后门:已补(见文首);首次启动对比在高负载下测得,低负载复测未做。
(4) 本版内容 (2)(g) 的 no-out-snap 细目。
