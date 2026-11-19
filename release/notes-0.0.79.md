# unisacc 0.0.79 发布说明

状态:同候选原生 CI run <CI_RUN_ID>(待回填)。整体裁决 NOT_QUALIFIED:全套 s9 本版未跑,产品资格域未整域实跑,严格证据只覆盖下列批次,不得称全批资格。

## 真实身份

| 对象 | 身份 |
|---|---|
| 候选点 T(整链输入) | ea10ab075d931aa837be84936aad17b97caa26a7(整链 499/499 rc 0,06:10 到 06:46,一次通过无重启) |
| 冻结提交(发布标签目标) | f649bc050741f6ecda7d50d3fa03ffdc034420e0(相对 T 产品输入零改动:src cx ext include exec weights seed kernel 无差) |
| 发布 APE SHA256(unisacc.com) | b328ed1f582e21bc427cbda96a047de855c56abb2127ebaec288077ef1ee725d(7387189 字节,与 0.0.78 同长;--version 打印 unisacc 0.0.79;打包后与链产物同字节,PACK_APE SAME) |
| 源快照 SID | 5f973acb56e2e418a062e14133d5cc4cc011fd9b50b426f0dfc677e7d29a1965(VERIFIED,T 的链快照) |
| pkg.bin | 72df86d11d1e015832ff228e040c30c21d988926b75a4da950b3d66c168092d2(与 0.0.78 同字节) |
| CI run | <CI_RUN_ID> |

## 产品改动(只是 exec 表结构重构,无产品行为变化)

相对 0.0.78 公开冻结提交 8c4f101d1b,产品输入只改 exec/ 表。17 份 -o 产物与 0.0.78 公开件 17/17 逐字节相同;但网构造输入变了,所以候选重新整链。

(1) T2 第二步 d0/d1 共用块:objectplan-manifest 的 d1(arm64)改引用 d0 同名节(`rows objectplan d0.sN direct_labels`)并删 d1 副本行,共用 25 对配对节(d0.sN 对 d1.s(N+1):s3-s7、s9-s15、s18-s22、s24-s31);删 objectplan-result 167 行与 objectplan-byte 48 行,manifest 改 25 行,不改生成器与网读法。未共用:错位一号的 s2/s3、s8/s9 与已知分叉四对。字节等价证据:对象产出 68/68(lnx 两目标)加复核补测 osx/win 四目标与 -c -funit 15/15;6 个对象网 delta JSON 与 0.0.78 逐字节相同;manifest 63 行展开 765 行 HEAD 与工作树全等;check-objectplan-pair 自检 8 项 pairs=18 same=14 known_diff=4 new_diff=0 shared=12;notcovered 基线 sites 2->1;check-literal total 3019->2958。
(2) N1:librarycontract 与 librarypartition 终点 DONE 未定义致 cxlib/delta 拒载、体检等于没量,7 行 DONE -> HALT(seed/tbl.c 把未定义名解析成 -1 等同 HALT,delta 差异只有终点名,同族三张表无 DONE);体检接入后两网 reach 与 warnings 首次量到各 0->2。lex 三网 44 个 r 态 ERROR 源于 gen-manifest 声明域 [0,20] 而 table-lint 按 0..256 判;改 table-lint(工具,不进产品)只对可达 r 值判全,ERROR 44->2(剩 CF.finish 弹栈进入 r 不设,见已知缺口)。
(3) 独立复核:T2 可提交带条件无阻塞(9 项独立复现);N1 可提交带条件无阻塞(table-lint 值域保守,3 个负例仍报 ERROR)。
(4) N3(5 组疑似未写与 INC2.f 缺 SBCLR)定性:INC2.f 说法证伪,无真实漏写(A 类 2 组,C 类 3 组),不改产品,转工具线。

## 开发工具链(不进产品)

(1) register --tool 行级回滚、--runner、--sync dry。
(2) kit 预热。
(3) Q-10 准入门 load-wait-timeout。
(4) G1 末态判定:同批同候选后出现且带匹配 RAW 的 rc0 PASS 可取代先前 FAIL 行,输出 GATE 1 STRICT SUPERSEDED,不再手工删行;G1 未测项门(最终 UNKNOWN 逐批登记 expected-unmeasured.tsv)。
(5) dvqualify 证据写锁。
(6) Q-01 原生 run-batch 默认关闭。

## 严格证据

(1) 冻结前预跑(origin=prefreeze-strict,T2=ea10ab075d 布局,候选 APE b328ed1f,dvqualify --jobs 3,一次 --out 加一次 --resume):affected --files 8c4f101d1b..T2 的 43 个路径选 510 批,与 product-qualification 606 批取交集 202 批;末态 PASS 199、FAIL 1(预期)、UNKNOWN 2、INVALID 0。证据 release/evidence/0.0.79/strict-prefreeze.tsv。
  - FAIL c-negative count=53/9:与 0.0.76 同批同数,登记 expected-fail.tsv(见已知缺口)。
  - UNKNOWN cx-booter-ape、tools-default-regression:affected.cx 报 no-producer(booter-raw-elf、old-checker),材料无生产者,确定性,记 UNMEASURED,不记 PASS。
  - ape-need-refusal、c99-whitespace、c-negative 首轮 outer-timeout-60s(冷启动),行保留,--resume 后得终态。
  - 产品等价抽查:calc、dijkstra、mandel 用候选与 0.0.78 公开件 --target lnx/x86_64 -o 逐字节相同。
