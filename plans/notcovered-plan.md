# not covered 编译覆盖缺口:普查、影响度与分期修复计划

日期 2026-11-15,工单 fnotcov(fable-notcov)。取数产品:共享检出已装 unisacc.com(0.0.67 线)。事实表:`tests/notcovered.tsv`(本计划的棘轮底表)。复现源与实跑输出留在 `$CLAUDE_JOB_DIR/tmp/notcovered/`(作业目录,非入库;第 0 期迁入 `tests/notcovered-cases/`),关键最小复现已原文抄在本文件第 3 节,不依赖该目录存活。

## 1. 结论

- 拒绝文案不在 C 源里,在网表里。`cx/`、`src/` 的 .cx 源没有任何一处发出 "not covered"(仅 `cx/compiler/rtsharedname.cx:19` 一条无关注释);全部文案是 `exec/` 下事实表与网表里的 REJECT 动作文本,共 267 条不同文案(`exec/**/*.tsv` 去重,不计 `.md` 与 `probes/`)。所以修复一律走"事实表 -> 生成器 -> 网",不存在可加特判的 C 位置。
- 267 条里按性质分:gap(真正的 C 写法未覆盖)159,diag(用户错误的正当诊断,只是带了 "not covered" 前缀)61,frame(内部帧或流格式畸形,用户不该看到)29,cap(硬上限)18。
- 已实跑约 100 个复现(r、t、x、y 四组单单元 80 个,s、u、v 三组多单元与畸形源 15 个,两份真实文件的删减变体 10 个)。确认被拒的文案 17 条,确认现已接受的 16 条(表里文案仍在,但常见写法已通),其余 234 条未实测,一律记 UNMEASURED,不作结论。
- 两份真实复现(commit-own 移植)已定因,都不是数组大小,而是源文件本身花括号不配平,多单元路径把它报成了不相干的文案:
  - `malformed unit frame`:文件作用域多出一个 `}`(原文件 main 的函数头被误删,函数体成了文件作用域的块)。命中 `exec/parse2/units-result.tsv` 152 行六种条件里的"花括号深度为负"。3 行即可复现(u03)。单单元路径对同一错误给的是带行列的 `not covered: top-level construct`,多单元路径给的是无位置的 `reject:`。
  - `errno.h:17:17 block-scope extern object ...`:main 里少一个 `}`(原文件 269 行的块未闭合)。用户单元在文件尾仍处于函数体内,后续单元的头文件文本被当成块作用域解析,`extern int errno;` 于是成了块作用域 extern。7 行即可复现(v03)。把原文件 272 行删掉配平后即编译通过(e8 实跑)。
  - 因此记忆宫殿里"函数局部约 8.9 KB 或 main 内 char[4400] 块触发 malformed unit frame"的说法不成立:8.9 KB 局部、嵌套块 char[4400]、多单元加 errno 嵌套块实跑全部通过(r03、r04、s03、s04)。真正的缺陷是诊断:畸形源在多单元下得不到"哪个文件哪一行少了括号"。
- 大型自动数组与帧缺陷单(fframe)结论一致:帧达 1000000 字节报 `an op outside the first encoder slice`(r01、r02),根因是 E3 control 网帧大小回填槽 7 宽,已在 origin/v0.0.69 改 10 宽(3babcd8287、cb76511e00)。本次另测到一条该单未覆盖的现象:恰好 `char a[900000];` 的自动数组报 `not covered: address or target declaration`(t08;只读首元素或用变量下标同样拒,与下标常量无关),而 500000、700000、800000、850000、900016、950000、999000 字节全部通过,999999 与 1000000 报回填槽文案,同样大小的 static 数组通过。这是一个点状拒绝,不是范围上限,根因未定,列入第一期定因。
- 真正属于"正常 C 被拒"的类,按影响排序见第 4 节;第一期(0.0.69)范围见第 5 节。

## 2. 普查:267 条文案的分布

