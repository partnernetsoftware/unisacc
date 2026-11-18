# unisacc 0.0.74 发布说明

状态:同候选原生 CI run 44274804173(输入 commit=X2 67aea86f03,候选表与 X 相同),八个作业全部 success,六目标 success,CANDIDATE_QUALIFY verdict=QUALIFIED fail=0 invalid=0。前一 run 44272330862 的 osx/x86_64 stream-pipe 两次 55 秒超时(55065、55125 ms),stream-pipe 按场景分两段后重派。已公开并置 Latest(2026-11-18T09:29:07Z);公开读回 c47-1-readback PASS,全新目录下载 unisacc.com sha256 63334e18。首次公开(17:27:12)后读回外层 57 秒超时触发自动回滚(转草稿、Latest 回 v0.0.73),第二次因复用输出目录被拒再次回滚,第三次用全新输出目录通过;标签与 release 均未删除。冻结到公开实测 1 小时 29 分 43 秒(15:59:24 -> 17:29:07),目标 12 到 15 分钟未达。整体裁决 NOT_QUALIFIED:全套 s9 本版未跑,c-negative 仍有 9 项已知 FAIL,affected 421 批只实跑了严格证据所列批次,不得称全批资格。

本版第一次采用"候选先于冻结":整链在分支末端提交 T 上起跑并出严格证据,之后 main 快进到 T 作冻结,冻结后不再跑整链。冻结到公开实测时长见时间账(0.0.73 为 42 分 13 秒,其中整链占 24 分 32 秒)。

## 真实身份

| 对象 | 身份 |
|---|---|
| 候选点 T(构造输入、冻结提交、发布标签目标) | 0dae4d30246052a436e7c0c6a9cee94a12ca31bd |
| 来源 SID(身份帮手实核 VERIFIED) | 511242bfd4c0ec5f90a3a5943884febf0f8af85f38ba4587e43e75a9fcb3383a |
| 发布 APE SHA256(unisacc.com) | 63334e185bb906d5352ff9f8b2c5dfa2c26b8fef1a35f965994ba8c7907cca39 |
| 发布 APE 字节数 | 7241055 |
| 整链产物 APE SHA256 | 63334e185bb906d5352ff9f8b2c5dfa2c26b8fef1a35f965994ba8c7907cca39(与发布件逐字节相同,自报 unisacc 0.0.74) |
| pkg.bin(P4 包) | 9cc1d5fe6775bdf590b794165d066d6d4e1d7e0022625c956d40abf8c6114b6e |
| slice-win-arm64(注入后) | 3f66f5ef5fd8361e562ed683d3d7e2d8ed928ceca909c0cbff62d5d7b6f7c47e |
| 构造记录 SHA256 | c5e2459dc7b27885a3341ccd24fa8309885238f526f1e529175739c0740d9d4f |
| SHA256SUMS SHA256 | a94058b62dda9ee2253407af3ed98b9b9939dc91ca1708ba5bdd9c1f6d4ff6c5 |
| 候选表提交 X | 159b877efec6f1ea34b1fd017d39d1ee0bc6e6bb |
| CI run | 44272330862(candidate-native.yml,ref v0.0.74,输入 commit=X) |

## 产品改动

(1) 结构体成员上限 64 -> 512。513 个成员或展开后超过 512 时三入口具名拒绝(`error: not covered: structure member capacity`,带文件行列)。
(2) 结构体编号扩位:成员名键步长 MEMBER_STRIDE 256 -> 4096。修复同一单元内超过 255 个结构体共用成员名时的静默错编(结构体 256+i 与结构体 i 的下一成员名键重叠)。这是已有缺陷,0.0.73 及更早的已装产品与发布件都会静默错编;本版 257、300、2040 个结构体样例默认运行输出与 gcc 一致,2100 个结构体具名拒绝 "structure id capacity"。
(3) 形参重名误拒修复:无名形参之后再出现有名形参的函数声明(如 `int pb1(int, int a, int, int b);`)此前被误报 `redefinition of parameter`。这是 0.0.73 S2 引入的回归,0.0.73 发布件存在该回归。无名形参后接真正重名的形参仍拒绝。
(4) 新批次 struct-member-capacity(42 项)与 check-struct-member-capacity(5 项)。

三入口(默认运行、-c -funit、-S)对本版新行为样例,链产物 63334e18 实测:

| 样例 | rc(默认/-c -funit/-S) | 首行 |
|---|---|---|
| m65(65 成员) | 0/0/0 | stdout `sizeof 392`,与 gcc 一致 |
| m512(512 成员) | 0/0/0 | stdout `sizeof 3072`,与 gcc 一致 |
| m513(513 成员) | 1/1/1 | `m513.c:516:12: error: not covered: structure member capacity`(三入口同) |
| structs257 | 0/0/0 | stdout `sizeof 16 16`,与 gcc 一致 |
| structs300 | 0/0/0 | stdout `sizeof 16 24`,与 gcc 一致 |
| 无名形参混合 `int pb1(int, int a, int, int b);` 等 | 0/0/0 | 无诊断 |
| 无名后真重名 `int pc(int, int a, int a);` | 1/1/1 | `reject: redefinition of parameter (C99 6.7p3)`(三入口同) |

