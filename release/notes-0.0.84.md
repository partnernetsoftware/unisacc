# unisacc 0.0.84 发布说明

状态:已公开(release-exec EXECUTED steps=7,公开读回 SHA256 与候选一致)。同候选原生 CI run 44647659602 一次通过:六目标 success,2026-11-20T00:39:59Z 到 00:47:15Z 约 7.5 分钟,无失败格重跑(0.0.83 曾因 osx/x86_64 stream-pipe 部件 e 超界重跑)。整体裁决 NOT_QUALIFIED:全套 s9 本版未跑,产品资格域未整域实跑,CI 六格只跑 version、hello、run、stdio-crossunit、stream-pipe、smoke 冒烟子集,不等于外目标资格,严格证据只覆盖下列批次,不得称全批资格。

## 真实身份

| 对象 | 身份 |
|---|---|
| 候选点 T(整链输入) | fa36c2e3bfef83fdb19e025c866a83ac900d9b1c(04:49 到 05:13 约 24 分钟,一次通过) |
| 冻结提交(发布标签目标) | df14f1e3345cec9e32e3d6f134c05fcaf1eb4dd6(F,即最终布局 T5,含 main 工具;相对整链输入产品输入零差异) |
| 发布 APE SHA256(unisacc.com) | 3a226945354604440e4392d85c54548f92e9f19a3e73ebc3b263c67daef9c7db(8417722 字节,比 0.0.83 的 7399573 多 1018149 字节,主要是 F1 七个模型成员改存原始表示使模型包增大;--version 打印 unisacc 0.0.84;打包后与链产物同字节,PACK_APE SAME) |
| 源快照 SID | d681393cb452a7dd8e848619bb5d964fe3828fadeed3287fde87d91587a03882(VERIFIED,T 的链快照 fa36c2e3bf-36dc4e25,identity-helper-real5) |
| pkg.bin | 01fa8893715f580c87ee21d41226a1be1b983bae09d3fdde7fe99f3b9f267d87 |
| slice-win-arm64 | 563759cfba57d20bd006c5c0274374a328331c69928af8931ca6c968f4e4e64d |
| SHA256SUMS | f64d35f4c05487a51e8e445b372aa277510c162d41dd8c33a71c2ae807631c96 |
| CI run | 44647659602(六目标 success,一次通过,候选 3a226945,commit 列为阶段 A F2=c9995131860cfc17cbc6672cd06e5ec5dc2a9d7c;release-exec Y1=f708c82ddd,Y2=f65c432114) |

## 产品改动

产品输入相对 0.0.83 冻结提交 42d1f989d3 为:cx/driver/cli.cx(F3)、cx/runtime 的 package_decode、package_lifetime、packagestruct、routestep 与 src 的 executor、packmodel、stages 及 exec/c/package_decode_host.h(F1),另有 ext/sysx/linuxns.cx 新增(agy-cage 的 Linux 命名空间库,落库,本版产品路径不调用);seed、weights、kernel、include 零改动。

(1) F1 首次编译的模型装载:打包器按流水线阶段名定位七个热模型,将已解压的二进制网以只读原始格式存放;读取器借用包内字节,仍核长度、CRC 与网签名再完整解码;其余 33 个模型沿用压缩表示。自举仍用原包,只在最后打包相转换格式。
  - 实测(Linux x86_64,A 为 0.0.83 公开件,B 为含 F1 读取器的私有 APE,每侧每输入七次交替,新应用缓存,启动前驱逐 APE 文件页并经 fincore 核驻留 0,中位数 ms):hello.c 装载 93.8 -> 46.3,总墙钟 522.1 -> 487.0;hello.cx 装载 94.2 -> 42.9,总墙钟 623.7 -> 557.7。
  - 未达与负收益:14 对样本装载均变快,但两类输入各有至少一轮总墙钟变慢(最差一对 hello.c 慢 25.2 ms,hello.cx 慢 10.1 ms);热态 hello.cx 命中运行缓存墙钟中位 8.4 ms,未达 7.5 ms 目标。
  - 字节与拒绝:五个输入 -o 产物与公开版逐字节相同;对七成员的载荷、长度、codec、CRC 与包尾构造的 30 个损坏样本全部具名拒绝,无崩溃。netregen 对 0.0.83 公开件 60/60 格 SAME(默认档 30、优化档 30,含 module 12 格),故本版镜像指纹预期差异登记为 0。
  - 平台:Linux x86_64 原生运行,Linux arm64 经 QEMU;macOS 与 Windows 原生执行待 CI。
(2) F3 裸 -I include 规范化(cx/driver/cli.cx):原先裸相对 -I 目录被静默忽略;补丁自带复现对 0.0.83 公开件红 4/8,候选 8/0。不改变编译产物字节。

## 开发工具链(不进产品)