取数命令(作业目录内的 gen.cx,第 0 期入库为 `tools/notcovered-census.cx`):遍历 `exec/` 全部文件,取 `not covered:` 起到制表符、引号、反斜杠或行尾的文本,去重,记出现次数与所在网(目录加表名前缀)。

| 类(class) | gap | diag | frame | cap | 合计 | 主要所在网 |
|---|---|---|---|---|---|---|
| pp 预处理 | 19 | 17 | 1 | 3 | 40 | exec/pp 的 rescan、hash、expression、literal、directive、cli、shared |
| library-abi 库合同与宿主 ABI | 32 | 2 | 6 | 2 | 42 | exec/lower/library*、exec/parse2/library*、exec/c、exec/nativeabi、ccinterop |
| array-init 数组、初始化与字符串 | 25 | 5 | 3 | 0 | 33 | parse2 的 control、shape、initializers、statics、tentative、exec/facts/strings |
| type-decl 类型与声明符 | 23 | 4 | 1 | 3 | 31 | parse2 的 control、gen2、shape、enumtypes、bitfields、floatconst、ladder |
| function-call 函数、调用与函数指针 | 14 | 4 | 3 | 5 | 26 | parse2 的 callcontrol、functiontypes、return、printfcontrol、arity、setjmpcontrol |
| backend 后端与目标格式 | 15 | 2 | 4 | 1 | 22 | exec/enc、exec/lower、exec/pipeline、exec/opt、exec/prune |
| constexpr 常量表达式 | 11 | 6 | 0 | 1 | 18 | parse2 的 constexpr、vla、offsetofcontrol、control |
| syntax-diag 语法诊断 | 1 | 13 | 0 | 0 | 14 | parse2/control 加 exec/facts/k2-errors.tsv(映射成参考诊断文本) |
| expr-stmt 表达式与语句 | 7 | 2 | 2 | 2 | 13 | parse2 的 control、gen2、update、ladder |
| scope-decl 作用域与声明可见性 | 6 | 5 | 0 | 0 | 11 | parse2 的 control、statics、enumtypes、gen2 |
| units 多单元与单元帧 | 5 | 0 | 5 | 1 | 11 | parse2 的 units、tokenlocations、layoutprovenance、exec/enc/unitcontainer |
| lex-frames 词法与位置帧 | 1 | 1 | 4 | 0 | 6 | exec/lex 的 location、sourcefacts、exec/parse 的 token* |

说明:
- 分类由 gen.cx 按所在网与文案关键词机械判定,是初分,边界行(例如 `width`、`type`、`declaration` 这类一个词的文案)需在第 0 期逐行复核;kind 列同理。
- "同类拒绝文案"不止 "not covered"。实测还有三族未带该前缀的拒绝:`reject: multiple definitions of this function`、`more than 32 uses in one unit`(驱动,候选已放到 48)、`field overflow`(core_field_fill)。本次未普查这三族,列为第 0 期补查项。
- `exec/facts/k2-errors.tsv` 把 11 条 "not covered: expected X" 之类改写成参考诊断文本(例如 `expected ')'`),所以用户看到的 `expected ')'`、`this is not the start of an expression`、`unknown identifier` 背后也可能是覆盖缺口而不是用户笔误(y05、y07、y08 即是)。
- 现成工具核对:`tools/tape-gap.cx` 只查某目标 lowering 缺口(输入 TAPE 与 TARGET),不覆盖前端拒绝;`build/tool-index.tsv` 里没有任何枚举拒绝文案的工具,本普查不重复已有能力。

硬上限(kind=cap,18 条,按主调度要求单列为缺口类):单元数 128(units 网 `CMPI unit 128`,曾由 64 放到 128,提交 1c66a90b3d、9c8c45f366)、每单元 use 32(候选 48,驱动文案)、调用实参多于 6 个(文案仍在,8 个整型实参实跑已通过,r14)、host 前推调用数 8192、#include 拼接 4000、头文件续行 4096、#error 文本 283 字节、结构体编号容量、结构体成员容量、shape 描述符容量、enum 描述符池、函数签名池、递归签名等价上限、签名图等价预算、setjmp 临时深度与嵌套深度、对象重定位容量、整型常量超 int、有序源布局事实上限。每条的实际数值与是否可达须在第 0 期逐条实测后填表。

