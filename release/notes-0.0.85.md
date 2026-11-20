# unisacc 0.0.85 发布说明

状态:已公开(release-exec EXECUTED,Y1=99eab18166,Y2=3b0a5ec143,公开读回 SHA256 与候选一致;公开时间 2026-11-20T05:28:22Z)。同候选原生 CI run 44680404617 一次通过(run_attempt=1):六目标 candidate 格 success,2026-11-20T05:20:43Z 到 05:28:01Z 约 7.3 分钟,CI 资格只覆盖这六格的冒烟子集,不等于外目标资格。整体裁决仍 NOT_QUALIFIED:全套 s9 本版未跑,产品资格域未整域实跑,严格证据只覆盖下列批次,严格证据所列批次之外为 UNMEASURED,不得称全批资格。

## 真实身份

| 对象 | 身份 |
|---|---|
| 候选点 T(整链输入) | e75c162dfffc07bdbfafd371317ae9e65fa2e05e(链目录 /tmp/unisacc-session-v085-chain/e75c162dfffc07bdbfafd371317ae9e65fa2e05e) |
| 冻结提交(发布标签目标) | f6c688d2cb363ff64677e572b747ee68e1c06481(F;相对 T 产品输入 src cx ext seed weights kernel include exec 零差异,已核) |
| 发布 APE SHA256(unisacc.com) | 751a40ac556e4a1d617a15dea3975cb3951edec6387e46fb84de6b41f9435324(8425614 字节,比 0.0.84 多 7892 字节;打包后与链产物同字节,PACK_APE SAME) |
| 源快照 SID | 2191773d956297be687e108b94837cfda16a25ce275b9b2db1574a3726cd878d(VERIFIED,链快照 e75c162dff-1c3bd0cf,identity-helper-real5) |
| pkg.bin | 5b26e4ee811e63fc4b12a0ea63364bb0c308aa886aaf3199875edd8bfe9c21aa |
| slice-win-arm64 | 013c5ae7741d9a44d8c1cfd54fc38f9e662869caaa51eeb421ea9bf5014498d1 |
| SHA256SUMS | 85bb4fb5a601b69c3668b7bf1522219a83879838ddfa7f0d1d070fb1bfa5947d |
| cache-manager | 沿用 0.0.73 起的 x64 d34bee6c、arm64 0aae3fbc(EXPLICIT_INPUTS) |
| CI run | 44680404617(六目标 success,一次通过,headSha 为整链输入 T=e75c162dff,候选 751a40ac;release-exec Y1=99eab18166,Y2=3b0a5ec143;证据 release/evidence/0.0.85/ci-44680404617.json 与 .log) |

## 产品改动

产品输入相对 0.0.84 冻结提交 df14f1e334 只改 exec 表(parse2、facts、pipeline 共 23 个文件)与 src/drive.cx 一行;seed、weights、kernel、include 零改动。主题为前端诊断三期。

(1) F1 局部指针初始化:c-negative 从 53/9 收至 55/7。默认编译对非零整数隐式初始化指针、以指向不同宽度整数类型的一级指针作不相容初始化,给出带文件、行、列的错误;空指针常量、显式转换与合法 void 指针互转保留正控。
(2) F2 拒绝位置:按计划发码点口径,26 个原缺位置项补 25 个;产品线集成样本 53 次实际拒绝中 51 次带文件、行、列。未定义 goto 标签在引用处报错,合法前向跳转仍可编译;负数组界在声明处报错。
(3) F3a 告警升级:-Wall -Werror 把已触发告警升级为带位置的错误,返回非零并不产出对象或镜像。
(4) F3b 分支提示:__builtin_expect 与 __builtin_expect_with_probability 只求值第一实参一次并返回其值;限制见已知缺口。可执行镜像已与宿主 GCC 比对退出码与标准输出;-c 路径只核编译退出码(对象带 _start,宿主无法同样链接)。

## 开发工具链(不进产品)

(1) S1 新批次资格布局自测门、S2 预跑驱动 tools/prerun-drive.cx 入库、S4 unit-frontcache-equiv3 拆分,均在槽前落 main。
(2) Q2 opt-frame4 八批补生产者:本版严格证据中有 producer,首轮部分 UNKNOWN rc=3 后同段重试 PASS,不再列 expected-unmeasured。
(3) flow 系 load1 负载门废除(项目所有者 2026-11-20):flowadmit 与 flowexec 不再按 load1 推迟或减半。
(4) devsvc git commit、commit-own 提交前剔除署名行,新增 commit-msg 钩子工具。

## 严格证据