(1) S1 生成器总闸 tools/gen-all-check.cx 接入起链(chain-ready 前置 15 条)与提交预检。三次复核教训:门在真实暂存索引树上执行并能找到已装产品;只有生成物确实漂移才以 DIFF 阻塞,工具不可用或超时保留 ERROR 原文;保留全量 42 个生成器实跑断言并另验真实提交路径;全树检查后续热修为 21 加 21 分片并与 drift 预检时限解耦。起链前 42/42 通过,另列两个既有 uncovered。
(2) S3 stream-pipe 拆分:原 main、e 两组拆为 11 个部件(basic、stdio、multi、reject、e-ref、e-0 到 e-4、e-compare),13 个断言多重集不变,每部件界限仍 55 秒不放宽;CI 首轮(run 44647659602)最慢部件为 osx/x86_64 multi 26829 ms,达到不高于 40 秒的目标;各格明细见已知缺口 S3 观测一节。
(3) R1 release-flow r2:阶段 A 备料与干跑分相,核严格证据段顺序、机器列与拆分父批覆盖。
(4) R2 第一半:送审三块(RAW 身份字段、真实程序缺口调研、闭包采集器原型);本版不据其建议免跑。
(5) R3 真实提交预演:私有仓以完整 main 与实际钩子执行拟提交补丁(tools/check-real-commit.cx),real-commit 两相(布局内 prebuild-self 后单跑)进入严格证据。
(6) linuxns 落库(ext/sysx/linuxns.cx,agy-cage);G1 镜像指纹机制保留,本版使用 0 条。
(7) 脚本基准与冷启动剖面(0.0.83 期间采,执行器为 0.0.82 公开件):热态与六个任务 cx 墙钟低于 Node 与 Python,Bun 在热 hello、JSON 与 glob 上更快;冷 hello 616 ms 中推理约 76%(E3 48.8%、lower 14.0%)。F1 后冷 hello.cx 557.7 ms,仍约为 Node 冷启动(42.6 ms)的 13 倍,本版打不赢 node 冷启动。
(8) 文档整理:ARCHITECTURE.md -> prd_arch.md,spec.md -> prd_spec.md(旧名留符号链接)。
(9) 新增批次在资格布局暴露的隐含依赖修复(依赖 git 历史、依赖环境变量、时限贴界)。教训:新增批次必须在单提交布局自测后再登记。

## 严格证据

(1) 预跑三次,不是一次通过:第一次因新增批次在单提交资格布局暴露隐含依赖停手修复;第二次 05:20 到 06:20 停手于 F1 漏改工具侧消费者(streampack 等);第三次 07:50 到 08:30 在 T5=df14f1e334 布局全量重跑 421 批、51 个窗口(50 窗加补跑窗),不复用旧 RAW。
(2) 拼合:release/evidence/v0.0.84-strict-2026-11-20.tsv(1152 行,sha256 6db56ea6e860...)为各窗原样拼接,不删不改,全部 candidate=3a226945...。
(3) 421 批:PASS 410、FAIL 1、UNKNOWN 10、INVALID 0。
  - FAIL c-negative 53/9:0.0.76 起已知,产品接受 9 个应拒的 C99 负例,非本候选回归;expected-fail.tsv 续期 expires=0.0.85。expected-fail.tsv 无 image= 行(netregen 指纹登记到期清零,12 格 module 全 PASS)。
  - UNKNOWN 10 个均为 no-producer,记 UNMEASURED 不记 PASS,expected-unmeasured.tsv 引用本次真实 DEFERRED 行:cx-booter-ape、tools-default-regression、opt-frame4-o1-negative-end、opt-frame4-o1-negative-eof、opt-frame4-o1-negative-label、opt-frame4-o1-positive、opt-frame4-o2-negative-end、opt-frame4-o2-negative-eof、opt-frame4-o2-negative-label、opt-frame4-o2-positive。
  - SUPERSEDED 2:notcovered 与 unit-frontcache-equiv3 的前段 FAIL 由同候选后段 PASS 取代;unit-frontcache-equiv3 以 jobs=1 单跑 54.0 秒,贴着 55 秒界限,时限风险如实登记。
  - 拆分父批 8 分片全部 PASS;netregen 60/60 SAME;real-commit 两相 PASS。
(4) 打包、预热与干跑:release-run --pack PACK_OK,SOURCES_SID VERIFIED,PACK_APE SAME;N4 WARM READY(5 个目标);release-exec --dry DRY_GATES_OK steps=7 deferred=6,G1 STRICT PASS rows=415 expected_fail=1 superseded=2(与私有演算一致),CANDIDATE_SHA、APE_VERSION unisacc 0.0.84、RATCHETS 15/0(真实检出 live-HEAD)、MAIN_ANCESTOR、ASSETS 六项、DECISION 均 PASS。

## 整链

整链输入 fa36c2e3bf,04:49 到 05:13 约 24 分钟,一次通过。冻结 F=df14f1e334 为合入 main 工具的最终布局 T5,产品输入相对整链输入零差异(差异只在 build、tests、tools、plans、ujs)。链目录 /tmp/unisacc-session-v084-chain/fa36c2e3bfef83fdb19e025c866a83ac900d9b1c。