## 3. 实跑复现(已装产品,默认 -run)

被拒(17 条文案;预期输出按 C 语义给出,修复后的验收值):

| 复现 | 最小形状 | 现状 | 修复后预期 |
|---|---|---|---|
| r01 | `char a[1000000];` 写首尾后打印和 | `reject: not covered: an op outside the first encoder slice` | 输出 3 |
| r02 | `int a[600][600];` | 同上 | 输出 3 |
| t08 | `char a[900000];`(仅此大小,相邻大小通过) | `reject: not covered: address or target declaration` | 输出 3 |
| u03 | 多单元(有一条 use),main 之后文件作用域多一个 `}` | `reject: not covered: malformed unit frame`,无文件无行号 | 带 文件:3:1 的错误,文案指出多余的 `}` |
| u01 | 多单元,函数之后直接跟 `{ ... } return 0; }`(函数头丢失) | 同上 | 带 文件:行:列 的错误(单单元路径现给 2:5 `top-level construct`) |
| v03 | 多单元,main 里多开一个 `{` 未闭合 | `errno.h:17:17: error: not covered: block-scope extern object without a visible file-scope declaration` | 带用户文件名与文件尾行号的 `expected '}'` |
| x16 | `static int f(void){ extern int shared_v; return shared_v; }` 之后才 `int shared_v = 5;` | `block-scope extern object without a visible file-scope declaration` | 输出 10 |
| y01 | `static struct s one; static char g1[sizeof one];` | `1 行:3:23 not covered: constant expression` | 输出 16 |
| y02 | `static int tab[] = {1,2,3}; static char g3[sizeof tab / sizeof tab[0]];` | 同上 | 输出 3 |
| y12 | `enum { NT = sizeof tab / sizeof tab[0] };` 再作数组长度与 case 标号 | 同上 | 输出 `3 1` |
| r09 | `_Static_assert(offsetof(struct s, b) == 4, "off");` | `not covered: nonconstant bound` | 输出 4 |
| s01 | 模块 nm1:`typedef struct { int a; } tt; int nm1_f(int (*fn)(tt *, int), void *c)`;另一单元 use 后调用 | `nm1.cx:3:31 not covered: incompatible semantic declaration type` | 输出 5 |
| t04 | `int (*q[2])[4];` | `not covered: array of pointers to arrays` | 输出 7 |
| t05 | `int (**pp)[4] = &p;` | `not covered: pointer to pointer to array` | 输出 7 |
| t10 | `typedef int row_t[4]; static int h(row_t rows[2])` | `not covered: array parameter whose element is a typedef array` | 输出 8 |
| t03 | `typedef int fn_t(int); static fn_t inc2;` 之后定义并调用 | `not covered: call through a non-function`(经强转时报 `not covered: width`,r27) | 输出 3 |
| t09 | `#include <sys/stat.h>` 后 `st.st_mtim.tv_nsec` | `not covered: member access` | 输出 5 |
| y09 | `_Noreturn static void bye(int c){ exit(c); }` | `1:1 not covered: top-level construct` | 输出 x |
| y05 | `char a[sizeof(int[4])];` | `expected ')'` | 输出 16 |
| y07 | `_Alignas(16) char buf[32];` | `expected ';'` | 输出 0 |
| y08 | `_Alignof(long)` 与 `struct __attribute__((packed))` | `this is not the start of an expression` | 输出 `8 5` |

