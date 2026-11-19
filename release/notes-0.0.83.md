# unisacc 0.0.83 发布说明

状态:同候选原生 CI run 待填。整体裁决 NOT_QUALIFIED:全套 s9 本版未跑,产品资格域未整域实跑,外目标执行格 UNVERIFIED,严格证据只覆盖下列批次,不得称全批资格。

## 真实身份

| 对象 | 身份 |
|---|---|
| 候选点 T(整链输入,第二次整链) | f24ba9737d54acad8088d3edb0d83b71ade4e8bd(01:39 到 02:06 约 27 分钟,rc 0) |
| 第一次整链输入 | 8d79aea9b9(23:44 到 00:10 约 26 分钟;APE 与第二次整链字节相同) |
| 冻结提交(发布标签目标) | 42d1f989d339a8bb7b839373d35fba60e861a277(F,含 main 工具的最终布局 T3,相对整链输入产品输入零差异) |
| 发布 APE SHA256(unisacc.com) | 12fd0c572996d57f7b3c6df2f1390f55ec6fbff5274896f7abd2cd0dcbf0258b(7399573 字节,比 0.0.82 的 7391587 多 7986 字节;--version 打印 unisacc 0.0.83;打包后与链产物同字节,PACK_APE SAME) |
| 源快照 SID | 38518355b324ce2d2e9a0131abf4c76edbcfdc9ba50f8411e63d81b70366908b(VERIFIED,T 的链快照 f24ba9737d-b01aff12,identity-helper-real5) |
| pkg.bin | 9489eaab801e7a2d0d3fc9f709c48a2b0d106ec0b4f24c90afeafc41f8b87e23 |
| slice-win-arm64 | 76354c15b2596bea93095fe8be0782dcc7cf1f47275ab53f28dd2250e6b71a4e |
| SHA256SUMS | 8883133ce9bced1255311def8eeb28cf651acb01fea6a4ffc295a2009af84afe |
| CI run | 待填 |

## 产品改动

产品输入相对 0.0.82 冻结提交 75cbdfe1c9 只有两类:exec/lex 表(基本 asm 与未实现 __attribute__ 具名拒绝)与 include/ 头(POSIX 小修)及其派生表(cx-flat.h、pp-autoinc-gen.tsv、build/libc-decls.tsv、k2-gen2.tsv 与 k2-libraryenv.tsv 的来源戳);src、cx、ext、seed、weights、kernel 零改动。

(1) 基本 asm 与 volatile asm 及未实现 __attribute__ 具名拒绝(原为静默忽略,有副作用的基本 asm 会被静默丢弃而产生错误代码):__asm__(...)、asm(...)、asm 标签、__asm__ volatile 与 __volatile__ 各形态改为 reject: not covered: inline asm;__attribute__ 白名单外(aligned、packed、cleanup、section、weak、alias、naked、vector_size、visibility hidden、未知属性)改为 reject: not covered: attribute。白名单 24 词(constructor、destructor、visibility 仅 default、pure、const、malloc、nothrow、leaf、sentinel、alloc_size、returns_twice 与无害属性)走事实表 exec/facts/lex-attr-whitelist.tsv -> tools/lex-attr-gen.cx(--check)-> exec/lex/attr-byte.tsv(175 节点)。66 个标准头逐个包含零误拒,60 个 tools 抽样 -c 对照 0.0.82 rc 与 stderr 一致,28 份示例 -o 逐字节相同。先红后绿:check-asm-attr-reject 对 0.0.82 红 7/17,候选 24/0。
(2) POSIX 小修(include/ 层):sigprocmask 真实屏蔽(SIG_BLOCK 屏蔽的信号 raise 时挂起,SIG_UNBLOCK 与 SIG_SETMASK 解屏蔽后按编号升序投递;屏蔽字与待决字为跨编译单元的全局暂定定义;sigaction 的 sa_mask 与 SA_NODEFER 与 SA_RESETHAND 参与;SIGKILL 不可屏蔽);extern char **environ 在不包含 unistd.h 时不再为 NULL;wchar_t 在 stddef.h 补守卫使 wchar.h 与 stdlib.h 同时包含不再 typedef 冲突。与宿主 gcc 对拍:屏蔽与投递、sa_mask、NODEFER 逐字节一致。限制:kill(getpid(), sig) 走内核系统调用不经用户态屏蔽(具名限制 KNOWN_LIMIT)。check-posix-fix 对 0.0.82 红 2/7 与 10/2,候选 12/0。
(3) netregen 字节栅栏的结构问题与处理:头改动使 .cx 模块镜像可写段 memsz 0x22a20 -> 0x227b0(代码长度不变,地址立即数移位),12 格(netregen-module 与 netregen-opt-module 各 6 目标)对旧产品参照逐字节不同(0.0.80 与 0.0.82 公开件编出镜像相同,其余 48 格对 0.0.82 全相同)。判据文本不改,新增精确指纹预期差异登记(旧新镜像 sha256 逐字相等才放行,带到期 expires=0.0.84)。下版以本版公开件为参照应回到相同。

