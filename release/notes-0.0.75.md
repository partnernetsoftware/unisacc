# unisacc 0.0.75 发布说明

状态:同候选原生 CI run <CI_RUN_ID>(待回填)。整体裁决 NOT_QUALIFIED:全套 s9 本版未跑,严格证据只覆盖下列批次,不得称全批资格。

## 真实身份

| 对象 | 身份 |
|---|---|
| 候选点 T(整链输入) | 0e5ea32afc86c29652c765db3c78324f8bff6129 |
| 冻结提交(构造输入、发布标签目标) | fe6dfb32cfea0dcea33df4974dbf861c425230f6(T 合入 origin/main 的合并提交;相对 T 产品输入零改动,候选键 2427c25c 相同) |
| 发布 APE SHA256(unisacc.com) | fc57bf5b5545b9b40c58734369a0b9c31ceec071040dde572ae0093279d147b6(7241315 字节,--version 打印 unisacc 0.0.75) |
| 源快照 SID | f3c876326104ad6c27310f34ccd947b3f8bca9f6c94422c4cc08c7b0d3f815f4(VERIFIED) |
| CI run | <CI_RUN_ID> |

## 产品改动

(1) 工单 1:-funit 编译的无 main 单元不再把进程入口序言发进第一个函数偏移 0 处。既有缺陷:把 -funit 对象交给系统 ld 链接并调用该函数时,序言会把返回地址写进 argc 格。tools/check-funit-noprologue.cx 在本版 APE 上 3/3 PASS;在 v0.0.74 公开 APE(63334e18)上 failed=2(nomain-one、nomain-two 首 8 字节 4889e4488b042448),作负控。三入口:default-run rc 0;-c -funit 出 ELF 可重定位对象;-S -funit 返回 NOT_IMPLEMENTED(0.0.71 起如此,不是回归)。
(2) R2 基础设施:单元对象键 unitobj、预制闭包对象 closure-obj、objlink 纯链接器。均未接线,不在产品运行路径上。
(3) profile 一期:perf-hot、perf-gate 回归门工具。
(4) 开发工具链(不进产品):devsvc A-01 到 A-05b(提交门 CAS、启动派生、native 去阻塞、常驻关系图、写锁绑定进程身份);qualify 分段落盘与续跑、dvledger、dvreport、release-prep、release-exec 吸收 v0.0.74 缺口。冻结后修复一次工具:92e49cac83 修 release-prep 在 qualify 布局(无历史)下失败,只改 tools,产品输入零改动。

## 严格证据

证据 release/evidence/v0.0.75-strict-2026-11-18.tsv(devsvc qualify,候选 APE fc57bf5b,冻结提交 git archive 布局),四段:
(1) sample 22 批(含随机 tools-compile 10 片,seed 75):外部负载 7 到 9 下跑三轮,终态 21 PASS、1 FAIL(c-negative 53/9,与上一版同批同数,列入 expected-fail);pp-negative 10/0。
(2) 本版新增与相关批次 24 批:funit-noprologue、unitobj、closure-obj、perf-hot、perf-gate、perfev、release-prep、release-prep-neg、loadavg-lib、dvledger、check-struct-member-capacity 与 struct-member-capacity 七批、release-exec、release-exec-ystep、release-run-exec-readback、tools-compile-112/116/135。22 PASS;unitobj、release-exec 首轮超窗 UNKNOWN,低载重跑 PASS(unitobj 21/0)。
(3) release-prep 0/13、release-prep-neg 2/9 曾 FAIL(原因:qualify 布局是 git archive 加单提交私有仓,没有 0.0.74 提交历史),原行保留并标 superseded-by;修后工具 92e49cac83 在同布局对候选 APE 补跑 13/0、11/0 PASS,受影响的 tools-compile-066、151 补跑 PASS。

## 已知缺口与未验证范围

(1) s9 全套本版未跑;产品资格域 574 批未整域实跑;affected 选批未全跑;严格证据所列批次之外为 UNMEASURED。
(2) arm64、osx、win 的 -funit 路径未验;-S -funit 返回 NOT_IMPLEMENTED(0.0.71 起如此,非回归)。
(3) 贴窗批次:pp-cli-includes、pp-header-locations、tools-compile-117 连续两轮 UNKNOWN(load 大于 5,55 秒窗内无 BATCH_RUN 或 60 秒外层超时),第三轮低载 PASS;unit-frontcache-budget 54.7 秒、c99-whitespace 55.0 秒、tools-compile-001/008 约 53.7 秒,贴近 55 秒窗口。时限未放宽。
(4) release-prep 批次在 qualify 布局曾因无历史 FAIL,已修(92e49cac83),补跑 PASS。release-prep 草稿对多段拼接的严格证据解析不全(把多段同批行当未覆盖,count 字段带 ;load_wait 后缀会误判 NEW_FAIL),本说明与 expected-fail 已人工审阅改写。
(5) devsvc qualify 续跑时出现 spawn-failed UNKNOWN(tools/dvqualify.cx:172),重试后有终态。
(6) 诊断无行号或行号偏移(DIAG-LINE-OFFSET、S2-INLINE-REJECT-NO-LOCATION)仍开放;其余开放项见 build/pkg-gaps.tsv。
(7) 冷缓存口径(UNISACC_RUN_CACHE=0 UNISACC_UNIT_CACHE=0)下 check-release-prep 正向半在全新布局超过 58 秒(release-prep 冷编约 40 秒加 15 个棘轮程序),默认缓存口径 23 到 32 秒。

## 证据入口

release/evidence/0.0.75/INDEX.tsv;release/evidence/v0.0.75-strict-2026-11-18.tsv;release/evidence/0.0.75/expected-fail.tsv;release/evidence/0.0.75/ratchets.txt;release/evidence/0.0.75/pack-run.out;release/evidence/0.0.75/funit-noprologue.txt;release/evidence/0.0.75/time-ledger.tsv。撤回方案:release/rollback.txt(回退 Latest 到 v0.0.74,不删标签与 release)。