(1) 预跑 prerun085 在 dvqualify 单提交布局(T 树)跑 435 批(0.0.84 的 421 批加 14 个新批),174 段逐窗原样拼接为 release/evidence/v0.0.85-strict-2026-11-20.tsv(1752 行,sha256 55cd08c39d40...),不删不改,全部 candidate=751a40ac...。real-commit 两相:布局内 prebuild-self PREPARED 后单跑 PASS。
(2) 435 批:PASS 432、FAIL 1、UNKNOWN 2、SUPERSEDED 3、INVALID 0。
  - FAIL c-negative 55/7:剩余七例见已知缺口,expected-fail.tsv expires=0.0.86。本版计划原裁定 c-negative 不再续期、须清零到 62/0;未达,改为逐例登记到期 0.0.86 并在此如实写明计划未达。
  - UNKNOWN 2(no-producer,记 UNMEASURED 不记 PASS):cx-booter-ape(booter-raw-elf)、tools-default-regression(old-checker),expected-unmeasured.tsv 引用本次真实 DEFERRED 行。
  - SUPERSEDED 3:slow-split(同窗 jobs=3 时 55027 ms transport=-1 窗内超时,run-batch.cx 第 209 行判 FAIL;单跑 15599 ms PASS);unit-frontcache-equiv0 与 equiv1(首轮 rc=1 约 40 秒,dvqualify 未留原始输出,未定因;单跑、成对 -j 2 与重跑均 PASS)。
  - netregen 全 SAME,无镜像指纹登记。
(3) 预跑工具偏差:W0073-02 起的普通窗所用 tools/dvqualify.cx 取自 main ecb8cb246f(sha256 48a5fa2e2bead3d8f734d96a993e730520dbb607a0694d1892eb906f49c9aa57),而非 T 树;原因是 load1 门被项目所有者废除,门只决定发车时机不决定结果;其余模块取自 T 树,未遇身份核验拒绝。W0000 到 W0072 与 W0073-01 用 T 树 dvqualify。
(4) 打包、预热与干跑:release-run --pack PACK_OK,SOURCES_SID VERIFIED,PACK_APE SAME;N4 WARM READY(5 个目标);release-exec --dry DRY_GATES_OK steps=7 deferred=6,G1 STRICT PASS rows=436 expected_fail=1 superseded=3,CANDIDATE_SHA、APE_VERSION unisacc 0.0.85、RATCHETS 15/0(live-HEAD)、MAIN_ANCESTOR、ASSETS 六项、DECISION 均 PASS。

## 已知缺口

(1) c-negative 剩余 7 例(产品接受应拒的 C99 负例),全部排入 0.0.86:assign-to-const、write-through-const-pointer、discards-const(const 三例合计 1.5 单位);distinct-pointer-compare(0.3 单位);conflicting-function-types(0.5 单位);implicit-function(0.6 单位);local-redefinition(补丁已备,推 0.0.86:四个工具 hostseed-step、hostseed-step-nativefirst、check-chainresume、libneed-native-material 有同块重名待清扫,改名后 hostseed 对象字节变化未定因)。
(2) F2 补位置 25/26;E1 两个词法负例(非法八进制数字、未闭合字符串)已拒绝但诊断缺位置,需 E1 输出可定位错误记号并改 gold 记号表,排入 0.0.86,估 1 到 1.5 单位。
(3) 行为变化:0.0.84 接受 typedef char T[1-2*1];,0.0.85 按 C99 数组界须大于零拒绝。
(4) __builtin_expect 限制:用户自定义保留标识符 __builtin_expect 时静默选用内建(复核样例 GCC 输出 102,本产品 1);静态初始化器中使用报 initializer element is not a compile-time constant(GCC 接受,0.0.86 评估);第二实参须为整数常量表达式,非常量提示有意拒绝;第三实参浮点算术常量表达式如 0.25+0.25 具名拒绝 not covered: floating constant expression in __builtin_expect_with_probability(-c 与 -o 均非零、带位置、无产物)。
(5) __builtin_unreachable 与 __builtin_trap 未纳入,仍报 undefined function。
(6) inttypes.h 既有缺陷的诊断位置前缀问题沿用,本版未修。
(7) 产品资格域非整域:严格证据所列批次之外为 UNMEASURED,不得称全批资格;cx-booter-ape 与 tools-default-regression UNKNOWN(no-producer)。
(8) prerun-drive 五处缺陷(另派修):prebuild-self 调不存在的参数且在 root 跑;每次 attempt 删 dvqualify checkpoint 且外层 58 秒早于窗口 flush;g1-dry 复制名不符 release-exec 第 323 行要求且 g1 布局 unisacc.com 指 root 而非候选;merge 只留一条尾 SUMMARY 与 release-exec 第 454 行冲突(本次改按 0.0.84 逐窗拼接);dvqualify 不留批 stdout,equiv0/1 首轮 FAIL 无法事后定因。
(9) 全套 s9 本版未跑;同候选原生 CI run 44680404617 六目标一次通过(只覆盖 CI 冒烟子集),整体裁决 NOT_QUALIFIED;外目标执行格仍 UNVERIFIED。
(10) 公开 GitHub release v0.0.85 正文为空(gh release view --json body 长度 0),本说明未随发布上传;已公开正文未改,更正另报主调度。仓内 release/decision.txt 仍为 0.0.74 的旧裁决,未随本版重算,本版整体裁决以本说明与 release/evidence/0.0.85/INDEX.tsv 为准。
