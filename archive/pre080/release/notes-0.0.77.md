# unisacc 0.0.77 发布说明

状态:同候选原生 CI run 44363578812(六目标 success,01:53 派发,02:06 通过)。整体裁决 NOT_QUALIFIED:全套 s9 本版未跑,产品资格域未整域实跑,严格证据只覆盖下列批次,不得称全批资格。

## 真实身份

| 对象 | 身份 |
|---|---|
| 候选点 T(整链输入) | 41cdb4e716f4171d8213339923979bfb3533c63f(整链 499/499 rc 0,23:59:56 到 00:32:28) |
| 冻结提交(发布标签目标) | 6136f6c30ed4090a5308748383de39b79dfccc96(相对 T 产品输入零改动:src cx ext include exec weights seed kernel 无差,候选键相同) |
| 发布 APE SHA256(unisacc.com) | ad6a096c98a7fac6e8976e007b0b9a18c5f62996039b391fb9894a832c0cb9ab(7327477 字节,比 0.0.76 的 7243365 多 84112 字节;--version 打印 unisacc 0.0.77;打包后与链产物同字节,PACK_APE SAME) |
| 源快照 SID | d2308587453b7a0eaf1633cd8905cdbcb9e3f3edbb5b4d7c6d7e6b231fffb5c4(VERIFIED,T 的 git archive) |
| pkg.bin | 72df86d11d1e015832ff228e040c30c21d988926b75a4da950b3d66c168092d2(与 0.0.76 同字节,即链产物 compiler.p4.pkg 未变) |
| CI run | 44363578812 |

## 产品改动(cxmod 内存安全乙期一段,全部 opt-in)

(1) P1:`#pragma unisa owns NAME...` 认声明并校验。拒绝文案 `unisa owns hid: no non-static function hid in this unit`、`unisa owns takes a function name`。E1(缺声明拒绝)留 0.0.78。
(2) P5:`#pragma unisa safe`(整单元)与 `#pragma unisa unsafe NAME...`(函数级豁免),加编译期静态拒绝:E2 释放后使用(`unisa safe: use after free of NAME in FUNC`)与 E2b 重复释放(`unisa safe: double free of NAME in FUNC`)。
(3) 未声明 safe 的单元字节不变:17 份 -o 产物与 0.0.76 sha256 17/17 相同。
(4) 兼容性:旧产品(0.0.76)不认识这三个 pragma,声明它们的源码会被旧产品拒绝(`unknown unisa pragma`)。仓内 .cx 采用这些 pragma 之前,不得再用旧 Root 引导。
(5) 验证:own-flow 夹具产品侧检查 passed=47/47 failed=0,KNOWN-LIMIT 3,unsafe=1;对 v0.0.76 发布件 failed=47(先失败后通过)。srcshape 5780/5854,未抬基线。三源 CXMOD_TRI passed=38 failed=0。

## 已知缺口(独立复核,均已登记 build/pkg-gaps.tsv 的 OWN-FLOW-*)

(1) 遮蔽清零:内层同名变量遮蔽时状态被清零,可能漏判。
(2) 词法失败静默跳过整单元:分析器词法失败时整单元不检查,也不报。
(3) 容量上限静默漏判:第 65 个变量、第 33 个标签、goto 只走 2 遍、循环嵌套超 64 层,超出部分不检查。
(4) 误拒两类:无括号 sizeof、条件释放(路径不敏感)。
(5) 每个单元只报第一个发现。
(6) 别名、字段、全局、函数指针仍漏判(夹具里的 KNOWN-LIMIT 3 行)。

## 开发工具链(不进产品)

(1) Q-01 原生 run-batch(默认关闭)补丁。
(2) N3 release-exec 主线祖先门(三步前)与 N4 读回预热。
(3) N6 devsvc git commit --merge-continue 保留双父合并提交。N3/N4 与 N6 两项均经独立复核:可发布无阻塞,三条低中项留 0.0.78。
(4) P1 P2 P3 表网卫生基线工具:check-net-health、check-objectplan-pair、table-cover。
(5) dvledger def_sha 带 runner。

## 严格证据