实跑通过(记 ACCEPT,对应文案仍在表里,说明只剩更窄的形状会触发,未再细分):8.9 KB 局部与嵌套块 char[4400](单单元与多单元)、errno 嵌套块赋值、goto、嵌套结构体成员、结构体赋值、结构体返回(局部、全局、三目)、8 个实参、可变参数定义与调用、单元内函数指针形参含结构体指针、三维四维数组、double 复合赋值与自增、无符号混合运算、复合字面量、全局初始化(指针、指定成员、字符串表)、相邻字符串与转义、三目两臂异型、位域与匿名成员与柔性数组、可变参数宏与 `##`、括号左值、十六进制浮点与后缀、VLA、各式数组初始化、setjmp、静态局部、指针算术、sizeof 表达式、结构体按值传参、结构体内函数指针表、返回函数指针的函数、局部 struct union enum typedef、stdint 各宽度、文件作用域 `sizeof(类型)` 与成员 sizeof 作数组长度、局部 `sizeof 对象` 作数组长度。

真实复现文件删减记录(原文件已留底 `commitown-port-repro-frame.cx`、`commitown-port-repro-extern.cx`):
- frame:文件内无 `int main`(grep 实证),258 与 287 行各结束一个 `natives` 定义(重复),288 行起是文件作用域的 `{ int ai; ...`,末行 `}` 无配对。删去 225 至 258 行(去重)仍拒;u01、u03 两个 3 至 5 行的例子给出同一文案。
- extern:删 main 体 257 至 322 行 -> 通过(e1);删 29 至 255 行全部辅助函数 -> 仍拒(e4);再删 263 至 320 行 -> 通过(e5);269 至 273 行为 `{ char root[4400]; ... { char root_local[4400]; }`,少一个 `}`;只删 272 行 -> 编译通过(e8)。
- 对照表 `exec/parse2/units-result.tsv`:DEAD.frame 的入边在第 6、9、12、24、77、102、106、150 行,152 行统一发 reject2。六种条件里 frame 复现对应"花括号深度为负"(counter-sub 段 `$state21` 观测 0 即深度已为 0 时再减);extern 复现不经 DEAD.frame,units 网放行了一个文件尾深度不为 0 的用户单元,拒绝由 control 网在后续单元的头文件文本上发出。精确到表行的确认(在 sim 上单步)留给第一期首个任务,本次只凭源形状与最小例实跑定因。

## 4. 影响度排序(真实受阻次数 乘 绕过成本)

证据来源:`git log --all --grep`(含 "not covered" 的提交 114 个,10 月 25 日以后约 19 个)、`build/pkg-gaps.tsv`、`prd_mempalace.md` 阻塞节、源码注释。仓内现有 tools 与 tests 的 .cx 都能编过(否则批次早已红),所以静态扫描只能看到"已经绕过之后"的形状,受阻次数以有文字记录的为下限,真实值更高;成本为估计。