速度账(tools/speed-ledger.cx --version 0.0.84 实跑,阶段 A 时):槽到起链 187.1 分钟;整链出口 211.1 分钟(整链 1/1 次,链效率 100%,链并行度 2.52);预跑并行度 1.43;选批 421/722(58.3%);计划单元 9;起链后提交 10;预跑完成与公开时点工具报 UNMEASURED,公开后重取。
公开后重取(同工具实跑,2026-11-20):槽到起链 187.1 分钟,整链出口 211.1 分钟(整链 24.0 分钟),预跑三次全部完成 416.5 分钟,槽到公开 425.6 分钟,整链出口到公开 214.5 分钟(含预跑三次与阶段 A);整链 1/1,链效率 100%,链并行度 2.52,预跑并行度 1.43,选批 421/722(58.3%);planned_units 9,done_units 与 unplanned_units 工具当时报 UNMEASURED(本次在 plans/v0.0.84.md 回填 6 与 6),throughput 工具报 UNMEASURED。

## 已知缺口

(1) S3 观测(取自 gh run view 44647659602 --log,部件界限 55 秒未放宽,全部部件 rc=0 status=OK):

| 格 | STREAM_PREPARE | stream-pipe 总 ms | 部件 ms(basic stdio multi reject e-ref e-0 e-1 e-2 e-3 e-4 e-compare) | 最大部件 |
|---|---|---|---|---|
| lnx/x86_64 | rc=0 3854 ms PREPARED | 7080 | 260 543 172 80 480 324 327 318 316 325 78 | stdio 543 |
| lnx/arm64 | rc=1 6530 ms FALLBACK | 85680 | 6835 7521 6915 6976 7676 7128 7284 7369 7303 7385 6757 | e-ref 7676 |
| osx/arm64 | rc=1 15100 ms FALLBACK | 178677 | 14962 14771 15260 13601 18789 16527 14876 12576 14422 13817 13970 | e-ref 18789 |
| osx/x86_64 | rc=1 26129 ms FALLBACK | 292372 | 23458 21613 26829 25685 20372 22981 24410 23084 25932 26360 25512 | multi 26829 |
| win/x86_64 | 不适用 | UNMEASURED | 便携子集 6/9 子检查 PASS,POSIX 部分 NOT_RUN(缺 fork、waitpid、kill、F_SETFD、environ、F_SETFL) | UNMEASURED |
| win/arm64 | 不适用 | UNMEASURED | 同 win/x86_64 | UNMEASURED |

  - osx/x86_64 是 0.0.83 首次失败的格:本版拆分后最大部件 26.8 秒,余量约 28 秒,原缺口 STREAM-PIPE-OSX-X86_64-55S-BOUND-THIN-MARGIN 的部件层余量问题缓解,但总时 292 秒偏长。
  - 预编译回退:三个非 lnx/x86_64 的 POSIX 格 STREAM_PREPARE 均 rc=1 FALLBACK(不止 macOS,lnx/arm64 同样),各部件各自冷编译,使总时偏长。登记缺口 STREAM-PREPARE-OSX-FALLBACK(build/pkg-gaps.tsv),建议 0.0.85 查预编译失败原因,并让 FALLBACK 不重复冷编译。
(2) cx-dev-a 到 cx-dev-e、test-loader-com、loader-net-replacement、kernel-base、logic-distribution 不在严格域,本版未测。
(3) check-package-decode-real 的归档 C 参照缺 core.h;old-product-material 缓存键不含布局路径;dvqualify --resume 多 SUMMARY 与 G1 的配合(DVQUALIFY-RESUME-MULTI-SUMMARY-VS-G1);real-commit 冷备料非自动(REAL-COMMIT-COLD-PREBUILD-NOT-AUTOMATIC);derive 全量 --check 约 54 秒贴界限;unit-frontcache-equiv3 单跑 54.0 秒贴界限。以 build/pkg-gaps.tsv 登记为准。
(4) F1 总墙钟有负收益轮次,热态未达 7.5 ms;冷启动仍约为 Node 13 倍。
(5) c-negative 53/9 续期(expires=0.0.85)。
(6) 全套 s9 本版未跑;产品资格域非整域,严格证据所列之外为 UNMEASURED;外目标执行格仍 UNVERIFIED。

## 未解决项(沿用)

(1) NN-VERIFY-WEAK。
(2) -S -funit 返回 NOT_IMPLEMENTED。
(3) 其余开放项见 build/pkg-gaps.tsv。

## 证据入口

release/evidence/0.0.84/INDEX.tsv;release/evidence/v0.0.84-strict-2026-11-20.tsv;release/evidence/0.0.84/expected-fail.tsv;release/evidence/0.0.84/expected-unmeasured.tsv;release/evidence/0.0.84/*-deferred.tsv;release/evidence/0.0.84/pack-run.out;release/evidence/0.0.84/cache-manager.tsv;release/evidence/0.0.84/warm-*.tsv。撤回方案:release/rollback.txt(回退 Latest 到 v0.0.83,不删标签与 release)。
