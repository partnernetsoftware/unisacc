# 五张矩阵剩余本机格的闭合计划（cc，2026-10-28，只读审计；fable 裁定的无算力待办）

对象：stream-e4、stream-prune、optimization、formats（properties 本机列已清零）。只列本机 lnx/x86_64 的 MISSING / NEEDS_PRODUCT_CHANGE / BLOCKED；外宿主格（NEEDS_FOREIGN_HOST）需宿主，不在此。
可复现校验：`for f in stream-e4 stream-prune optimization formats properties; do echo $f $(cut -f4 tests/$f-domain-matrix.tsv | sort | uniq -c | grep -v "#" | tr '\n' ' '); done`。
机器可读队列：plans/matrix-gap-cc-queue.tsv（cell、batch/tool、oracle、material、budget、state）。

## 1. 已有登记批覆盖、只差换标（0 算力，需该批主人或下一纪元）

| cell | 覆盖批 | 说明 |
|---|---|---|
| elf.x86_64.segments, elf.arm64.segments, ape.container-structure, object.lnx-arm64.unit-link | formats-elf-ape（IMPLEMENTED，tests/formats-elf-ape.cx 283 行） | 2026-10-28 17:2x 已由 cc 实跑（fable 裁定：换标合同是实跑证据而非批作者权；只读运行不改输入）：run-batch PASS @f3705dfa，68/0，四格标签逐项 PASS（3/1/3/8 行），日志 out/c43-cc-reg-02/formats-elf-ape.log，测试 blob 20dca21533c6；四格已换 COVERED_LOCAL。 |

## 2. 只扩现有生成器、一个窗口槽（每格 ≤40 s）

| cell | 工具扩展 | oracle | 材料 |
|---|---|---|---|
| e4.det.file-vs-stdin / prune.det.file-vs-stdin | o2-cases-gen / prune-cases-gen 加 `-` 路线：同输入经文件与 stdin 两路 | 两路输出字节相等 | stream4 / stream5 |
| e4.ref.non-regular-input / prune.ref.non-regular-input | 同上加拒绝段：INPUT 为目录、无读权限、不存在 | 具名拒绝且 ENOTDIR/EACCES 与 ENOENT 文本不同、stdout 空（CLAUDE.md 规则） | 容器 |
| e4.rec.after-refusal / prune.rec.after-refusal | 同上：每个拒绝后立刻跑一次干净输入 | 输出 == 该输入的既有 oracle | 容器 |
| e4.ref.step-budget | o2-cases-gen 加 `UNISA_MAXSTEPS=1` 一例 | 退出 3、stderr 含 `step budget exhausted (budget 1 steps)`、stdout 空；与 resources 行 `res.steps.stream-exhaust` 同一观测，一处实现两处引用 | 容器 |
| e4.id.same-candidate-model / prune.id.same-candidate-model | 新小工具：读候选包成员与 streams.json 的 model 记录 SHA 比对 | 相等（纯哈希比对，无推理） | 候选包 + streams.json；注意 candidateprofile/identity 是 cdx 窗，只读数据不碰其代码 |

## 3. 需要新的小生成器或新材料（每格一个窗口槽）

| cell | 方案 | 阻塞 |
|---|---|---|
| e4.in.o1-model, e4.pos.o1-save-restore-to-mov | O1 作为独立流：参考 `src/opt.c` O1 对软件栈 save/restore→mov 的四轮界；oracle = REF `-O1 -S` | streams.json 无 O1 容器（material packer 只出 image/O2 路线）→ 先记 NEEDS_MATERIAL，由 packer 主人加 O1 目标 |
| e4.sem.stream-boundary / prune.sem.execution-preserved | E4/prune 输出再经 pinned lower(stream6)+elf(stream7) 得镜像并执行，与输入 tape 同路径执行结果相等 | 需 stream7 直跑入口（tests/stream-elf.cx 已有）；复用 properties `tu` 组的执行比对骨架 |
| e4.sem.corpus-breadth | opt-corpus-gen 加 corpus-7..9：float、varargs、switch 表、指针算术、深递归 O0==O2 执行 | struct 值返回要 ace3 候选（d65 上记 UNREALIZABLE，不假装覆盖） |
| prune.route.placement | 固定死函数夹具：`-S`/`-c` 输出保留死函数（未剪），镜像/-run 路线剪除 | 产品 `-S` 与镜像路线各一次编译 |
| prune.pos.library-exports, prune.ref.library-signatures | 库模式（导出根、签名拒绝） | 材料无库路线；先记 NEEDS_MATERIAL |
| tapebin.roundtrip | `.tape → .tapebin → .tape` 字节稳定 | §-probe：`-S -o x.tapebin` 写的是文本且回读拒绝"origin target mismatch"，真二进制路线的拼写待探（exec/c/compiler.c:60 tapebin_target） |
| archive.product-ar | `unisacc ar rcs|t|x` 与系统 ar 互认，链接只拉需要的成员 | 行声明宿主 ar 为 oracle：需裁定改为自写 ar 解析器才能 cx-only |