| 序 | 缺口 | 有记录的受阻 | 绕过成本 | 判断 |
|---|---|---|---|---|
| 1 | 多单元下花括号不配平的诊断(malformed unit frame、errno.h 块作用域 extern) | 今日 2 次(commit-own 移植);历史上同一文案还被单元数上限复用(389dc30249:64 单元起拒),提交 3 个 | 很高:文案不指向文件与行,两次都被误判为帧大小缺陷,造了 6 个缩小例、改写了循环与局部数组、误记进记忆宫殿并派了一张定因单 | 第一期首项 |
| 2 | 跨单元函数指针形参含结构体指针(incompatible semantic declaration type) | 0.0.58 cx.stream(pkg-gaps 2026-11-12)、0.0.67 修门回退 R3 的 T4(f7537dce4d)、记忆宫殿阻塞行,共 3 次 | 高:公开接口被迫用 `void *` 加调用方强转,类型检查失效,R3 去强转至今挂起 | 第一期 |
| 3 | 文件作用域 `sizeof 对象` 与 offsetof 作常量表达式 | devsvc-core.cx:878(今日)、G15 DEBT(f1957e9b8e,offsetof 静态断言,2 个受影响用例),共 2 次 | 中:改写成字面量或宏,丢掉与类型同步的保证;静态断言只能删或挪到运行期 | 第一期 |
| 4 | 帧达 1000000 字节(回填槽)与 900000 字节现象 | 今日 1 次(大型二维自动数组) | 中:改 static 或堆分配即可绕过,但文案完全不指向原因 | 第一期(修复已在分支) |
| 5 | `_Noreturn`、函数 typedef 声明函数、`_Alignas`、`_Alignof`、packed、`sizeof(类型[N])` | `_Noreturn` 1 次(9fe7f20ac4:为此造了 tool_die 与 cx-lint R3-die 规则) | 中低:换写法即可,但 `_Noreturn` 的绕过是加了一条 lint 规则 | 第二期 |
| 6 | 宿主头结构体的嵌套成员(st_mtim) | 1 次(pkg-gaps 2026-11-09) | 中:读不到纳秒 mtime,取证退到宿主命令 | 第二期 |
| 7 | 声明符形状:指向数组的指针的数组、指向数组指针的指针、元素为数组 typedef 的形参 | 0 次有记录 | 低 | 第三期 |
| 8 | 块作用域 extern 先于文件作用域定义(x16) | 0 次有记录(今日那次实为第 1 项) | 低 | 第三期 |
| 9 | 硬上限 18 条 | 单元数上限 2 次(64 放到 128);use 上限 2 次(32 放到 48,refactor.cx 与 check-cxmod-tri.cx 有绕过注释) | 中:触顶时文案不带上限值与当前值 | 第三期先改文案,数值按需 |

## 5. 分期

主调度已裁定:0.0.69 = 编译覆盖缺口修复,原 0.0.69 至 0.0.72 顺延一号。版本序表原 0.0.73 至 0.0.78 已占用,顺延一号会与 0.0.73 相撞,所以本次把 0.0.69 之后的全部槽位(原 0.0.69 至 0.0.78)统一顺延一号,见 `build/version-seq.tsv`。

对顺延的评估(合理,有三点代价):
- 依赖上无冲突。R1(现 0.0.70)只要求在 0.0.68 方案 A 之后;内存安全乙期的约束是"不早于 0.0.69",顺延后仍满足;链接单元一期、netraw 与本版无输入交集。
- 代价一:netraw 第三次顺延(0.0.67 -> 0.0.69 -> 0.0.72 -> 0.0.73),E4 网络面三平台再晚一版。
- 代价二:生态门推算各晚一版(`prd_cx.md` 201 行的 G0 推算 0.0.76、G1 推算 0.0.78 须改为 0.0.77、0.0.79;该文件及 `prd.md` 正文其余提到旧版本号的句子不在本工单写域,见第 8 节遗留)。
- 代价三:v0.0.69 分支已有帧槽提交,与进行中的 0.0.68 同改 E3 相关表的可能须在合入前核(`git diff v0.0.68...v0.0.69 -- exec/parse2`)。
- 类别上成立:本版全部改动落在 E3 前端网的事实表(exec/facts/k2-*.tsv 与 exec/parse2 各网表)及其生成器,属"模型与网类"一类大改,不含驱动、库包或 cxmod 改动。

### 第 0 期(不占槽,工具,在 main 上,约 2 小时):棘轮落地

- 做什么:(1) `tools/notcovered-census.cx`(由作业目录 gen.cx 入库,`--check`、`--write`)重算 `tests/notcovered.tsv` 的 message、sites、nets 三列;(2) `tools/check-notcovered.cx` 加批次 notcovered;(3) 复现源迁入 `tests/notcovered-cases/`,每个带期望(REJECT 文案或 ACCEPT 输出);(4) 并入 `tools/ratchets.cx` 成第 14 项。
- 验收:`sh ./unisacc.com tools/notcovered-census.cx -- --check` 末行 `rows=267 drift=0`;`sh ./unisacc.com tools/check-notcovered.cx` 末行 failed=0;故意往任一网表加一条新文案后 `--check` 非零(负控)。

### 第一期 = 0.0.69(模型与网类:前端覆盖缺口一期,分支 v0.0.69)

