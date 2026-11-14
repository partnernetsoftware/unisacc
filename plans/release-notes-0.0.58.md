# unisacc 0.0.58 -- 发布说明

状态:公开发布(GitHub Release v0.0.58,Latest)。同候选 CI QUALIFIED,s9 commit 档 PASS(NOT_FULL,仅 12 个 osx 批本机不适用),release-decide QUALIFIED YES unknown=0 fail=0。未实跑的项一律写未跑。

- 冻结 rev F `2a46e1a4275a616628f3a8965e9a749aa5bdfe40`(标签 v0.0.58-src;父提交 9a34cb97b4,不在任何分支上,见账本 FREEZE-058B)。
- 提交 X `7b551b85d50bf22ed0d597ad09afa1debd04e334`(只含 release/candidate.tsv)。
- 提交 Y:tag v0.0.58 所指提交(本文件随 Y 提交;候选表回填 ci_run_id=43247952428)。
- 候选 APE `unisacc.com` sha256 `3744eacfa6b2472de8b18ef3d64ffad6f6a47f956d62e4a43760dee6e768632e`,6751268 字节,`--version` 实测 `unisacc 0.0.58`。
- git 范围 `v0.0.57..Y`。

## 是什么

面向 C99 的编译器,自己写出 ELF、Mach-O、PE,可编译自身;产品形态为单文件 `unisacc.com`。0.0.58 的产品改动只有一类:cx.stream(含其 Windows 修正)。上一发布版见 `prd_hist.md` 往期发布小节。

## 行为变化(产品输入)

(1) 新增核心包 cx.stream@0.1.0:`cx/stream/item.cx`、`cx/stream/pipe.cx`,12 个公开函数 xs_*,只依赖 mem.grow、mem.slice、mem.bytes、text.text;接口清单 build/layer-api/cx.stream.tsv(`2afa486d68`)。
(2) 首个消费者 `src/drivestream.cx` 改为两阶段管道(剥属性表,再进模型阶段或 sidecar)。
(3) `9a34cb97b4`:ext/memx 的跨单元 use 改为包根限定名,消掉 whole-front 的重复导出。
(4) win cx.stream 修正(FIX058-WINBOUND,`ea8499cfbb` 的产品部分换入冻结树):`xs_open` 的「不设上界」原写 `0x7fffffffffffffffL`,win 两目标 long 为 32 位,实参装入后为 -1,管道在任何输入前被拒,表现为 rc=1、无输出、无标准错误;改为 `(long)(~0UL >> 1)`。dv_stream 在管道搭建失败时写出原因,不再静默。新批 xs-bound-width 在六目标上对调用点实跑,旧字面量作对照。
设计预期是 stream 命令行为不变;lnx 上常量值相同。

## 身份与证据

| 对象 | 结果 | 证据 |
|---|---|---|
| 整链 | bg-candidate 整链 498/498 到 pack,PROMOTE_CHECK OK,compare objects/kernels/slices/product 全 same | out/b1/2a46e1a4275a616628f3a8965e9a749aa5bdfe40 |
| 打包 | release-run --pack PACK_OK,源 SID VERIFIED | out/rel058/pack2.log,out/rel058/rr2 |
| 七资产 | 草稿读回逐件同字节,SHA256SUMS 6/6 OK,release-layout --check PASS;公开后再次读回见下 | out/rel058/draft-readback2,out/rel058/public-readback |
| 同候选原生 CI | 运行 43247952428(检出 X,ape_sha256 3744eacf,release_tag v0.0.58):CANDIDATE_QUALIFY verdict=QUALIFIED pass=42 fail=0 not_run=0 invalid=0;win/x86_64 与 win/arm64 的 stream-pipe 格本次 PASS | out/rel058/ci-run2.log |
| s9 commit 档 | 最终一轮 `S9 COMMIT_ONLY tier=commit host_scope=lnx/x86_64 selected=223 status=PASS failed_or_unknown=0 pending=0 qualification=NOT_FULL`,not_applicable=12(osx) | out/rel058/s9-merge-r7.txt,out/rel058/s9-clone6/out/s9-run |
| release-decide | 步 1 至 8 全 PASS,`QUALIFIED YES unknown=0 fail=0`,rc=0;步 8 用的是公开前的草稿读回(7 件逐件同字节,SHA256SUMS 6/6),公开后另做公开读回 | out/rel058/release-decide2.txt,out/rel058/draft-readback3 |

win 的真实 PE 只由同候选 CI 判定(本机没有 Windows 宿主),该 CI 已通过。

## s9 的如实经过

