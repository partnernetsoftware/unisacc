# unisacc 0.0.73 发布说明

状态:同候选原生 CI run 44248703181 attempt 1(输入 commit=X e289dd473d),六目标全部作业 success(lnx x86_64、lnx arm64、osx x86_64、osx arm64、win x86_64、win arm64),CANDIDATE_QUALIFY verdict=QUALIFIED pass=42 fail=0 not_run=0 invalid=0。尚未公开(Y1 记录,公开与读回在 Y2 补记)。 整体裁决 NOT_QUALIFIED:全套 s9 本版未全跑,c-negative 仍有 9 项 FAIL,产品域仍有 FAIL 与 UNKNOWN,不得称全批资格。

## 真实身份

| 对象 | 身份 |
|---|---|
| 冻结提交 F(构造输入、发布标签目标) | 3865c7c9e0e5b7b75a18e9839a459f12553d0b55 |
| 来源 SID(身份帮手实核 VERIFIED) | 212afcfde65dc8a1277c349557e0d1863b67dcdb779afa091f2fde3248edb683 |
| 发布 APE SHA256(unisacc.com) | 82d0926e9bed988091dac4ed43e07ab3c199652b0dc470e1cb5e445564e3f3a3 |
| 发布 APE 字节数 | 7241019 |
| 整链产物 APE SHA256 | 82d0926e9bed988091dac4ed43e07ab3c199652b0dc470e1cb5e445564e3f3a3(与发布件逐字节相同) |
| pkg.bin(P4 包) | 02cf8516e7f411db7d374d8c61687a46df33d59153a29d3a9da89ff351651dd5 |
| slice-win-arm64(注入后) | 262d103ebe0db1603e793143a31c2d79e0e5e4f9cd80017aaf214a619c54c28c |
| 构造记录 SHA256 | c43906db48a8377b42b5dd5ab3ec27b24523f984883d427b239c885a0346c44c |
| SHA256SUMS SHA256 | b746097aa45448f00abaada8f59cd0dc036fe5de4656dd245f820f07ccf5132d |
| 候选表提交 X | e289dd473d72bcf229fefd13ab68652a9ea55f0a |
| CI run | 44248703181(candidate-native.yml,输入 commit=X) |

版本槽本版生效:冻结提交里的候选表已是 0.0.73 行,整链产物实跑 --version 打印 unisacc 0.0.73;release-run --pack 打出的 APE 与链产物同 sha,0.0.72 那种"链产物自报旧版本、发布件另行注入"的差异本版不再出现。六目标切片与 0.0.72 同 sha,本版产品差异全部在 P4 包(前端表)内。

## 产品改动:8 项具名拒绝(行为收紧)

这是行为收紧:下列代码此前被静默接受(编译通过或产生未定义结果),本版起编译失败,退出码 1,三个入口(默认运行、-c -funit、-S)首行诊断一致。依赖这些写法的代码需要修改。

| 类 | 最小输入 | 诊断首行 |
|---|---|---|
| 预处理 macro-name-not-identifier | `#define 3x 1` | `FILE:1:1: error: macro-name-not-identifier: macro name must be an identifier` |
| 预处理 header-name-unclosed | `#include <stdio.h`(缺 `>`) | `FILE:1:1: error: header-name-unclosed: missing terminating > or " in #include` |
| void-parameter-named | `void b15(void x){ }` | `reject: parameter of type void must be unnamed and alone (C99 6.7.5.3p10)` |
| multiple-storage-classes | `static extern int b16;` | `reject: multiple storage classes in declaration specifiers (C99 6.7.1p2)` |
| duplicate-parameter | `void vv(int a, int a){}` | `reject: redefinition of parameter (C99 6.7p3)` |
| zero-array-size | `int t2[0];` | `reject: array size must be greater than zero (C99 6.7.5.2p1)` |
| void-value-used | `int v2(void){ int x; x = (void)0; return x; }` | `reject: void value not ignored as it ought to be (C99 6.5.16.1p1)` |
| int-plus-assign-string | `int x2(void){ int i = 0; i += "str"; return i; }` | `reject: invalid operands to compound assignment: arithmetic left operand with pointer right operand (C99 6.5.16.2p1)` |

U26(产品对应拒绝输入静默接受)本版转 PASS 的共 8 项:c-negative 6 项(44/15 -> 50/9)加 pp-negative 2 项(8/2 -> 10/0)。c-negative 其余 9 项仍 FAIL,顺延 0.0.74。

## 严格证据(发布件即链产物 82d0926e)

独立本地布局根逐批实跑 22 批:21 PASS,c-negative FAIL 50/9(预期,9 项未修)。PASS:pp-negative 10/0、pp-differential、pp-cli-targets、pp-cli-macros、pp-cli-includes、pp-header-locations、pp-targets-lines、c99-whitespace、unit-frontcache-budget、run-warm、cx-driver-cli、tools-compile 随机 10 片(006 017 037 038 042 046 049 053 055 132)。每批 candidate= 等于 APE sha256;UNKNOWN 0。三入口 8 项诊断首行一致,正控 rc 0。tools-compile 首轮在无 .git 的归档根里 rc 2 "git inventory failed"(夹具,非产品),改私有 git 索引后重跑 PASS,如实记于证据文件。

## 开发工具链改动(概要,不进入产品)

版本槽门(候选表目标版本行与分支名一致,整链产物自报目标版本);run-batch {candidate-root} 占位符,c-negative 与 pp-negative 经 --candidate 跑到候选产品;affected 选批精度改进;整链在负载高时让出(rc77)与显式 cache-manager;react 触发键;devsvc 增 git merge、untrack、worktree、f append 等。上述均为开发与验证侧改动。

## 已知缺口与未验证范围

(1) U20 core-compat、U22 材料与平台、U23 stages 字节差异、U25 interfaces-lnx-x86_64 oracle 迁移,延续未关闭。
(2) U26 剩余项:c-negative 9 项 FAIL,顺延 0.0.74。
(3) DIAG-LINE-OFFSET:诊断行号偏移问题未修。
(4) FRONTEND-STRUCT-MEMBER-CAP-64:前端结构体成员上限 64 未放开。
(5) S2 已知限制:duplicate-parameter 在无名形参槽位可能残留旧名;S2 六项诊断无文件与行号(S2-INLINE-REJECT-NO-LOCATION),只有 `reject:` 文案。
(6) s9 全套本版未全跑,第 4 步 UNKNOWN;发布件严格证据只覆盖上列 22 批与 8 项三入口样例。
(7) osx cache-manager 两份:chain2 现场在打包前被清理,用已装 0.0.71 的 tools/cache-retain.cx 重建,sha256 与整链所用 d34bee6c(x64)、0aae3fbc(arm64)逐字节相同;checkpoint 行仍不带 cache-manager 身份(已登记缺口)。

## 证据入口

release/evidence/0.0.73/INDEX.tsv(逐项带时间戳);release/evidence/v0.0.73-strict-2026-11-18.tsv;release/evidence/v0.0.73-s1-cycle-2026-11-18.tsv;release/evidence/v0.0.73-s2-cycle-2026-11-18.tsv;release/evidence/v0.0.73-s0-census-2026-11-18.tsv。撤回方案:release/rollback.txt(回退 Latest 到 v0.0.72,不删标签与 release)。