范围按第 4 节前四项,共四个工作包,写域互不相交,可并行:

(1) 单元帧诊断(序 1)
- 修什么:多单元下花括号不配平给出带用户文件与行列的诊断;units 网的 reject2 按原因拆成具名文案(单元数达上限并带上限值、单元长为 0、范围越界、多余的 `}`、文件作用域 static 未结束、括号下溢);用户单元文件尾深度不为 0 时在该单元内报 `expected '}'`,不让后续单元文本落进它的块作用域。
- 根因层:facts 与 generator。`exec/facts/k2-units-tokens.tsv`(文案)与 units 网规则 -> 生成器 -> `exec/parse2/units-result.tsv`、`units-reject.tsv`;位置由 tokenlocations 网已有的单元起止给出。不在 drive.cx 加任何预检特判。
- 首个任务:在 sim 上对 u03、v03 单步,确认各自落在表的哪一行(本计划凭形状推定的"深度为负"与"文件尾深度不为 0 被放行"须由单步证实)。
- 验收:u01、u03、v03 的 stderr 含用户文件名与行号,且不含 `malformed unit frame` 与 `errno.h`;两份真实复现原文件报出 288 行与文件尾附近的位置;单单元路径输出不变。

(2) 跨单元函数指针形参类型等价(序 2)
- 修什么:导入接口的原型与定义里,函数指针形参带结构体指针(typedef 结构指针、struct 指针、const 结构指针)时判为相容。
- 根因层:facts 与 generator。`exec/parse2/functiontypes` 网的语义声明类型等价(文案 `incompatible semantic declaration type` 仅此一处),缺的是形参位置上结构体类型身份跨单元的归一;同单元内已通(r16)。
- 验收:s01 输出 5;`cx/stream/pipe.cx` 的阶段回调可改回 `xs_item *`(R3 的 T4 去强转,作为随行消费者验证,不改则至少新增一个模块夹具);负控:形参结构体类型确实不同的仍被拒。

(3) 文件作用域常量表达式里的对象 sizeof 与 offsetof(序 3)
- 修什么:`sizeof 对象`、`sizeof 数组 / sizeof 数组[0]` 用于文件作用域数组长度、enum 值、case 标号;`offsetof` 用于 `_Static_assert` 与数组长度。
- 根因层:facts 与 generator。`exec/parse2/constexpr` 网(reject2、reject3)不查已声明对象的大小;局部作用域同一写法已通(y11),说明 control 网侧已有对象大小事实,缺的是 constexpr 网的入口。sizeof 结果依赖数据模型(win 的 long 为 4 字节),必须读 `exec/parse2/datamodel` 事实,不写常数。
- 验收:y01 输出 16、y02 输出 3、y12 输出 `3 1`、r09 输出 4;G15 DEBT 登记的两处受影响用例转绿并销债;`tools/devsvc-core.cx` 878 行可改回 sizeof 写法(随行);负控:真正非常量的长度在文件作用域仍被拒。

(4) 帧槽收尾(序 4,接 fframe 已有提交)
- 修什么:槽宽 10 的候选构建、四处金样更新、其余目标验证;900000 字节现象定因并修(t08)。
- 根因层:第一部分 facts(`exec/parse2/control-result.tsv` 1277、1317 行与 `cx/core/output.cx:16` core_field_fill 的槽宽合同);第二部分未定,文案出自 `exec/enc/address`;因为只有 900000 这一个大小触发、相邻大小都通过,更像某个帧内偏移的十进制文本或编码与地址网的某个模式相撞,而不是范围限制,先在 sim 上取该例的 tape 定因再定层。
- 验收:`sh ./unisacc.com tools/check-large-frame.cx` 末行 failed=0 unverified=0(0.0.69 起拒绝即 FAIL);r01、r02、t08 各输出 3。