- s9 不在 X 上跑:X 的 tools 树里有检查调用 `sy_mkdtemp`,该接口在 ef18cf3560 之后才进 main,用候选 APE 在 X 的树上编不过这些检查。因此 s9 在 main 的较新提交上用同一个候选 APE 跑,结论绑定的是「候选 APE 字节加该提交的测试树」,不是「X 的测试树」。
- 第一轮(克隆 out/rel058/s9-clone,提交 ef18cf3560,候选 APE):selected=210,结论 FAIL,failed_or_unknown=1(账本 S9-058-R3)。6 个先前 UNKNOWN 的超时批(含 tlib、tlib-prep)用工具自带的封存重试切片重跑后 PASS,时限未放宽。唯一 FAIL 是 cx-reaper:20 项中 14 项 FAIL,均为 np_run pid=-1。原因:该检查把 RLIMIT_NPROC 设为「同 uid 现有进程数加 200」,而这个上限按用户计,s9 并行波里同用户的其他批次与其他窗口吃光余量,起进程失败;属测试脆弱,不是产品缺陷。安静机器单跑两次 20/0。release-decide 因此拒绝(rc=1),当时不公开。
- 修正:`7b15d69d2d` 把余量改为 1000,三处 spawn 补成真有界,断言一条未改;`72657431f3` 修 s9-shards 在含重试切片的 RUN 上 merge 误拒(S9-AGG-MERGE-RETRY)。
- 第二轮(克隆 out/rel058/s9-clone2,提交 7b15d69d2d,同一候选 APE 3744eacf):第一次 RUN(s9-run)作废,克隆根的 unisacc.com 没有执行位(由 layout 拷入时为 0644,准备疏漏),凡经 exec 调产品的批都失败(xs-bound-width、include-shadow、entry-receipt-head、release-ctor-fast 等),不计。补执行位后新 RUN(s9-run2):selected=217,FAIL,failed_or_unknown=3,cx-reaper 仍 14/20 FAIL(余量 1000 也不够),另 fixedpoint-sample、tlib-prep 为 UNKNOWN。
- cx-reaper 的真正成因:上限计数用 `awk ... /proc/[0-9]*/status`,波次里有进程在 glob 与读取之间退出,gawk 读不到文件即致命退出,计数为空,上限变成绝对值 1000,低于本用户现有约 1240 个线程,于是所有 spawn 失败(np_run pid=-1)。余量从 200 改 1000 因而无效。修正 `0341f9a157`:改经 `cat ... 2>/dev/null | awk` 读取,计数为空时不设上限,NOTE 打印实际上限;断言未改,单跑 20/0。gawk 遇缺失文件致命退出已实测;s9 中的那次消失未直接观测到(帮手的标准错误未被捕获),属强推断。
- 第三轮(克隆 out/rel058/s9-clone3,提交 7b15d69d2d 加 `0341f9a157` 的 check-reaper 修正;不在 main HEAD,因为其时 main HEAD 含 b00ff0cb5b,其 tests/batches.tsv 第 995 行 std-surface-v2 的覆盖号在 tests/coverage.tsv 里不存在,s9 init 拒绝 registry-metadata unknown-coverage):selected=217,FAIL,failed_or_unknown=2。cx-reaper PASS。register-metadata-rollback 与 candidate-chain 超时,在封存重试切片中 PASS。剩下两项在同一切片 slice-0-22:cx-lint FAIL(85 项中 1 项,check-anchor-private-unknown.cx 的子编译报 `no such file for #include "/tmp/ua-cxif-43d8279d347795c4.h"`),fixedpoint-sample UNKNOWN(超时);该切片因含 FAIL 被 retry 拒(sticky-fail),无法封存重试。cx-lint 安静单跑 85/0。产品写 ua-cxif 接口头是「临时文件加 rename」原子写,产品本身不删除它;共享 /tmp 里是谁在两步之间删掉了它未查明。判断为共享环境干扰而非候选回归,但未证实,所以不记 PASS。
- 第四轮(克隆 out/rel058/s9-clone4,main 1741a7f69d,无本地补丁;起这轮前补了 b00ff0cb5b 漏提交的 interfaces.std-surface-v2 覆盖行 `1741a7f69d`):整轮被拒 frozen-input-changed。成因:tlib-prep FAIL,`62cf6fedab` 引入的 use_kit_dir(kitdir) 用 kitdir 格式化自身把它清空,kit 以 `--out ""` 把 .tlib 写进克隆根。修正 `b2fab66bf0`(经独立缓冲复制),候选 APE 克隆内单跑 failed=0。
- 第五轮(克隆 out/rel058/s9-clone5,main b2fab66bf0):init 被拒,`6f7835b4b4` 登记的 iface-gap 批的覆盖行未入库;补行 `aa15da4a0b`。
- 第六轮,即最终一轮(克隆 out/rel058/s9-clone6,main aa15da4a0b,无本地补丁,unisacc.com 为候选 APE 3744eacf 且有执行位):整棵 s9 进程树用私有 TMPDIR(克隆内 out/tmp),避免共享 /tmp 的同名文件(如 ua-cxif-*.h)被别的进程删掉。这是隔离措施;第三轮 cx-lint FAIL 的确切成因未证实,共享 /tmp 冲突的假设也未验证。结果 PASS:selected=223,failed_or_unknown=0;register-metadata-rollback、fixedpoint-sample、real-input-check、tlib、tlib-prep 五批首跑超时 UNKNOWN,在封存重试切片中 PASS,时限与断言未改。
- tlib 与 tlib-prep 安静单跑约 52.5 与 48.7 秒,贴近 55 秒批预算(DEBT-TLIB-BATCH-SPLIT,未拆)。
- 12 个 osx 批本机 NOT_APPLICABLE,qualification 记 NOT_FULL;osx 由 CI 的 osx 格覆盖,s9 本身不称全量。
- 之前更早一个候选(9a34 构造,X=931fa623)在 s9 上的 3 FAIL 加 1 UNKNOWN 已由 S9FIX-058 四项修复处理:(1) chain-phases-gen 检查改读生成器输出的限时(`ee846a2298`);(2) productpkg-dir-identity 夹具改为固定桩(`d396838926`);(3) batch-packages 重生成并由 register 自动重生成(`8ff69ecace`);(4) chain-batchkeys 预算 55 改 90,由 LIMIT-90-TEMP 覆盖。