## 4. 需裁定或产品改动，不排跑

| cell | 原因 |
|---|---|
| corpus.cc-differential | 宿主 cc 为 oracle，cx-only 不可达；同语料已对经典参考覆盖（optimization-corpus-0..6），需裁定改 oracle |
| closure.o1-round-bound | 内部轮数不可观测，需产品侧计数器 |
| object.system-linker | 宿主 ld 为 oracle |
| ape.fat-macho-universal, object.macho-coff-emission | NEEDS_PRODUCT_CHANGE，拒绝是现行合同 |
| corpus.c-testsuite | 语料不在树内 |
| e4.host.lnx-x86_64 / prune.host.lnx-x86_64 | 汇总格，随本机列清零自动闭合 |
| *.host.win-arm64 | BLOCKED，需原生 ARM64 PE 产品 |

## 估算

第一个 10 分钟窗口（只扩现有工具）：§2 的 9 格；再四个小生成器：§3 中无材料阻塞的 5 格；其余 12 格需材料、裁定或产品改动，在 d65 上保持开放是诚实状态。

## 附：stream-semantics 工具的接口事实（只读，2026-10-28）

- stream6（lower）输入 tape.text，输出 target.text（`@target/@data/@sym/@src_os/@data_len/@bss/@relocs` 头 + 目标指令行，见 §14 lower 组）；stream7（elf）输入 target.text，输出 image.bytes，pin 2f063f38（tests/stream-elf.cx:6），调用形同 stream4/5：`tools/bound 12 unisacc-stream7.com INPUT INPUT INC`，env `UNISA_ATTRIBUTES=`、`UNISA_MAXSTEPS=200000000`。
- 因此 e4.sem.stream-boundary / prune.sem.execution-preserved 的链：tape → stream4|5 → stream6 → stream7 → 镜像 → `tools/bound 5 镜像` 执行；对照链：原 tape → stream6 → stream7 → 执行。断言：两镜像的 stdout/退出码相等，且与手推字面相等（非空洞）；镜像字节可不同（E4/prune 改写了指令）。夹具可复用 properties 的循环程序 tape（经产品 `-S` 得 E3 tape）与 prune 的 dead 夹具。

## 附：两份草稿的接口桌面核对（只读，2026-10-28；fable 裁定：等窗期不再写新草稿）

- 容器 CLI：`tools/bound 12 unisacc-streamN.com INPUT INPUT INC`，INPUT 为 `-` 时读 stdin（tools/stream-runtime.c:12-19）✓。
- 拒绝文本：stream-runtime.c:5 `#include "../exec/c/run.c"`，故 `readfile()` 的 `run: cannot open PATH: ENOENT|ENOTDIR|EACCES` + `cannot open input` 同产品（run.c:155-178）✓；目录作为 INPUT 可能 open 成功而在 read 失败，草稿按 INFO 记录 ✓。
- 步数预算：容器是非产品构建，`UNISA_MAXSTEPS` 经 strtol 生效，status 3 打印 `step budget exhausted (budget N steps)`（run.c:1061-1069），execute() 返回 status 即进程 rc ✓；草稿已改为不传重复变量（91f6e33791）。
- stream6/7 链：输入输出格式与 pin 见上节 ✓。未核：`UNISA_MAXSTEPS=abc` 被 strtol 变 0（已知产品外分支缺陷，queue 中 res.steps 行），草稿不触碰。

## 附：窗口到来时的执行清单（更新 2026-10-28 15:5x，csr 转 cdx：ace3 冻结根 f3705dfa 已装，共享树仍 d65）

