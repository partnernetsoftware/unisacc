# unisacc 0.0.76 发布说明

状态:同候选原生 CI run <CI_RUN_ID>(待回填)。整体裁决 NOT_QUALIFIED:全套 s9 本版未跑,产品资格域 574 批未整域实跑,严格证据只覆盖下列批次,不得称全批资格。

## 真实身份

| 对象 | 身份 |
|---|---|
| 候选点 T(整链输入) | d9903e5874ed9ccc7d0f2e6bd1d091a7683dc36f(整链 499/499 rc 0,22:00:31 到 22:25:12) |
| 冻结提交(发布标签目标) | debb8f43f9f111a59dcfccebe9c79da1e737269d(T 两次合入 origin/main 得 a2deed1d12,再加 release-exec 检查修复;相对 T 产品输入零改动,候选键 cfc17146 在 T 与冻结树相同) |
| 发布 APE SHA256(unisacc.com) | 8ffe6b0ba318be4bb77049f1b69165d8b3376ac7c6a917cd1969b5b39ef32e60(7243365 字节,比 0.0.75 的 7241315 多 2050 字节即 0.028%;--version 打印 unisacc 0.0.76;打包后与链产物同字节,PACK_APE SAME) |
| 源快照 SID | 1b2c0208ce9e55dd91854212ca4d63f77f5a30c212e855d196a9929043e95b98(VERIFIED) |
| CI run | <CI_RUN_ID> |

## 产品改动

(1) W2a:-funit 对象的进程槽 argc/argv 导出为具名 COMMON 符号 _unisa_proc_argc 与 _unisa_proc_argv,使多个 -funit 对象能合并进程状态(exec/enc 表与 exec/facts/enc-objectplan.tsv)。x86_64 的 lnx 对象有变化;整程序 -o 产物字节不变(17 份对拍)。arm64 的进程槽仍为匿名(只做了 x86)。
(2) modelcache 补丁(cx/runtime/modelcache.cx):UNISACC_CACHE_DIR 或 UNISACC_E3CACHE 设为 0 或空值时视为关闭缓存;相对路径会被拒绝,只打一行诊断。此前设 0 会在 cwd 建名为 0 的目录并写缓存。注意:设为空值的语义变了,现在是关闭。
(3) W2c:objlink 合并 COMMON(cx/driver/objlink.cx)。纯链接器,未接线,不在产品运行路径上。
(4) 开发工具链(不进产品):devsvc A-01 到 A-06 复核补丁、qualify 分段落盘、dvledger、dvreport、release-prep、release-exec 吸收缺口、release-packcheck(Q-07 版本槽门)、handoff-check 与 brief-lint、Q 系列、own-flow 原型(尚未接入产品,仅工具)、affected 显式原生 run-batch(默认关闭)。冻结后修了一次工具:debb8f43f9 修 check-release-exec 的 N0 在 qualify 布局(无 85a2d3242e 历史)下 die,改用入库夹具 tests/release-exec/old-release-run-85a2d3242e.txt,断言不变。只改了 tools 与 tests,产品输入零改动。

## 严格证据(本版首次前移到冻结前预跑)

(1) 预跑(origin=prefreeze,T 的 git archive 加单提交布局,候选 APE 8ffe6b0b,--jobs 2,外部 load1 在 6 到 8.6 之间),证据在 release/evidence/0.0.76/strict-prefreeze.tsv。sample 22 批:15 PASS、7 UNKNOWN;新增批加 10 片 tools-compile 共 39 批:31 PASS、1 FAIL(release-run-exec-gates rc 2,即上面那次修复的对象)、7 UNKNOWN。
(2) 复用为 0。冻结树相对 T 多了 main 上的 tools/affected.cx(批次运行器,每一批都要读它)、tests/batches.tsv、tests/pp-header-locations.cx、tools-compile 清单(157 片变 158 片)以及 tools/check-release-exec.cx 的改动,不满足复用条件 (c),所以全部预跑行都不复用,只留作首次冷跑记录。
(3) 冻结后(origin=postfreeze,冻结提交 debb8f43f9 的布局,同一 APE)一次跑 66 批:sample 22 批、本版新增与拆分批次、release-exec 三批、release-packcheck、own-escape、handoff-check、brief-lint、own-flow、own-flow-tree、tools-compile 共 21 片(含新增的 157 号)。结果 65 PASS、1 FAIL(c-negative 53/9,与上一版同批同数,列入 expected-fail);c-negative 与 pp-negative 首窗冷启动 UNKNOWN,批内重试后得到终态(pp-negative 10/0 PASS)。墙钟 515 秒。证据在 release/evidence/v0.0.76-strict-2026-11-18.tsv。