## 严格证据

链产物 63334e18 经 devsvc qualify 在 main 布局 b8932325e2(独立本地布局根,逐批 affected -> run-batch 实跑,外层 60 秒)跑 30 批:29 PASS,c-negative FAIL 53/9(预期,9 项已知,见 release/evidence/0.0.74/expected-fail.tsv)。PASS 含 struct-member-capacity 七批 42 项(accept-a 15、accept-b 9、accept-c 6、reject 9、s2040-run、s2040-c、s2040-asm 各 1)、check-struct-member-capacity、pp-negative 10/0、pp-differential、pp-cli-targets、pp-cli-macros、pp-cli-includes、pp-header-locations、pp-targets-lines、c99-whitespace、unit-frontcache-budget、run-warm、cx-driver-cli、tools-compile 10 片(000 005 009 055 056 062 086 108 126 140)。每批 candidate= 等于 APE sha256;UNKNOWN 0、INVALID 0。

前序轮次如实记录:首轮布局取冻结前 main,缺分支批次,作废;原单批 struct-member-capacity 42 项两次超过自身 55 秒上限(已跑 31 项全 PASS),拆两批后 reject 批在冷产品缓存下仍超时,最终按冷缓存实测拆成七批(每批 35 秒内),断言总数不变,时限不放宽。affected 对本版改动选出 421 批,其余未跑,UNMEASURED。

## 开发工具链改动(概要,不进入产品)

devsvc qualify(候选 APE 一条命令出严格证据,独立本地布局根逐批实跑);release-run --execute(候选就绪到公开读回一条带门命令,本版首次真实使用)与 --plan;release-exec G1 本版改为可读 qualify 证据格式(RAW BATCH_RUN、count=P/F、SUMMARY 身份);dvtiming、dvgitx、dvfile;react 与候选键;devsvc 拒绝文案;缺口表与追踪表。均为开发与验证侧改动。

## 时间账(候选先于冻结)

| 事件 | 时刻(+0800) |
|---|---|
| T 提交 | 2026-11-18 15:23:48 |
| 整链起 / DONE | 15:24:17 / 15:50:16(25 分 59 秒,冻结前完成) |
| 冻结(main 快进到 T 并推送) | 15:59:24 |
| 打包(--pack --commit T) | 16:00:56 |
| 严格证据(拆批后最终轮)完成 | 17:01 |
| X 提交 159b877efe | 17:02:46 |
| CI 首派 44272330862(osx/x86_64 stream-pipe 超时) | 17:03:29 至 17:10 |
| stream-pipe 分段提交 67aea86f03,CI 重派 44274804173 | 17:19:16 至约 17:26 |
| Y1 3115389996 | 17:27:08 |
| 公开(最终) | 17:29:07 |
| 公开读回 PASS | 17:29:38 |
| Y2 642c44488a | 17:29:42 |

冻结到公开 1 小时 29 分 43 秒。超出主要来自三处:严格证据阶段 struct-member-capacity 冷缓存超时并两次拆批(约 60 分钟)、CI osx/x86_64 stream-pipe 超时并拆段重派(约 23 分钟)、release-exec 首次真实使用暴露的三个夹具差异(G1 读不了 qualify 格式、远端缺 v0.0.74 分支、Y1 把目录交给 --whole)与读回外层超时。整链本身没有在冻结后重跑。

## 已知缺口与未验证范围

(1) S2-INLINE-REJECT-NO-LOCATION:S2 诊断只有 `reject:` 文案,无文件与行号。
(2) sqlite3.c 仍未编过(38598 行 function pointer cast result type 等),长尾 UNMEASURED。
(3) U20 core-compat、U22 材料与平台、U23 stages 字节差异、U25 interfaces-lnx-x86_64 oracle 迁移、U26 剩余(c-negative 9 项 FAIL)延续未关闭。
(4) s9 全套本版未跑;affected 对本版改动选出 421 批,本版只实跑严格证据所列批次,其余 UNMEASURED。
(5) 严格证据首轮布局取了冻结前的 main(缺分支批次),第二轮 struct-member-capacity 在 55 秒批次上限内未跑完(load1 5.7,transport=-1),均如实保留,以低载重跑轮为准。

## 证据入口

release/evidence/0.0.74/INDEX.tsv;release/evidence/v0.0.74-strict-2026-11-18.tsv;release/evidence/0.0.74/expected-fail.tsv;release/evidence/v0.0.74-struct-id-and-pdup-2026-11-18.tsv。撤回方案:release/rollback.txt(回退 Latest 到 v0.0.73,不删标签与 release)。