整版验收(全部须实跑,未跑不记 PASS):
- `tests/notcovered.tsv` 里本期涉及的 9 条文案 measured 由 REJECT 转 ACCEPT 或文案被具名文案取代,行数只减不增;`tools/check-notcovered.cx` failed=0。
- 回归:`sh ./unisacc.com tools/affected.cx -- --files <所动表与生成器> --run` 末行 fail=0;`sh ./unisacc.com tools/ratchets.cx` 全过;七个基线源 `-o` 产物 sha256 改前改后相同(覆盖放宽不得改变已接受程序的产物;帧槽加宽会改变序言文本宽度,若 sha 变化须逐源说明并更新金样)。
- 自举:用 bootstrap-0.0.59 多单元编译新源通过(新表不得使用旧引导产品拒绝的写法;尤其第 (3) 项落地前,产品源自身不得改用文件作用域 sizeof 对象);两代不动点一致。
- 六目标:前端网与目标无关,但 (3) 依赖数据模型、(4) 改序言模板,六目标各跑 r01、y01 的对象路编译(lnx 两架构实跑,osx、win 四格按 s9 档),未跑的格如实标 UNVERIFIED。

风险:
- 放宽等价判定(2)可能让原先被拒的错误程序通过,必须带负控。
- 引导产品 0.0.59 的内嵌网是旧的,凡产品源与工具源在本版内不得依赖新接受的写法,否则整链在 g1 停(0.0.59 整链曾因新编译器拒编 cx/stream/item.cx 停过一次,5f5c74a9bb)。
- 四个包同改 E3 表,虽不同网,生成器若共用须串行提交。
- 估时:(1) 3 小时,(2) 4 小时,(3) 4 小时,(4) 2 小时加定因 1 小时;并行后日历时间约半天,冻结到公开按现状 230 至 310 分。超过 2 小时的单包先拆(例如 (3) 拆成 sizeof 与 offsetof 两步)。若 (2) 的等价归一牵动签名池结构,移到第二期,不拖本版。

### 第二期(建议入候选池,类别同为模型与网类,一版):说明符与类型形状

- 修什么:`_Noreturn`、函数 typedef 声明函数(t03、r27)、`_Alignas`、`_Alignof`、`__attribute__((packed))`、`sizeof(类型[N])`、宿主头结构体嵌套成员(t09)。
- 根因层:facts 与 generator。control 网的顶层构造与声明说明符词表(`exec/facts/k2-control.tsv`)、unarycontrol 与 membercontrol 网;packed 另涉 layoutfacts 与 nativeabi 表,若牵动 ABI 则单拆。
- 验收:y05、y07、y08、y09、t03、r27、t09 转正确输出;`tools/cxlib/die.cx` 的 tool_die 可标 `_Noreturn`(cx-lint R3-die 规则保留)。
- 风险:packed 与对齐改布局,六目标 ABI 须逐格核。估时 6 至 8 小时。

### 第三期(建议入候选池,一版):声明符长尾、作用域与硬上限文案

- 修什么:指向数组的指针的数组、指向数组指针的指针、元素为数组 typedef 的形参(t04、t05、t10);块作用域 extern 先于定义(x16);18 条硬上限文案统一带"上限值与当前值",逐条实测可达性,按需放宽。
- 根因层:shape 网与 control 网(声明符),control 网(extern 可见性),各网的 CMPI 上限常数改读一张上限事实表(新表 `exec/facts/k2-caps.tsv` -> 生成器 -> 各网),不再散落字面量;验收须遍历该表所有消费者。
- 估时 6 小时;上限放宽每条单独评估内存与时间。

### 之后:234 条 UNMEASURED

每版随行清 20 至 30 条(造最小复现,实跑,填 measured),优先 gap 类里 array-init 25 条与 type-decl 23 条。pp 类 19 条 gap 多为 `#if` 字面量与 `##` 操作数形状,library-abi 32 条属库模式专用,优先级低。

## 6. 长期机制:not-covered 覆盖度棘轮