## 开发工具链(不进产品)

(1) R1 release-flow(阶段 A 一次产出与离线重放 --check):对 0.0.81 阶段 A 13/13、0.0.82 19/19 文件逐字节相同;0.0.82 点名 signal-targets 与 snapshots-cx 两个拆分父批缺席并 rc2 不发布;独立复核后加固(证据段顺序校验、分相 --bg、缺口不发布、临时目录进会话目录)。本版阶段 A 打包、预热与干跑沿用 release-run --pack 与 release-exec 分相执行。
(2) R2 第一半(增量预跑身份与保守覆盖建议,开关默认关,只给建议不免跑):安全硬门对 0.0.81 的 28 个与 0.0.82 的 35 个实际补跑批全部 MUST_RERUN,漏选 0;效率 0/1529 与 0/1640 COVERED(原因:缺原批证据、RAW 缺 runner 身份、未决终态),数据缺口(RAW 身份摘要字段)排 R2 第二半。
(3) R3 复核模板与真实提交预演夹具(七类检查单、五段简报、check-real-commit)。
(4) 拆分父批旁表与 G1 r4(tests/split-parents.tsv since=0.0.83,父批缺席时分片须 witnessed 且 PASS,防删表绕过,check-slow-split 单一来源)与 G1 netregen 指纹规则(含旧段无指纹行继承的精化,独立复核加固 3 处绕过)。
(5) 脚本场景基准(cx -run 对 node、python3、bun,8 场景 4 引擎各 7 次):热态多数场景领先 node 与 python3(0.15 至 0.8 倍),JSON 与 glob 落后 bun(2.1 与 1.6 倍),冷启动 616ms 对 node 42ms(14.5 倍)是最大短板;冷启动分段:推理 76%(E3 48.8%、lower 14.0%)、模型装载 15%;热态 6.5ms(运行缓存命中)。0.0.84 以此为执行器与装载优化依据。
(6) S2 第二期设计(授租与抢占,0.0.84 三单元)。
(7) 文档整理:ARCHITECTURE.md -> prd_arch.md,spec.md -> prd_spec.md(旧名留符号链接),README 与 prd 增内置能力一节(内置 libc、POSIX 子集、cx、-run)与按角色阅读顺序,plans/INDEX 唯一入口。

## 严格证据

(1) 预跑(T 布局,候选 APE 12fd0c57,dvqualify):00:15 起至 02:23,分段多次(strict-00、01、re、re2、re3、t3、t3b、t3c、t3d)。
(2) 拼合:整合文件 release/evidence/v0.0.83-strict-2026-11-20.tsv(1122 行,sha256 96a33efbcc4e...)为各段原样拼接,不删不改任何行,共 417 处 BATCH_RUN,全部 candidate=12fd0c572996...。同批多段有行时以后段为准。
(3) 最终 406 批:PASS 383、FAIL 13、UNKNOWN 10、INVALID 0。
  - FAIL c-negative 53/9:与 0.0.76 起各版同批同数,产品接受 9 个应拒的 C99 负例,非本候选回归;登记 expected-fail.tsv,续期到 0.0.85。
  - FAIL 12 格 netregen 预期差异:netregen-module 与 netregen-opt-module 各 6 目标(lnx-x86_64、lnx-arm64、osx-x86_64、osx-arm64、win-x86_64、win-arm64)。原因是本版 include 头改动使 .cx 模块镜像可写段 memsz 0x22a20 -> 0x227b0,代码长度不变而地址立即数移位,镜像对旧产品参照逐字节不同。登记方式为精确指纹:expected-fail.tsv 每格写 image=旧镜像 sha256->新镜像 sha256,G1 只在严格证据 FAIL 行的指纹与表逐字相等时放行,改一位即拒;到期 0.0.84,下版以本版公开件为参照应回到相同。这是已登记的预期差异,不是 PASS,也不证明镜像语义等价。
  - UNKNOWN 10 个,均为材料无生产者(no-producer),记 UNMEASURED 不记 PASS,expected-unmeasured.tsv 引用本次真实 DEFERRED 行:cx-booter-ape、tools-default-regression、opt-frame4-o1-negative-end、opt-frame4-o1-negative-eof、opt-frame4-o1-negative-label、opt-frame4-o1-positive、opt-frame4-o2-negative-end、opt-frame4-o2-negative-eof、opt-frame4-o2-negative-label、opt-frame4-o2-positive。
  - SUPERSEDED:notcovered 前段一条(transport=-1)由同候选后段 PASS 取代,G1 合计 superseded=1。
  - 拆分父批 signal-targets(3 片)、win-argv(3 片)、snapshots-cx(2 片)本版 8 分片全部 PASS 且有见证,补上 0.0.82 的覆盖缺口。