(2) 复用与补差:T2 -> F 的非白名单差只有 build/pkg-gaps.tsv。affected.cx --files build/pkg-gaps.tsv 选出 381 批,与预跑 202 批交集 164 批。
(3) 冻结后补差(origin=postfreeze,冻结提交 F 的布局,同一 APE,--jobs 2):164 批全部重跑(08:18-08:30,单次 --out,墙钟 768 秒),SUMMARY PASS 163、FAIL 0、UNKNOWN 1、INVALID 0。module-units 与 notcovered 首窗 outer-timeout-60s(同时进行 pack、N4 预热与 prepare 棘轮,load1 约 4-6),同进程段内重试 PASS,超时行保留;tools-default-regression 仍 UNKNOWN(rc 3 无 BATCH_RUN,与预跑同为材料无生产者),记 UNMEASURED。整合严格文件原样拼入预跑全部段再接补差段,无任何删减。证据 release/evidence/v0.0.79-strict-2026-11-19.tsv。
(4) 复用 38 批:预跑中不受 T2 -> F 差影响的批次,结果与 RAW 保留(含 c-negative 预期 FAIL 与 cx-booter-ape UNKNOWN)。
(5) G1 本版实战:FAIL 末态判定(superseded)与未测项门。expected-fail 登记 c-negative 53/9;expected-unmeasured 登记 cx-booter-ape、tools-default-regression,reason 均为 no-producer,各引用同批真实 DEFERRED 行(cba-deferred.tsv、tdr-deferred.tsv)。
(6) N4 读回预热 WARM READY(5 个目标 HIT)。release-exec --dry 末行 DRY_GATES_OK(G1 STRICT PASS rows=363 expected_fail=1)。棘轮:冻结提交 git archive 上 15/0。release-prep 末行 verdict=NOT_QUALIFIED(rc 1;c-negative 被 prep 列为 NEW_FAIL 是因 0.0.78 expected-fail 未列该批,本版 expected-fail 按 0.0.76 同批同数人工登记;uncovered 计入无 RAW 的超时行与 no-producer 行)。

## 整链

499/499 rc 0,06:10 到 06:46,一次通过无重启。链目录 /tmp/unisacc-session-v079-chain/ea10ab075d931aa837be84936aad17b97caa26a7。promote --check 通过。

## 已知缺口

(1) c-negative 53/9:产品接受 9 个应拒的 C99 负例(assign-to-const、implicit-function 等),0.0.76 起同批同数;0.0.77、0.0.78 严格证据未含该批。列入 expected-fail,不是本候选回归。
(2) cx-booter-ape 与 tools-default-regression 材料无生产者,UNMEASURED。
(3) CF.finish 弹栈进入 r 不设,r 未定值风险(LEX-CF-FINISH-R-UNSET-ON-POP)。
(4) DONE -> HALT 的网逐字节相同但未独立重建。
(5) objectplan pair 检查只覆盖 result 表(OBJECTPLAN-PAIR-CHECK-RESULT-TABLE-ONLY)。
(6) 共用 d0 节:改 d0 会同时改 arm64(COUPLING,写在 check-objectplan-pair 头注释)。
(7) FIX2 ENT3 sim --cov 对象网无配方,UNMEASURED(OBJECTPLAN-NET-SIM-COV-NO-RECIPE)。
(8) 外层 timeout 只杀 devsvc 外壳,dvqualify 孤儿继续写,被证据写锁兜住(DEVSVC-X-TIMEOUT-ORPHANS-DVQUALIFY)。
(9) 0.0.78 的 safe 静默漏判与误拒(OWN-FLOW-*、OWN-E1-*)未变;E3 与 moves 顺延;旧产品不认识 safe/owns/unsafe pragma,仓内 .cx 采用前不得再用旧 Root 引导。

## 未解决项(沿用)

(1) s9 全套本版未跑;产品资格域未整域实跑;严格证据所列批次之外为 UNMEASURED。
(2) -funit 对象与系统 ld/gcc 互操作的既有缺陷:NN-VERIFY-ELF-INIT、NN-VERIFY-WEAK、NN-VERIFY-FUNIT-SYSV。
(3) unit-frontcache-equiv0 deps=DIFF;UNIT-FRONTCACHE-EQUIV3-LOAD-DEPS-DIFF。
(4) -S -funit 返回 NOT_IMPLEMENTED(0.0.71 起如此,不是回归)。
(5) 其余开放项见 build/pkg-gaps.tsv。

## 证据入口

release/evidence/0.0.79/INDEX.tsv;release/evidence/0.0.79/strict-prefreeze.tsv;release/evidence/v0.0.79-strict-2026-11-19.tsv;release/evidence/0.0.79/expected-fail.tsv;release/evidence/0.0.79/expected-unmeasured.tsv;release/evidence/0.0.79/ratchets.txt;release/evidence/0.0.79/pack-run.out;release/evidence/0.0.79/cache-manager.tsv;release/evidence/0.0.79/warm-*.tsv。撤回方案:release/rollback.txt(回退 Latest 到 v0.0.78,不删标签与 release)。