## 已知缺口与未验证范围

(1) s9 全套本版未跑;产品资格域 574 批未整域实跑;严格证据所列批次之外为 UNMEASURED。
(2) arm64 的进程槽仍匿名(W2a 只做了 x86_64);osx、win 的 -funit 未验;-S -funit 返回 NOT_IMPLEMENTED(0.0.71 起如此,不是回归)。
(3) unit-frontcache-equiv0 在已装产品和开发产品上同样失败(既有缺陷 UNIT-FRONTCACHE-EQUIV0-CX-DEPS-DIFF)。
(4) release-prep 棘轮批在冷缓存全新布局下 38.4 秒,占 55 秒限的 70%(Q-09)。
(5) -funit 对象与系统 ld/gcc 互操作的既有缺陷(nn-verify,release/evidence/nn-verify-2026-11-18.tsv;x86_64 Linux 复验,arm64 未验 UNMEASURED;不是本版引入):
  - NN-VERIFY-ELF-INIT:-funit/-c 对象里带初值的全局与 const(如 int x=5; const int k=32)被放进 .bss,没有 .init_array/.data/.rodata;交给系统 ld 链接后读到 0,属静默错误。
  - NN-VERIFY-WEAK:__attribute__((weak)) 符号发射为 GLOBAL 而非 WEAK,与 gcc 对象链接时报 multiple definition。
  - NN-VERIFY-FUNIT-SYSV:带参数的 -funit 对外函数从 %rax 取第一个参数,而不是 SysV 的 %rdi(0.0.75 修了无 main 单元的入口序言污染,无参函数返回正确,带参的仍错)。
  本版的 W2a 具名 COMMON 与 W2c objlink 是 unisacc 内部链接路径(objlink)的基础设施;把 -funit 对象交给系统 ld/gcc 链接并调用,仍不是受支持的互操作路径。
(6) UNKNOWN 处理:预跑的 14 个 UNKNOWN(首窗冷启动超 60 秒)没有复用,冻结后全部重跑并得到终态;没有 UNKNOWN 记成 PASS。
(7) release-prep 草稿把同一批的多行(重试前后)当成未覆盖,并把 count 字段带 ;outer-timeout-60s 后缀的行判为 NEW_FAIL(与 0.0.75 的已知缺陷相同);本说明与 expected-fail 已人工审阅改写。
(8) devsvc git commit 在合并冲突后收尾会丢掉 MERGE_HEAD 第二父(DEVSVC-COMMIT-DROPS-MERGE-HEAD,build/pkg-gaps.tsv),本次用原生 git commit 收尾(钩子照跑)。
(9) 诊断无行号或行号偏移(DIAG-LINE-OFFSET、S2-INLINE-REJECT-NO-LOCATION)仍开放;其余开放项见 build/pkg-gaps.tsv。

## 证据入口

release/evidence/0.0.76/INDEX.tsv;release/evidence/v0.0.76-strict-2026-11-18.tsv;release/evidence/0.0.76/strict-prefreeze.tsv;release/evidence/0.0.76/expected-fail.tsv;release/evidence/0.0.76/ratchets.txt;release/evidence/0.0.76/pack-run.out;release/evidence/0.0.76/cache-manager.tsv;release/evidence/0.0.76/time-ledger.tsv。撤回方案:release/rollback.txt(回退 Latest 到 v0.0.75,不删标签与 release)。