事实表 `tests/notcovered.tsv`(已入库,267 行)。列:message(键,文案原文)、class、kind(gap、diag、frame、cap)、sites(出现次数)、nets(所在网)、measured(REJECT、ACCEPT、UNMEASURED)、repro(复现名)。

规则(由第 0 期的 check-notcovered.cx 执行,并入 ratchets):
(1) 只减不增:`exec/` 里出现表外的新 "not covered" 文案即失败。确需新增拒绝时,必须同一提交在表里加行,kind 不得为空,且 repro 列给出已入库的最小复现;gap 类新增还须在 `prd_mempalace.md` 阻塞节登记缺口。
(2) gap 行总数是棘轮值(当前 159),只许下降;一条文案从表里消失或 kind 由 gap 改 diag 时,必须有对应复现由 REJECT 转 ACCEPT 的实跑记录。
(3) measured 不得由人工改成 ACCEPT:检查实跑 repro,以实际退出码与输出为准;UNMEASURED 行数只许下降。
(4) frame 类 29 条是内部帧畸形,用户不应看到;任何用户源能触发 frame 类文案都算缺陷(本次的 malformed unit frame 即是),复现入库后该行改判并排修。
(5) diag 类 61 条逐步去掉 "not covered: " 前缀(它们是正当诊断),经 k2-errors.tsv 映射成参考文本;去前缀不算新增。
(6) 文案必须可定位:不带 文件:行:列 的 `reject:` 形态只允许出现在与源位置无关的后端类;前端网的拒绝一律带位置。

## 7. 与帧缺陷单(fframe)的对齐

- 一致:大数组根因是回填槽宽,修在表,不在 C。
- 修正:malformed unit frame 与数组大小无关,fframe 的推测"花括号深度或作用域在单元边界失衡,与 errno.h 的 block-scope extern 同向"成立,且两者是同一类用户错误(括号不配平)在多单元路径上的两种表现:多一个 `}` 报前者,少一个 `}` 报后者。
- 新增:900000 字节单数组的点状拒绝(t08)不在 check-large-frame 的用例里,建议该检查补一例,并补 999999 字节一例(帧含对齐后达 7 位数上界,现报回填槽文案)。

## 8. 遗留与未验证

- 未实跑:234 条文案;六种 DEAD.frame 条件与表行的逐条对应(只证了"多余 `}`"一条的源形状);t08 根因;硬上限 18 条的实际数值与可达性;osx、win、arm64 目标上的任何复现(全部只在 lnx/x86_64 默认 -run 实跑)。
- 抽样实编全部 tools 与 tests 源未做:它们已在批次里常态编译,受阻形状已被绕过,实编不会给出新信息;影响度靠文字记录估计,是下限。
- 版本号顺延只改了 `build/version-seq.tsv` 与 `prd.md` 版本表的行首版本号;`prd.md` 正文其余句子与 `prd_cx.md`(201、440、1604 行等)里的旧版本号未动,需主调度或各节维护者跟进。
- 工具缺口:`devsvc doc` 的节路径按 `/` 分层,`prd.md` 9.4 节标题自身含 `build/version-seq.tsv`,因而该节无法寻址(ENOSEC),候选池摘要改用 `devsvc f edit` 写入;建议 flowdoc 支持对标题内 `/` 转义或按节号前缀匹配。
- 记忆宫殿 90、91 行及"8.9 KB 触发 malformed unit frame"的说法需按本文件第 1 节更正(本工单不碰 `prd_mempalace.md`)。
- (2026-11-15 cc)产品 -run 编译缓存按源路径取键,同内容换路径即冷编(6 至 10 s 起,devsvc 启动器实测 6355 ms 对 187 ms),拖慢所有夹具类检查与新克隆首调用;非 not covered 类,但属 0.0.69 同期应修的产品缺陷,建议并入第一期或单列一包:键改内容(源闭包 sha 加产品 sha),路径不入键(含 __FILE__ 的单元除外)。证据与 devsvc 侧绕法见 plans/devsvc.md 第 6 节。
