# unisacc 0.0.82 发布说明

状态:同候选原生 CI run 待填(阶段 B 回填)。整体裁决 NOT_QUALIFIED:全套 s9 本版未跑,产品资格域未整域实跑,严格证据只覆盖下列批次,不得称全批资格。

## 真实身份

| 对象 | 身份 |
|---|---|
| 候选点 T(整链输入,第二次整链) | 75cbdfe1c924717c879386eb001aeb47750a3cfb(T2,21:14 到 21:39 约 25 分钟,rc 0) |
| 第一次整链输入 | d20a8f64af(20:04 到 20:29 约 26 分钟一次通过;APE 与第二次整链字节相同) |
| 冻结提交(发布标签目标) | 75cbdfe1c924717c879386eb001aeb47750a3cfb(F 即 T2,相对整链输入产品零改动) |
| 发布 APE SHA256(unisacc.com) | 371a4c741f8b0229092e21b2eb1afd8ba5cb5f23dabcddd2a8dcb9f77103e977(7391587 字节,比 0.0.81 的 7390117 多 1470 字节;--version 打印 unisacc 0.0.82;打包后与链产物同字节,PACK_APE SAME) |
| 源快照 SID | 3638ec2244d5cd7507be177b663eb1c69a78a00da17ea53536866b72268fdcf8(VERIFIED,T2 的链快照 75cbdfe1c9-1705aa26,identity-helper-real5) |
| pkg.bin | 325f8db98742dce656a380482be7e3c01b3443908994f67b7f204160d7846a21 |
| slice-win-arm64 | c53be0b723cd4c919ac5539f8bd0877da54f787a9644cba691e23e16838926ab |
| SHA256SUMS | c98b542bc489fc4521b13011a067b562fa0899fd2cd38e9839b0c6ee03ba3964 |
| CI run | 待填 |

## 产品改动

产品输入相对 0.0.81 冻结提交 8b46bf21ee 只有 exec/ 下 17 个文件(enc 2、facts 8、lower 2、parse2 4、pipeline 1);src、cx、include、ext、seed、weights、kernel 零改动。整程序 -o 路:lnx/x86_64 与 lnx/arm64 各 17 份 -o 产物与 0.0.81 逐字节相同;对象产出 lnx/arm64 的 -c 与 -c -funit 17/17 变字节(0.0.81 起 -c 与 -c -funit 同为单元对象),x86_64、osx、win 对象不变。