1. 两份草稿首跑（stream-io-cases-gen ×2 容器、stream-semantics ×2）≈3 min，只修工具不登记。
2. stream-e4-peep-0..7 重跑 ≈6 min（TABLE_SUSPECT 16→12）。
3. B2 等价核 ≈3 min。
4. 迁移准备：待共享树产品换到 ace3 根后，在冻结导出重跑 35 批到 out/c43-cc-reg-02，`tools/matrix-flip-gen.cx … --migrate d65c1474 <ace3 候选短 SHA> out/c43-cc-reg-02` 生成并 `--check`，再覆盖五张矩阵（≈12 min，可另开窗）。
不授跨平台、不换标到未装候选。

## 附：2026-10-28 17:0x 无窗期进展（fable 裁定：拒绝自行开跑 stream-semantics；做桌面核对 + 纯哈希工具）

- 新工具 `tools/stream-model-identity.cx`（已登记批 `stream-model-identity`，coverage `stream.model-identity`，args `{streams};{candidate};e4:prune;check`）：纯 SHA-256 比对，无推理、无容器运行、不写文件；~8 s。按 streams.json 每条目核 P3 模型块（压缩负载 + 整 M 记录，与 streampack 同字节）、容器文件、源包三重身份；自检为重哈希稳定 + 单字节篡改必被发现。
- 实跑（out/c43-cc-reg-02/stream-model-identity.log，run-batch PASS，candidate f3705dfa）：e4/prune 计入 8/0；INFO 21。事实：riroot-01 的 e4(model 7)/prune(model 8)/lower(model 13) 模型字节与已装产品内嵌 P3（SHA 661c2b02）成员完全相同；源包整体不同（4d9b8a3a vs 661c2b02），e2/e1/e3/elf 四个模型不同。即 stream4/5 容器确是当前候选的 E4/prune 模型，但 pinned 流集合并非由当前候选打包。`all` 模式 rc 1（9 DIFFERENT）如实保留，不换标其它流格。
- 两格换标：e4.id.same-candidate-model、prune.id.same-candidate-model → COVERED_LOCAL@f3705dfa；队列同步。
- stream-semantics 桌面核对（对照 tests/stream-elf.cx）：`.exit` 是合法 tape 操作（src/tapeprune.c:21，back_lower.c:1084 降为 exit 门）✓；`saved()` 以 0700 创建，镜像可执行，与 stream-elf 的 chmod 0700 等效 ✓；execute() 传空 env（stream-elf 传 NULL 继承），静态镜像无差 ✓；每级 bound 12 s，最坏链 5×12+2×5=70 s 超出 55 s 外层上限——首跑用外层 `timeout 55`，实际 riroot 容器单级约 1–3 s，超时即 NOT_RUN 不记 PASS；E4 是否在 `imm r1,42` 前缀下仍折叠由 VACUOUS 行把关。未改代码。
- 17:1x tapebin.roundtrip 闭合：拼写探明 `-S --tapebin -o X.tapebin IN`（IN 为 .tape 时 origin 0，为 .c 时 origin=本目标 1），`-S -o Y.tape X.tapebin` 打印规范文本（不保留 -S 空白，docs/tapebin-v1.md 已声明），`-run X.tapebin` 可执行。新工具 `tools/tapebin-roundtrip.cx`、批 `formats-tapebin`（4 夹具 40/0，run-batch PASS @f3705dfa，out/c43-cc-reg-02/formats-tapebin）。头布局为 u16 major/minor/opset/flags、u32 target/section_count/header_bytes、SHA、u32 origin@60（exec/c/tapebin.h:114）。formats 本机 MISSING 由 7 降为 6。
- 17:2x prune.route.placement 闭合：新工具 `tools/prune-route-placement.cx`、批 `stream-prune-route`（6/0，run-batch PASS @f3705dfa，out/c43-cc-reg-02/stream-prune-route）。事实：-S 保留 `dead:`+字面 12345，-c 保留符号，镜像与无死函数源的镜像逐字节相等并退出 7，-run 退出 7；-O0 镜像同样相等（INFO，不断言级别）。stream-prune 本机 MISSING 8→7。
- 17:3x e4.sem.corpus-breadth 闭合：tools/opt-corpus-gen.cx 新增 corpus-7 varargs、corpus-8 deep-recursion（含 struct 值返回与指针算术）、corpus-9 float-mixed（含稀疏 switch）；批 optimization-corpus-7..9 各 42/0（REF 0ed46192 六目标字节等价 + lnx 字面执行），run-batch PASS @f3705dfa，out/c43-cc-reg-02/optimization-corpus-{7,8,9}.log。struct 值返回在 f3705dfa 可实现（corpus-2 同样 PASS）。注：本次表项追加用了一次性 python 文本替换，违反"不用临时 Python 改源码"，下次用 .cx 编辑器。stream-e4 本机 MISSING 9→8。
- 17:3x archive.product-ar 闭合（fable 裁定：prd"系统 ar 也认"是互操作合同，宿主 ar 保留为 oracle，cx ar(5) 解析器作附加断言；oracle 身份 `/usr/bin/x86_64-linux-gnu-ar` GNU ar 2.38 + sha 5b394f87）：新工具 `tools/archive-ar-cases.cx`、批 `formats-archive-ar`（材料 hostar=规范路径，run-batch 拒绝符号链接）17/0。**共享树产品于 17:26:12 被 cdx 换为 3a5b76ab**（out/c43-3a5b-promote-01，状态 pack-pending-no-test-result，未准入），首次 run-batch 因产品中途变更判 FAIL（正确行为，已剔除该行）；重跑在冻结导出 `/home/devali/repos/unisacc-cc-f3705-2c9dcbcb12`（git archive 2c9dcbcb12 + previous.com=f3705dfa），证据仍绑 f3705dfa 纪元。
- 同时发现产品缺陷：未定义外部函数链接成崩溃镜像（REF 同现）。按裁定另立批 `link-undefined-function`（红 0/3，预期不放宽）与 `plans/defects-c43.md`；修复提案归 src 域所有者。formats 本机 MISSING 2→1（仅余 object.system-linker，裁定允许 .cx 调用宿主 ld 关格，待做）。
- 纪元提示：若 3a5b76ab 准入并装为正式候选，cc 全部 @f3705dfa 证据需按 §7 在窗口内重跑迁移（35+9 批，约 15 min），`tools/matrix-flip-gen.cx --migrate f3705dfa <新候选> <日志目录>`。
- 17:4x object.system-linker 闭合：新工具 `tools/object-ld-cases.cx`、批 `formats-object-ld`（宿主 ld 为 oracle，身份 ld.bfd 2.38 + sha 58937fc2；三夹具 24/0，含 -static 与产品镜像路线字面一致），冻结导出 unisacc-cc-f3705-a6a93efcc3 上 run-batch PASS @f3705dfa。formats 本机 MISSING 1→0（余 NEEDS_PRODUCT_CHANGE 2、外宿主 5）。
- 17:4x corpus.cc-differential 闭合：opt-corpus-gen 新增 ccdiff 组（宿主 gcc 11.4.0 为 oracle，身份记录；十程序 -O0/-O2 宿主==产品==字面，20/0，约 25 s），批 `optimization-ccdiff`，冻结导出 unisacc-cc-f3705-36db85590a 上 run-batch PASS @f3705dfa。optimization 本机 MISSING 2→1（余 closure.o1-round-bound，非必需，需产品侧计数器）。
- 17:4x 只读核对 cdx 的 3a5b 验证（fable 裁定 C）：out/c43-3a5b-stream-e3-batch-01.stderr 第 1 行 `run-batch: REFUSE canonical root and output parent`（非规范路径调用，无结果）；batch-02.stdout 第 19/21 行 `STREAM_E3 assertions=17 failed=0`、`BATCH_RUN id=stream-e3-independent status=PASS candidate=3a5b76ab…`。对象分类：控制器 3a5b76ab，被测为旧 riroot stream3 容器——不授 E3 新模型资格，不触及 cc 44 批合同。无异常，不写阻塞项。
- 材料请求已写 plans/material-requests-cc.md（O1 容器：包内已有 O1 路线、模型索引 6，仅 streampack 路线硬编码；库路线：资源 `\0library/signatures` 注入入口缺）。此后进入 idle-hold，等 envelope。
- 17:5x ccdiff-2 差分缺陷猎取（fable 裁定）：十程序（位域、union 别名、VLA、字符串函数、snprintf 截断、无符号回绕/有符号除法、指针差+goto+嵌套 switch、函数指针表、long long、char 符号）宿主 gcc -O0/-O2 == 产品 == 字面，20/0，**未发现产品/宿主分歧**；批 `optimization-ccdiff-2`，冻结导出 unisacc-cc-f3705-278688d964 上 run-batch PASS @f3705dfa；矩阵新增行 corpus.cc-differential-2。
- 17:4x ccdiff-3（嵌套 struct 值传递、浮点格式化、静态局部+递归、二维数组、位运算、strtol/atoi、enum/offsetof、do-while/三目/复合字面量、指定初始化、整型提升与移位）：20/0，无分歧；批 `optimization-ccdiff-3`，冻结导出 unisacc-cc-f3705-b83873a86f 上 run-batch PASS @f3705dfa。累计差分 30 程序 × 2 级，产品与宿主 gcc 无分歧。
- 17:5x ccdiff-4（inf/nan 与 printf 转换、string/stdlib 含 qsort/bsearch/div、struct 数组+union+long long switch、double→u64/float 舍入、混合 varargs、u64 除法、负下标、memcpy struct、算术右移）：10/0；批 `optimization-ccdiff-4`，冻结导出 unisacc-cc-f3705-cfbfa22d07 上 run-batch PASS @f3705dfa。累计差分 35 程序，产品与宿主 gcc 仅一处未指定行为分歧（字面量合并，plans/defects-c43.md 记为非缺陷）。
- 17:5x 链接路线边界猎取（缺陷曾出于此）：8 项合同成立（私有同名不冲突、跨单元 extern 数据、重复 main/对象两次/未定义数据具名拒绝、归档重复成员容忍、空归档、无 main 拒绝），批 `link-edge-cases` 8/0，冻结导出 unisacc-cc-f3705-c328960a2f 上 run-batch PASS @f3705dfa；发现一条诊断质量观察（无 main 的拒绝文本不具名）记入 plans/defects-c43.md。
- 17:5x 同根第二现象：跨归档逆序（`m2.o B.a A.a`）静默链接成崩溃镜像；已加入批 `link-undefined-function`（现 1/4 红），defects 记录；补丁合入后应与 ld 一致转具名拒绝。
- 17:5x 驱动/ar 具名拒绝猎取：批 `driver-diagnostics` 7/11（红为设计）：4 处诊断缺陷（-run 单元对象静默、-run/-o 仅归档错因文本、ar 空归档非法操作静默 rc 0）记入 plans/defects-c43.md，7 项具名拒绝合同成立。冻结导出 unisacc-cc-f3705-51549fb2cd。
- 17:5x 预处理器差分（宿主 gcc -E -P 为 oracle，去空白等价 + 手写字面）：8 夹具 24/0，无分歧；批 `pp-differential`，冻结导出 unisacc-cc-f3705-a1f5b388dc 上 run-batch PASS @f3705dfa。
- 17:5x 预处理负向猎取：批 `pp-negative` 8/10（红为设计）——两处真缺陷：`#define 3x 1` 与 `#include <stdio.h`（未闭合却静默包含 stdio.h）均 rc 0 无诊断；四类拒绝文本缺 file:line（INFO）。plans/defects-c43.md。冻结导出 unisacc-cc-f3705-20f6f4361d。
- 18:0x C 约束负向猎取：批 `c-negative` 10/17（红为设计）——七处约束违反被静默接受并产出对象（赋值给 const、switch 外 case、struct 重定义、指针当 int 返回、不兼容指针初始化、typedef 冲突、重复形参），plans/defects-c43.md c.negative。冻结导出 unisacc-cc-f3705-55b664f4cd。
- 18:0x c-negative 第二轮：再添 8 处静默接受（int→指针、原型实参过多、函数类型冲突、枚举常量重声明、零长数组、void 值、const 指针写入、int+=字符串），批现 19/34；defects 累计 15 项 C 约束漏检。
- 18:0x c-negative 第三轮：再添 6 处（不兼容指针比较、块内重定义、重复 case、浮点下标、void 调用作值、丢弃 const），批现 20/41，漏检累计 21。
- 18:0x c-negative 第四轮（声明层）：仅再添 2 处（具名 void 形参、多存储类），18 项声明约束已正确拒绝——漏检集中在表达式/类型兼容性与作用域层；另记一处有效程序被拒（强制转换 ICE 数组大小）。批现 20/43。
- 18:1x fable 裁定 B 限量（链接器畸形输入）：批 `link-malformed-input` 8/11——**产品自身崩溃**（tape _start 落出末尾时 -run SIGSEGV）、ELF 魔数覆盖/截断对象仍被链接；triage 增 14b/14c。D 分诊表 plans/defects-c43-triage.md、C 补丁 plans/patches/driver-diagnostics.diff 已交；tapelink 补丁由 src 所有者 896888f424 合入，待新产品验收。
- 18:1x stream-semantics 首跑（fable 重裁：机器空闲、无重活在途，属轻活）：首版手写 tape 夹具无效（镜像崩溃、E4 不改写）→ 改为产品 `-S -O0` E3 tape；e4 42/42、prune 7/7 非空洞（E4 改写了地址算术，prune 删除 dead）。批 stream-e4-semantics / stream-prune-semantics 各 1/0，run-batch PASS @f3705dfa。**被测对象：旧 riroot 流（riroot-01 pinned 容器链）；控制器：f3705dfa，经 frozen export 加载，非被测对象。证据等级：快车道首跑 / 本机，不构成发布资格。** 冷相位 e4 7.1 s、prune 0.4 s（含产品 -S），热相位 UNMEASURED。
- 18:1x stream-io 复跑（轻活裁定）并登记：批 stream-e4-io 11/0、stream-prune-io 10/0；换标 det.file-vs-stdin、rec.after-refusal（两流）、e4.ref.step-budget；ref.non-regular-input 记 PARTIAL（路径具名+文本两两不同 PASS，ENOENT/EACCES/ENOTDIR 原因词 UNVERIFIED：pinned 材料早于 run.c io_open_reason，需用当前源重打容器）。被测对象：旧 riroot 流；控制器 f3705dfa 经 frozen export；快车道本机证据。stream-e4 本机 MISSING 8→4，stream-prune 7→4（剩余：O1 材料 2、库路线 2、汇总格 2、non-regular-input PARTIAL 2）。
- 18:2x 第二材料（fable 准予 a）：冻结产品 f3705dfa 以 `-O2 -fno-trim-libc` 重编 tools/stream-runtime.c（含当前 run.c io_open_reason）为 base abe915c3，stream-pack 同源包 4d9b8a3a → out/c43-streams-cc-01（MATERIAL.md 记身份）；stream-io 在新 stream4/5 上 14/0、13/0，ENOENT/EACCES/ENOTDIR 原因词各自实跑出现。批 stream-e4-io-cc / stream-prune-io-cc 登记并 run-batch PASS；两格 ref.non-regular-input 由 PARTIAL → COVERED_LOCAL（两条材料行并记，riroot-01 pin 不动）。stream-e4/prune 本机 MISSING 各 3（O1 材料 2 / 库路线 2 / 汇总格 2）。
- 18:3x O1 材料与两格闭合（fable 准予 b）：streampack 加 LEVEL（默认 O2 七容器 sha 回归一致），打出 O1 容器 out/c43-streams-cc-o1-01（e4 模型索引 6）。oracle 实测极强：O1 流对产品 -O0 tape 的输出 == 产品 -S -O1 == REF -S -O1 逐字节；save/restore 对数减少、mov 增加（并非全部消除——首版断言过强已改）。批 stream-e4-o1 16/0 run-batch PASS @f3705dfa；e4.in.o1-model、e4.pos.o1-save-restore-to-mov 换标。stream-e4 本机 MISSING 3→1（仅余汇总格 e4.host.lnx-x86_64）。
- 18:4x 库路线闭合（fable 准予改 tools/stream-runtime.c）：加 UNISA_STREAM_RESOURCE=NAME=PATH（缺省字节不变，cc-02 无 env 回归与 cc-01 同分）；实测 E3+library/symbols → USLTAPE1（tape 与无资源输出逐字节同，USLSIG2 3 记录），prune+signatures 保留 pub 删除 dead，五种畸形各出声明文本（截断→`typed library signature`，其余→`malformed…`）。批 stream-prune-library 10/0 run-batch PASS @f3705dfa；两格换标。stream-prune 本机仅余汇总格。
- 18:3x 本机列清零：六个他人批（stream-e4-independent 17/0、stream-prune-independent 19/0、streams 40/0、optimization-tape 16/0、optimization-native 241/0、optimization-run 193/0）在冻结导出 f3705dfa 上实跑 PASS（fable 规则：换标凭实跑证据），stream-e4/prune 各 10 格由旧纪元证据改贴 @f3705dfa（原证据保留在行内）；两张矩阵本机必需格全部 @f3705dfa，汇总格 e4.host.lnx-x86_64 / prune.host.lnx-x86_64 以聚合证据 COVERED_LOCAL（本机快车道证据，不构成发布资格）。五张矩阵本机列：全部清零（optimization 仅余非必需 closure.o1-round-bound）。