(4) 预热与干跑:N4 读回预热 WARM READY(5 个目标);release-exec --dry DRY_GATES_OK steps=7 deferred=6,G1 STRICT PASS rows=417 expected_fail=26 superseded=1(26 = c-negative 1 条加 netregen 12 格的 FAIL 行 25 条),CANDIDATE_SHA、APE_VERSION unisacc 0.0.83、RATCHETS 15/0、MAIN_ANCESTOR、ASSETS 六项、DECISION 均 PASS。

## 整链

第一次整链(输入 8d79aea9b9)23:44 到 00:10 约 26 分钟。之后发现 exec/facts/k2-gen2.tsv 与 k2-libraryenv.tsv 的来源戳随 include 头改动过期(与 0.0.82 的 win-argv 同类,无内容改动),这两张表的生成器不在 derive 门,改上游时无门强制重算;重算后第二次整链(输入 f24ba9737d)01:39 到 02:06 约 27 分钟,rc 0,APE 与第一次字节相同。生成器总闸 tools/gen-all-check.cx 待 0.0.84 起链前落库。链目录 /tmp/unisacc-session-v083-chain2/f24ba9737d54acad8088d3edb0d83b71ade4e8bd。

速度账(tools/speed-ledger.cx --version 0.0.83 实跑):槽到起链 520.7 分钟;整链出口 546.5 分钟(整链 2/2 次);预跑完成 UNMEASURED(账本未取到,实际预跑 00:15 至 02:23 分段);公开 待填。

## 已知缺口

(1) 基本 asm 具名拒绝意味着含 asm 与 asm 标签与 pure 以外未登记属性(packed 等)的外部 C 代码现在编译失败(原为静默忽略);白名单可随需求扩充。
(2) kill(getpid(), sig) 不经屏蔽字;线程同步、原子与 TLS 未实现(0.0.87 起);裸 -I include 被静默忽略(INCLUDE-BARE-RELATIVE-I-IGNORED,补丁草案 0.0.84)。
(3) c-negative 53/9 续期(expires=0.0.85);12 格 netregen 指纹预期差异(expires=0.0.84);F4、F5、equiv0 与 equiv3 定因未做。
(4) rt-shared-gen 对 _unisa_lbin 的登记缺口(main 既有),生成器总闸与 R2 第二半与 S2 第二期未做;dvqualify --ids 缓冲 8192 字节(400 个 id 溢出)、devsvc 门内层 60 秒不随负载伸缩、bg 作业偶发 JOB_LOST。
(5) derive-flow 批次在 60 秒窗口内跑不完(check-derive-flow 并行跑 derive --check 与 derive-flow --check,窗口到点被杀,未出 FAIL 行),本版严格证据不含该批,记 UNMEASURED。
(6) 全套 s9 本版未跑;产品资格域本版仍非整域,严格证据所列批次之外为 UNMEASURED,不得称全批资格;外目标执行格(osx、win、arm64 原生)仍 UNVERIFIED。

## 未解决项(沿用)

(1) NN-VERIFY-WEAK。
(2) -S -funit 返回 NOT_IMPLEMENTED。
(3) 其余开放项见 build/pkg-gaps.tsv。

## 证据入口

release/evidence/0.0.83/INDEX.tsv;release/evidence/v0.0.83-strict-2026-11-20.tsv;release/evidence/0.0.83/expected-fail.tsv;release/evidence/0.0.83/expected-unmeasured.tsv;release/evidence/0.0.83/*-deferred.tsv;release/evidence/0.0.83/pack-run.out;release/evidence/0.0.83/cache-manager.tsv;release/evidence/0.0.83/warm-*.tsv。撤回方案:release/rollback.txt(回退 Latest 到 v0.0.82,不删标签与 release)。