(1) c-negative 第一步与第二步(只告警模式,默认路径零改动):-Wall 下同块局部对象重复声明发 warning(规则 ID -Wc99-local-redefinition,文案 redefinition of local object 'x',含 static 前后顺序),找不到声明的调用发 warning(-Wc99-implicit-function,文案 call to undeclared function 'name')。规则挂在 e3 的 unusedwarnings 模型里复用作用域撤销栈,文案表 exec/facts/c99warnings-text.tsv;rc 0 对象照写,无 -Wall 的默认路径对象与 stderr 与 0.0.81 逐字节相同(27 夹具加 17 份 apps);17 份 apps 在 -Wall 下命中 0,realprog 四项命中 0,60 个 tools 抽样仅 2 处真违规(同一文件先用后定义)。c-negative 批次仍 53/9(规则只告警,不转拒)。先红后绿:check-c99-warn 对 0.0.81 为 16/2 与 48/6,新产品 54/0。P4 包身份变化。
(2) arm64 -funit 单元栈修复(缺口 OBJABI-ARM64-X7-STACK-BELOW-SP-SIGNAL):单元专用 uentry(预留 1MB 块在 sp 之上,x7=x29=块顶,信号帧落块下不覆盖)、uleave、ustk(内部调用前 add x16,sp,#4096; cmp x16,x7; b.lo +8; brk #0xf7 越界固定拒绝 SIGTRAP),整程序 hentry 引用点不动;常量表 exec/facts/unit-x7-stack.tsv(RES_4K=256、MARGIN_4K=1、TRAP_IMM=247)。qemu 实跑:0.0.81 同例带处理函数的 SIGALRM 每毫秒 -> SIGSEGV rc139,新版 rc0;三定时器同开每 100us、处理函数 8KB 栈 bad=0;5000 层小帧递归与 20 层 8KB 帧递归通过(0.0.81 可通过的容量不回归)。check-objabi 新增 a-sigalrm、a-sigovf、a-calleesaved、a-icall、a-deep、a-x16free、a-branchsense、a-bigframe(已知限制)。实现中发现并修了第一版的严重缺陷(检查分支方向写反导致健康调用也 brk、只覆盖全局名调用),经独立复核。

## 开发工具链(不进产品)

(1) S2 令牌调度第一期:dvshadow 资源采样(affinity 与 cpuset 交集、cgroup v2 嵌套配额取最紧、MemAvailable,读不到记 UNMEASURED)、实际占用块(相邻 CPU 时间差、进程树去重、RSS)、dvqualify 观测点影子(开关 DEVSVC_S2_SHADOW 默认关闭,关闭路径经预处理器整段删除,RAW 与 rc 与起跑数逐项等于原版;开启时非阻塞 datagram,接收侧同 uid 校验,杀接收者与队列满真实发车结果不变)。依赖 sys.ffi 与 sys.netraw 两个模块,不受开关控制;打点版构建失败回退普通版。第二期(授租与抢占)0.0.84。
(2) derive_gate(预提交门):触及派生表输入而派生物未重算则拒绝提交并具名缺表与修法,derive 起不来时 DERIVE_GATE_UNAVAILABLE 放行(失败安全);derive 生成器统一只读受跟踪文件(genkit gk_tracked_load 接口,根治 table-index 与 doc-links 与 derive_gate 三次假漂移);derive --check 对本地 .devsvc/ 镜像不再判漂移(MIRROR_STALE);register 登记后打印 REGISTER_NEXT 提示。
(3) 慢批拆分:signal-targets 拆 3 片、win-argv 拆 3 片、snapshots-cx 拆 2 片(每片全冷耗时不超过 22.1 秒),多重集相等检查 slow-split;modelcache 两批整组独占不拆。拆分父批旁表与 G1 新门(r4)留 0.0.83。
(4) speed-ledger:速度账 18 列(含计划与完成与未计划工作项数、起链后提交数、吞吐、链后到公开分钟),四个版本回填(0.0.81 11/6/8 吞吐 1.21);chain-ready 新增前置 12 到 14(工作树文件类型、整链快照缓存、Root 记录一致性,根治 0.0.81 起链前四次失败)。
(5) R0 发布工具两条缺口修复:release-exec 步骤 1 识别已提交候选表(记 exec-1-X-commit 行,冲突与未推送具名拒绝)、release-prep 默认按版本号找严格文件;tests/release-exec-assertions.tsv 补 bc 段 3 条与 na 段 32 条(修复 main 上 release-run-exec-gates 此前一直 ASSERTION_INVENTORY_MISMATCH 的真红)。
(6) ref 类 22 批判据替换(cdx):经典参考编译器(冻结提交 500d6cf3d8 复建 ua_ref,独立复建与收据逐字节一致)的 "镜像字节等于参考" 旧判据过时(X3 起产品指令选择有意变化),替换为参考镜像手推字面 stdout/rc、结构与确定性、宿主 cc 差分、外目标 UNVERIFIED;corpus 十批 30 片 960/0、closure 131/0、layout 七片 43 子片 7489/0、properties 26/0 与 53/0 与 53/0;netregen 优化档 30 格(0.0.80 对 0.0.81 候选 30/30)。ref 备料持久缓存(~/.cache/unisacc/ref-material,键含冻结提交与导出器与参考 ELF 与工具链与配方身份,命中逐次核字节)。
(7) old-product 材料生产者(固定公开资产下载与完整性缓存,29 项离线检查)与 30 个 netregen 批 30/30;realprog 四项接回;0.0.80 以前文件归档 82 个到 archive/pre080;U 类 tsv 清理;POSIX 与 asm 能力实测矩阵(POSIX 17/29,asm 4/17)与 attribute 使用普查入路线图。

## 严格证据

(1) 预跑(T 布局,候选 APE 371a4c74,dvqualify):380 批,约 40 分钟;补跑 34 批;T2 布局补 1 批(c99-warn 补登 batch-product-inputs 行后)。
(2) 拼合:整合文件 release/evidence/v0.0.82-strict-2026-11-19.tsv(1044 行,sha256 9743bc87ea16...)为各段原样拼接,不删不改任何行,共 407 处 BATCH_RUN,全部 candidate=371a4c741f8b...。同批多段有行时以后段为准。
(3) 最终 389 批:PASS 378、FAIL 1、UNKNOWN 10、INVALID 0。
  - FAIL c-negative 53/9:与 0.0.76 起各版同批同数,产品接受 9 个应拒的 C99 负例,非本候选回归;登记 expected-fail.tsv,到期 0.0.83。
  - UNKNOWN 10 个,均为材料无生产者(no-producer),记 UNMEASURED 不记 PASS,expected-unmeasured.tsv 引用本次真实 DEFERRED 行:cx-booter-ape、tools-default-regression、opt-frame4-o1-negative-end、opt-frame4-o1-negative-eof、opt-frame4-o1-negative-label、opt-frame4-o1-positive、opt-frame4-o2-negative-end、opt-frame4-o2-negative-eof、opt-frame4-o2-negative-label、opt-frame4-o2-positive。
  - SUPERSEDED:win-argv 两条(预跑 rc 2 的派生表来源戳过期,重算后同候选 PASS)与 unit-frontcache-equiv4 一条,由后段 PASS 取代;另 G1 将 notcovered 的前段 UNKNOWN 行记为 SUPERSEDED(后段 PASS),G1 合计 superseded=4。
  - ref 类 22 批拆 78 片,新判据 78/78 PASS;0.0.81 的 20 行旧判据 expected-fail 全部核销。
(4) 预热与干跑:N4 读回预热 WARM READY(5 个目标);release-exec --dry DRY_GATES_OK steps=7 deferred=6,G1 STRICT PASS rows=407 expected_fail=1 superseded=4,CANDIDATE_SHA、APE_VERSION unisacc 0.0.82、RATCHETS 15/0、MAIN_ANCESTOR、ASSETS 六项均 PASS。

## 整链

第一次整链(输入 d20a8f64af)20:04 到 20:29 一次通过,约 26 分钟。预跑发现两处问题:win-argv-gen 管的三张派生表(enc-x86win-winargs、enc-armwin-winargs、lower-data)来源戳注释过期(enc-literals.tsv 加行后未重算,无内容改动),c99-warn 缺 tests/batch-product-inputs.tsv 行致 affected 拒绝选批;重算并补行后第二次整链(输入 75cbdfe1c9)21:14 到 21:39 约 25 分钟,APE 与第一次字节相同。链目录 /tmp/unisacc-session-v082-chain2/75cbdfe1c924717c879386eb001aeb47750a3cfb。

速度账(tools/speed-ledger.cx --version 0.0.82 实跑):槽到起链 255.5 分钟;整链出口 279.8 分钟(整链 2/2 次);预跑完成 UNMEASURED(账本未取到,实际预跑约 40 分钟加补跑);公开 待填。

## 已知缺口

(1) arm64 -funit 单元栈:单帧大于预留(1MB)可越过防护(OBJABI-ARM64-X7-LARGE-FRAME-SKIPS-GUARD);hostcall 回调重入每层叠加 1MB 预留(OBJABI-ARM64-X7-REENTRANT-RES-STACK-USE);越界拒绝无文案仅 SIGTRAP rc133;仅 qemu 实跑,原生 arm64 与真实内核 UNVERIFIED。
(2) c-negative 告警模式:static 与 -Werror 未升级为拒绝(WERROR-NOT-ESCALATING);__builtin_expect 误报(C99WARN-IMPLICIT-BUILTIN-FALSE-POSITIVE);参数与函数体同名无诊断;conflicting-function-types 与 const 三例与乙组未做(0.0.83 起)。c-negative 批次仍 53/9 expected-fail。
(3) 实测发现的产品缺陷待修:sigprocmask(SIG_BLOCK) 未屏蔽、environ 为 NULL、wchar.h 与 stdlib.h typedef 冲突、基本 asm 与未实现属性静默忽略(aligned、packed、cleanup 静默错码)(0.0.83 小修包);线程同步 0/3、原子与 TLS 全拒(0.0.87 到 0.0.88)。
(4) 拆分父批旁表(r4)、S2 第二期、R1 release-flow 与 R2 增量预跑未做(0.0.83 到 0.0.84)。
(5) F4、F5、equiv0 与 equiv3 定因(0.0.81 顺延)本版仍未做。
(6) 全套 s9 本版未跑;产品资格域本版仍非整域,严格证据所列批次之外为 UNMEASURED,不得称全批资格;外目标执行格(osx、win、arm64 原生)ref 类仍 UNVERIFIED。
(7) check-ctestsuite 间歇失败一次未定因;pkg-gaps 18 行格式不齐。
(8) PRODUCT-TABLE-GENERATORS-NOT-IN-DERIVE-CHECK:产品表生成器(win-argv-gen 等)不在 derive.tsv 的统一 --check 里,改 enc-literals 一类上游表时无门强制重算。
(9) REGISTER-NO-BATCH-PRODUCT-INPUTS-ROW:register 登记批次时不强制写 tests/batch-product-inputs.tsv 行。
(10) F4 检查与 F5 链接合同检查、unit-frontcache-equiv0 与 equiv3 的 deps=DIFF 定因,本版仍未做,顺延 0.0.83。
(11) 拆分父批旁表与 G1 新门(r4)、gaps-archive、R1 release-flow 与 R2 增量预跑顺延 0.0.83。

## 未解决项(沿用)

(1) NN-VERIFY-WEAK。
(2) -S -funit 返回 NOT_IMPLEMENTED。
(3) 其余开放项见 build/pkg-gaps.tsv。

## 证据入口

release/evidence/0.0.82/INDEX.tsv;release/evidence/v0.0.82-strict-2026-11-19.tsv;release/evidence/0.0.82/expected-fail.tsv;release/evidence/0.0.82/expected-unmeasured.tsv;release/evidence/0.0.82/*-deferred.tsv;release/evidence/0.0.82/pack-run.out;release/evidence/0.0.82/cache-manager.tsv;release/evidence/0.0.82/warm-*.tsv。撤回方案:release/rollback.txt(回退 Latest 到 v0.0.81,不删标签与 release)。