(1) 冻结前预跑(origin=prefreeze-strict,T2=f7f460d3a8 布局,候选 APE ad6a096c,dvqualify --jobs 3):194 批 PASS 190、FAIL 1、UNKNOWN 3、INVALID 0。证据 release/evidence/0.0.77/strict-prefreeze.tsv。
  - FAIL tools-compile-107:tools/dv/timing.cx 单编时 net.unix 无处解析,根因是仓根 cxroot.tsv 缺 net 行(顶层靠 tools/net 符号链接)。已装产品同样失败,不是候选回归,属工具源/模块解析的已知债;冻结提交里已在 tests/tools-compile-expected.tsv 登记为精确已知失败(过期会自动报错),修复留 0.0.78(改 cxroot.tsv 会进构造输入)。
  - UNKNOWN cx0-e2、cx-booter-ape、tools-default-regression:affected.cx 报 no-producer,材料没有生产者,结果是确定性的,记 UNMEASURED,没有记成 PASS。
(2) 复用与补差:T2 -> F 的差含 build/doc-links.tsv、build/pkg-gaps.tsv、tests/tools-compile-expected.tsv,不在复用白名单内。affected.cx --files 选出受影响 380 批,与预跑 194 批交集 166 批(159 片 tools-compile 加 commit-merge、module-units、notcovered、run-batch-identity、table-cover、tools-default-regression、tools-inventory-boundaries)。
(3) 冻结后补差(origin=postfreeze,冻结提交 F 的布局,同一 APE,--jobs 2):166 批全部重跑,结果 165 PASS、0 FAIL、1 UNKNOWN、0 INVALID。段一 17 批(commit-merge、module-units 首窗 60 秒 UNKNOWN,load1 约 7 到 10,--resume 后 PASS;tools-compile-107 在 F 上 PASS,已知债登记生效);段二 149 片 tools-compile 全 PASS,墙钟 467 秒。tools-default-regression 仍 UNKNOWN(rc 3 无 BATCH_RUN,与预跑同为材料无生产者),记 UNMEASURED。证据 release/evidence/v0.0.77-strict-2026-11-19.tsv。
(4) 复用 28 批:预跑中不受 T2 -> F 差影响的批次,PASS 行与 RAW 保留在 strict-prefreeze.tsv(其中 cx0-e2、cx-booter-ape 为 UNKNOWN,记 UNMEASURED)。
(5) 合计本版严格证据覆盖 194 批:PASS 191、FAIL 0、UNKNOWN 3(UNMEASURED)。expected-fail 本版为空(c-negative 不在本版严格集内,UNMEASURED,不沿用 0.0.76 的 53/9)。
(6) 棘轮:冻结提交 git archive 上 15/15 ok(srcshape 5780/5854)。N4 读回预热 WARM READY(5 个目标 HIT)。release-exec --dry 末行 DRY_GATES_OK。

## 整链

499/499 rc 0。守卫因 /tmp/.git 间歇出现拒绝三次(chain-step 报 batch stage parent lies in repository /tmp,疑似其他窗口的探针,未定位),第四次通过并续跑完成,证据见链目录 bg.log。

## 未解决项(沿用 0.0.76)

(1) s9 全套本版未跑;产品资格域未整域实跑;严格证据所列批次之外为 UNMEASURED。
(2) -funit 对象与系统 ld/gcc 互操作的既有缺陷:NN-VERIFY-ELF-INIT(带初值全局进 .bss,静默读 0)、NN-VERIFY-WEAK(weak 发射为 GLOBAL)、NN-VERIFY-FUNIT-SYSV(带参对外函数从 %rax 取第一个参数)。
(3) unit-frontcache-equiv0 deps=DIFF(既有缺陷 UNIT-FRONTCACHE-EQUIV0-CX-DEPS-DIFF)。
(4) release-prep 棘轮批冷缓存全新布局 38.4 秒,占 55 秒限的 70%(Q-09)。
(5) -S -funit 返回 NOT_IMPLEMENTED(0.0.71 起如此,不是回归)。
(6) 其余开放项见 build/pkg-gaps.tsv。

## 证据入口

release/evidence/0.0.77/INDEX.tsv;release/evidence/0.0.77/strict-prefreeze.tsv;release/evidence/0.0.77/strict-postfreeze.tsv;release/evidence/0.0.77/expected-fail.tsv;release/evidence/0.0.77/ratchets.txt(冻结提交 git archive 上 15/15 ok);release/evidence/0.0.77/pack-run.out;release/evidence/0.0.77/cache-manager.tsv;release/evidence/0.0.77/warm-*.tsv(N4 读回预热 READY)。撤回方案:release/rollback.txt(回退 Latest 到 v0.0.76,不删标签与 release)。