## 整链经过(如实)

- 先前候选(冻结 9a34cb97b4)在同候选 CI 43219651302 上 FAIL(pass=40 fail=2,win 两格 stream-pipe),未公开(账本 REL058-HOLD);修正见上文第 (4) 条,重新冻结为 F(FREEZE-058B),候选仍只含 cx.stream 一类产品改动。
- F 的整链 21:09 起、23:22 到 pack DONE,约 2 小时 13 分,75 个分段,链中记 2 条 BLOCKER:
  (1) 第 205 次尝试 kernels-g2 在 90 秒时限处被杀(wall 88.0 秒;上一条链同相 80.2 秒),此时全链 3 次 resume 已在 gen1 的 osx/win 相用完,记 resumes-exhausted;
  (2) 同目录重启后第 208 次尝试被 chain-step 拒(被杀那次留下了未记录的 kernel-route 目录)。处置 KERNELS-SCRUB:在 candidate-chain 里对无记录的 kernels 残留做清理,有记录的一概不动;不放宽时限、不加 resume 次数。kernels 相 80 至 88 秒对 90 秒时限余量不足,仍是风险。
- 链工具取自工作检出,冻结的是产品输入,不是链工具。

## LIMIT-90-TEMP(时限临时放宽,如实)

项目所有者 2026-11-12 授权把链相位与批子进程时限由 55 秒临时放宽到 90 秒,明确是临时覆盖「不放宽时限」铁律。本候选的整链在 90 秒界下跑。不变:分段 270 秒、各批 budget_seconds 仍 55、g2 预算 55000 毫秒。尚未回收,回滚办法见账本 LIMIT-90-TEMP。

## 已知缺口(如实)

- s9 只跑了 commit 档,而且在 main 的较新提交上而非 X;release 档、daily 档未跑。
- osx 批在本机 NOT_APPLICABLE。
- win PE 的行为只由同候选 CI 判定,本机未跑。
- 整版超 2 小时铁律:F 整链约 2 小时 13 分,加上发布尾部更长,视为设计需要改。
- LIMIT-90-TEMP 仍在生效;kernels 相余量不足;tlib、tlib-prep 贴近 55 秒。
- cx-reaper 的 nproc 上限仍是固定余量 1000,自适应根治未做(DEBT-CX-REAPER-NPROC)。
- 产品的 _Static_assert 常量求值对 `~0UL >> 1` 按算术右移算,属编译器缺陷,另立项,未修。
- ROUTES(`9b107888db`)与 link-DCE(`f9a79c9dbd`)是 0.0.59 的产品草稿,不在本候选。
- check-release-usage 失败路径往 ROOT 写文件,未修。
- release-oneshot 真实执行入口、assetguard 的代码路径未在本次发布上使用;发布尾部按手册逐步手工执行。
- 沿 0.0.57,本版未处理:BASE6 字节 oracle 未重建;p3 的 release-ctor case 材料未重建;全域矩阵 release 档红格未回填。

## 撤回与回滚

撤回公开:`gh release edit v0.0.58 --draft=true -R partnernetsoftware/unisacc`;彻底撤回再删 tag:`git push origin :refs/tags/v0.0.58` 与 `git tag -d v0.0.58`。本机根回退:`sh ./unisacc.com tools/install-ape.cx -- ROOT <v0.0.57 公开 APE> c116a52ff62a427929334d728bcb7946860b120d3faebbb6ee0fdb2c3fa2d985 --write`。候选表回退到 0.0.57 记录:取 `git show 99fc0b48a2:release/candidate.tsv`。
