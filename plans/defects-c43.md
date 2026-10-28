# c43 已知缺陷登记（cc，2026-10-28 起）

## link.undef-function-refusal — 未定义外部函数链接成崩溃镜像（开放）

2026-10-28 cdx 实测更新：共享3a5b完整原批为1 PASS/4 FAIL，收据
`out/c43-3a5b-link-undef-01/results.tsv`；object/archive/reversed三个错误镜像rc139，direct-C为rc127，不能统称SIGSEGV。
本刀`tl_check_undef`检查所有未解析extern，仅豁免随后生成的`__init`；诊断有界截断，不再跳过长名。
修改源码经现有build_ref构造的独立参考`out/c43-link-undef-fix-01/reference.elf`已实测：
main.o、main.o none.a均rc1并具名used；m2.o B.a A.a具名inner且rc1；三者未产生镜像。
正序m2.o A.a B.a仍成功且运行rc106。首次归档试命令误写lib.a，得到cannot open，不计通过；随后正确none.a实测如上。
直接C路径仍生成动态ELF并rc0，说明仅修tapelink尚未闭合整个合同；继续查动态外部符号解析边界。
当前仅源码及参考验证，未重建/安装产品，登记批仍红，不授修复完成。

后续复核纠正直接C归因：`src/main.c` R21-4a'明确让写出的镜像也转发宿主函数；
`exec/c/compiler.c`的fwdwant明确覆盖mode0但排除linking；`src/fwdstub.c`生成的缺符号分支具名exit127。
新参考实际执行direct.elf为rc127、stderr=`unisacc: no host function used`；合法abs(-7)的直接C镜像仍rc7。
因此direct-C不是同一静态链接缺陷；现登记批要求编译期拒绝与既有迟绑定合同冲突。
保留该断言及原红，未通过删断言或限制所有未知宿主名来造绿；需将两个合同明确区分后再登记验收。
长名探针250字符在前端按63字符上限拒绝，未到链接器，不能算长名链接负控通过。
相关文件在`out/c43-link-undef-fix-01/`；这不扩授产品或跨平台资格。

模型驱动验证：共享3a5b以-O2编译当前`exec/c/asmcompiler.c`成功，私有ELF
SHA `e2292a760d559dd9c733aca6a39f8bac896fdb04a5c448382f0f67218514ed9a`，
显式挂载原正规包ff696（未重造模型），未安装。薄包装在`out/c43-link-driver-subject-01/`，
不作为已装产品身份。原缺陷批`out/c43-link-driver-test-03/`实为4 PASS/1 FAIL：
对象、缺定义归档、逆序归档三拒绝均正确，正序实际rc106；唯一保留红为直接C合同冲突。
`out/c43-link-driver-archive-02.stdout`原归档互操作批17/0（独立/usr/bin/ar）。
测试夹具首次包装把--models放在ar前，造成运输失败；修复后第二次遗漏watchdog的bound.c，
导致执行控制失败；两轮原收据均保留。补全watchdog源码后才产生上述最终实跑结果，不把夹具失败误归产品。

接续正规构造：896888f424已git archive到仓外`/home/devali/repos/unisacc-epoch-link-896888f424`；
tar SHA af06942e0334e96e81757c5ad04a1fb4569fa1d7daf9a0ce78123b2b9574fe70。
sizeof纪元固定identity-helper实读新源SID为
`8390b60b0b8f40138c1da649a4cd55afa97edefc7a6eb0e8e88cb369d60f6549`（rc0）。
相对e5bade1851，src/exec/include/kernel/seed内仅src/tapelink.c变化；旧mixed包收据仍绑定353c，不能直接改标成8390。
私有模型驱动对象格式批`out/c43-link-driver-formats-01.stdout`21/0；其中打印的installed-product字样是工具固有标签，
本次实际对象始终是显式私有wrapper+e229驱动+ff696包，不是共享安装，也不是跨平台运行资格。

正规复用前置实测：新增薄入口`tools/nn-inputs-compare.cx`直接调用既有ni_compare，
对8390与353c冻结根、固定原生reader5c6a7756实际返回rc2
`necessary-data-tree-mismatch`，证据`out/c43-link-undef-fix-01/nn-inputs.stderr`。
同根负控亦rc2。原因明确：现有nninputs整树读取exec/unisa/src/weights，src/tapelink变化必然拒绝。
这只证明现有保守复用合同不适用，不证明模型需要重造；下一步核实构造器实际输入闭包，
在保留独立原包验证与新源身份的前提下建立适用证明。未删现有校验、未改旧包收据、未授准入。

复用拒绝现已具名定位：nninputs的等值检查新增首个不同规范树行的路径诊断，
不改变任何相等/拒绝条件。新实跑仍rc2，明确`path=src/tapelink.c`，见
`out/c43-link-undef-fix-01/nn-inputs-located.stderr`。五个宿主构造器的既有引用闭包
在nninputs的ni_deps/ni_quotes中列举，不含tapelink；这只是编译依赖边界，
还不能替代运行时数据输入闭包证明，故未据此放宽复用准入。

运行时读入口审查进展：seed/tbl.c仅以argv[1]读JSON，seed/net.c仅以argv[1]读表；
seed/pack.c读显式manifest、manifest指定模型和mount文件。source-package-pack.cx的正式pack命令
仅mount include、kernels、declaration/predefines，未mount src。gen.c的事实加载器固定前缀
exec/facts，通用stage清单固定exec前缀，但模板路径来自清单展开，不能只凭fopen文字搜索宣称闭包已证。
读入口列表保存在`out/c43-link-undef-fix-01/constructor-read-sites.txt`；正式gen命令与展开路径
仍需逐项绑定，下一步重点是这部分，不重复审已确认的tbl/net单输入路径。

生成器实际依赖观察已完成：从sizeof纪元34份gen command.argv.tsv取stage/flags，
保留原工具与40秒预算，仅把工作目录切到8390冻结根、输出写入新out目录；
外层strace -f trace=%file记录每次实际文件访问，34命令全rc0。
执行工具SHA5145075e08da56a5e4e25cf90d4eecf0366b322b49938b1cfd8889f04496818b。
34份JSON与353c原result.json逐份SHA相等，34 SAME/0 DIFF，未运行tbl/net重造权重。
`out/c43-link-gen-trace-01/COMPARE.tsv` SHA90953f766402415c697e8387c04dd01f396b6c1c57eb82e0839337f0572dd80e；
同目录保存每命令trace/stdout/stderr/JSON。trace未出现tapelink或直接src路径；
默认strace字符串截断使它不是完整规范路径清单，34输出逐字身份比较另有独立证据，不能混称通用依赖证明。
下一步将这份实际34同结果与原包来源链接入显式复用合同；当前ni_compare默认拒绝仍保留。

原生独立内容复验入口`tools/generator-json-compare.cx`已落地：枚举原gen目录、逐字比较新旧JSON、
绑定原命令文件SHA，输出规范witness；不从文件存在推断历史退出码或完整作业资格。
默认-run实读34份为different=0、rc0，witness9f1d7ec3076f65d812af21da912a192c5834caf46614070e06dc5386b1f118f4。
证据native-compare-02.stdout；首次纯CX SHA超55秒rc142保留，改用既有kernelsha后在原预算内完成。
独立JSON内容差异夹具rc1（out/c43-json-compare-negative-01），未放宽相等条件。

私有完整产品进展：冻结8390源用共享3a5b、显式冻结include/exec-c路径、-O2，
依次构造win/x86_64、lnx/arm64、osx/x86_64、osx/arm64、lnx/x86_64五切片，各40秒内rc0。
既有sizeof纪元host APE wrapper（seed/ape.c源同SHA ad3c21cf）挂载未变ff696包，封装rc0。
`out/c43-link-ape-01/unisacc.com` SHA4c2831c3fa1c871f5abf9549bcfd1f98f46f61554639706f698ed3ebd680e5b0。
真正APE上的原formats-elf-ape批68/0，out/c43-link-ape-formats-01；先前薄包装59/1保留，不冒充APE。
完整APE上的原链接缺陷批仍4/1，唯一失败direct-C迟绑定合同冲突，其余静态拒绝与正序rc106均通过。
这是未准入私有候选，不替换共享3a5b，不授外国宿主运行或完整发布资格。

有限源码适用证明入口`tools/link-source-delta.cx`已实跑：对两冻结根逐项核对
src/exec/unisa/include/weights/kernel/seed/iterate-kernel的完整目录名、类型/模式、普通文件字节、符号链接字面目标，
只接受src/tapelink.c固定b9af154f→16925f64这对哈希，其余差异拒绝。
实读1932文件/49目录/8符号链接，唯一允许差异1；witness
a064ffa55512277cc8f51e7f0739ad08ef808a206b39aee87969a80458b07d94，rc0。
交换根顺序实测rc2 unapproved-link-source-pair；证据source-delta*.stdout/stderr。
该证据不是原子文件系统快照或完整仓库身份，也不单独授准入；默认ni_compare仍保持原拒绝。

- 现象：`int used(void); int main(void){return used()+2;}` 经 `-c -b lnx/x86_64 -funit` 得 main.o 后，`unisacc -o img main.o`（或加只定义 `unused` 的归档、或直接 `-o img main.c`）rc 0 并写出镜像；运行 SIGSEGV（rc 139）。预期：`link: undefined reference to used`，rc≠0，无镜像。
- 定位：src/tapelink.c:351 `tl_check_undef`（R20-3）只检查名字以 `g_` 开头的未定义外部（全局数据对象），函数外部不检查。
- 同现：经典参考 REF 0ed46192 行为相同（共享缺陷，非回归）；产品==REF 字节差分仍有效，不阻塞候选 f3705dfa。
- 复现入口即批次：`tests/batches.tsv` 行 `link-undefined-function`（tools/link-undefined-function.cx），当前红 0/3，预期不放宽；日志 out/c43-cc-reg-02/link-undefined-function.log；另有 out/c43-cc-link-undef-repro-01/REPRO.md。
- 拟议修复（由 src 域所有者 cdx/grk 在本纪元合入；cc 不动 src/）：`tl_check_undef` 去掉 `s[0]==103 && s[1]==95` 前缀条件，对所有 `tl_ext` 中未在 `tl_def` 出现的名字（除链接器自身生成的 `__init`）发 `link: undefined reference to NAME`。需同步 REF（tools/build_ref.sh 源）以保持差分；下一慢车道纪元重建后批次转绿。
- 裁定：fable 2026-10-28（选项 B+C 修复部分）：拒绝条款不属 archive.product-ar 格合同，归独立批次。

### 拟议补丁（2026-10-28 17:4x，cc 起草，未应用；src/tapelink.c 由 src 域所有者合入）

事实核对（冻结导出 f3705dfa，`-c -b lnx/x86_64 -funit` 单元 tape 的链接属性）：函数外部记为裸名 `.extern used`；全局数据记为 `.extern g_shared`；宿主库调用（printf/malloc/free）**不产生** `.extern`；链接器自身提供的 `__init` 以 `.extern __init` 出现，须白名单。

```
--- a/src/tapelink.c
+++ b/src/tapelink.c
@@ int tl_check_undef(void) {
-        if (n > 2 && n < 250 && s[0] == 103 && s[1] == 95 && !tl_inset(tl_def_at, tl_def_len, tl_ndef, s, n)) {
+        if (n > 0 && n < 250 && !tl_is(s, n, "__init") && !tl_inset(tl_def_at, tl_def_len, tl_ndef, s, n)) {
             pre = "link: undefined reference to "; k = 0;
             while (pre[k]) { msg[k] = pre[k]; k = k + 1; }
-            n = n - 2; s = s + 2; while (n > 0) { msg[k] = *s; k = k + 1; s = s + 1; n = n - 1; }
+            if (n > 2 && s[0] == 103 && s[1] == 95) { n = n - 2; s = s + 2; }   /* print data objects without the g_ tag */
+            while (n > 0) { msg[k] = *s; k = k + 1; s = s + 1; n = n - 1; }
```

验收：批 `link-undefined-function` 三行转绿（`link: undefined reference to used`，rc≠0，无镜像）；`objects-link`、`formats-objects`、`formats-archive-ar`（17）、`formats-elf-ape`（68）不变；REF 需同步同一补丁后 `optimization-corpus-*` 六目标字节差分保持 0 DIFF。风险：若某单元对 `_start` 或 `__init_u` 发 `.extern`（本次夹具未见），需把它们加入白名单。

## 观察（非缺陷）：相同字符串字面量的指针相等性（2026-10-28 ccdiff-4 探针）

- `const char *s1="dup",*s2="dup"; s1==s2`：宿主 gcc 11 输出 1（字面量合并），产品 f3705dfa 输出 0（不合并）。C99 6.4.5p6 明示"相同内容的字面量是否为同一对象未指定"，两者都合规；不入差分语料，不改产品。

## 观察（诊断质量，低）：无 main 的链接拒绝文本不具名（2026-10-28 link-edge-cases）

- `unisacc -o p a.o`（a.o 无 main）：rc 1、无镜像（合同满足），但 stderr 为 `reject: not covered: address or target declaration`，未说明"undefined reference to main"。建议 src 域所有者在 tl_check_undef 补丁同批加 `_start`/`main` 缺失的具名诊断。批 `link-edge-cases` 以 INFO 行记录现文本，不断言。

### 同根第二现象：跨归档顺序（2026-10-28 17:5x，cc 探针）

- `m2.o A.a B.a`（A 含 outer，B 含 inner，outer 调 inner）rc 0 且退出 106；`m2.o B.a A.a` 也 rc 0，镜像运行 SIGSEGV。原因：exec/c/compiler.c:401 / src/tapelink.c:446 归档按给定顺序各自扫到不动点，不回扫前面的归档（与 GNU ld 单遍语义相同——ld 在此情形报 `undefined reference to inner`），而未定义函数又不被 tl_check_undef 拦截，于是静默链接。上述补丁合入后此例转为具名拒绝（与 ld 一致）；是否改为跨归档回扫属产品合同选择，未要求。
- 复现入口：批 `link-undefined-function` 行 `cross-archive-order.reversed-refused`（工作顺序行 `working-order` 断言 106，保证非空洞）。

## driver.diagnostics — 驱动与 ar 的四处不具名/错因拒绝（开放，诊断类；exec/c/compiler.c，所有者 cdx/grk）

批 `driver-diagnostics`（tools/driver-diagnostics.cx，7/11，红行即复现）：
1. `unisacc -run u.o`（单元对象）：rc 1，**stderr 为空**。预期具名拒绝（如 `cannot run a unit object; link it first`）。
2. `unisacc -run ok.a`（仅归档）：文本 `these objects were compiled for another target; running needs this machine's: `（目标名为空，且原因错误：问题是没有可运行的单元/main）。
3. `unisacc -o p ok.a`（仅归档链接）：文本 `model driver: tape domain: required tapecheck model is missing`（错因）。预期与无 main 一致的具名拒绝。
4. `unisacc ar zz empty.a`（空归档上的非法操作）：rc 0 无输出；product_ar 只在遍历成员时校验 op（compiler.c:445），空归档不触发。预期 `ar: operation must be r, q, t or x` rc 2（非空归档已如此）。
附：`ar q2`/`rcsX` 等后缀不校验（op[0] 判定），低优先。

## pp.negative — 预处理器两处未拒绝的约束违反（开放；E2/E1 模型域，所有者 cdx/grk）

批 `pp-negative`（tools/pp-negative.cx，8/10，红行即复现；宿主 gcc 11 对两者均 rc 1）：
1. `#define 3x 1`：产品 `-E` rc 0、无 stderr。C99 6.10.3："The identifier immediately following the define…"——宏名须为标识符，须诊断。
2. `#include <stdio.h`（缺 `>`）：产品 rc 0 且**静默包含了 stdio.h**（stdout 56860 字节），无 stderr。C99 6.4.7 header-name 必须闭合；须拒绝。
诊断质量（INFO，不断言）：`unterminated conditional directive`、`argument count`、`#if expression`、`unterminated macro call` 四类拒绝文本无 `file:line`，而 `#error`、`#else without #if`、`no such file for #include` 有。
非缺陷：`#foo bar` 为 6.10 非指令（产品接受合规，gcc 选择报错）；`#define __STDC__ 2` 产品与 gcc 都接受（gcc 仅警告）。

## c.negative — 七处 C99 约束违反被静默接受并产出对象（开放；E3/parse2 模型域，所有者 cdx/grk）

批 `c-negative`（tools/c-negative.cx，10/17，红行即复现；宿主 gcc 11 `-std=c99 -pedantic-errors` 对七者均 rc 1；产品 rc 0、无 stderr、写出 .o）：
1. `const int k=1; void f(void){ k=2; }` — 6.5.16p2 可修改左值。
2. `int j(void){ case 1: return 0; }` — 6.8.1p2 case 须在 switch 内。
3. `struct S{int a;}; struct S{int b;};` — 6.7.2.3p1 同作用域重定义。
4. `int v(void){ int a[3]; return a; }` — 6.8.6.4p3/6.5.16.1p1 指针→int。
5. `int *p=0; char *c=p;` — 6.7.8p11/6.5.16.1p1 不兼容指针初始化。
6. `typedef int T; typedef char T;` — 6.7p3（C99 禁止 typedef 重定义）。
7. `void vv(int a, int a){}` — 6.9.1p9/6.7p3 重复形参。
前四项影响语义（接受错误程序），后三项为类型系统漏检。十项已正确拒绝的里有 5 项诊断带 file:line（INFO）。

### c.negative 第二轮（2026-10-28 18:0x）：再添八处静默接受（批现 19/34）

8. `int *p = 5;` — 6.7.8p11 int→指针初始化。
9. `int k2(int); k2(1,2)` — 6.5.2.2p2 原型存在时实参过多。
10. `int n2(int); char n2(int);` — 6.7p4 函数类型冲突。
11. `enum E2{A2,B2}; enum F2{A2};` — 6.7p3 枚举常量重声明。
12. `int t2[0];` — 6.7.5.2p1 数组大小须大于零（gcc 扩展接受，`-pedantic-errors` 拒绝）。
13. `x = (void)0;` — 6.5.16.1p1 void 值被使用。
14. `const char *c="x"; c[0]='y';` — 6.5.16p2 经 const 指针写入。
15. `int i; i += "str";` — 6.5.16.2p1。
诊断质量：`int s2[-1]` 被拒但文本为 `reject: not covered: tape data directive`（错因，INFO）。

### c.negative 第三轮（2026-10-28 18:0x）：再添六处（批现 20/41，漏检累计 21）

16. `char *p; int *q; p == q` — 6.5.9p2 不兼容指针比较。
17. `int x = 1; int x = 2;`（同一块作用域）— 6.7p3。
18. `case 1: case 1:` — 6.8.4.2p3 重复 case。
19. `a[1.5]` — 6.5.2.1p1 下标须为整型。
20. `int a19(void){ return a18(); }`（a18 返回 void）— 6.8.6.4p1。
21. `const int c; int *p = &c;` — 6.7.8p11 丢弃限定符。

### c.negative 第四轮（2026-10-28 18:0x，声明层）：声明层覆盖良好，再添两处（批现 20/43，漏检累计 23）

22. `void b15(void x){ }` — 6.7.5.3p10。
23. `static extern int b16;` — 6.7.1p2 多个存储类。
位域宽度/类型、VLA 存储、register 取址、函数数组/返回数组、`long long long`、`unsigned float`、不完整类型、union 多初始化器等 18 项均已正确拒绝。

## 观察（有效程序被拒，低）：`int vla_fs[(int)1.0 + 0];`

- 文件作用域数组大小为整型常量表达式（C99 6.6p6 允许把浮点常量强制转换为整型），gcc 接受；产品拒绝（`-c -b`，rc 1）。属 E3 常量折叠对强制转换的覆盖缺口；非安全问题。

## link.malformed-input — 畸形输入：两处容器不校验、一处产品自身崩溃（开放；exec/c/compiler.c tl_tape 定位 / 运行器）

批 `link-malformed-input`（tools/link-malformed-input.cx，8/11，红行即复现）：
1. **S2 产品崩溃**：`_start:\n  imm r0, 7\n`（无 ret，落出末尾）`-run noret.tape` → 产品进程 SIGSEGV（rc -11，无 stderr）。预期具名拒绝（"control falls off the end of _start"）或运行期有界终止。`-o` 镜像路线 rc 0 并写出镜像（INFO）。
2. S1 容器不校验：ELF 魔数被覆盖为 `XXXX` 的单元对象仍被链接成镜像（rc 0）；截断到前 2000 字节（无节头表）的对象同样被链接。tl_tape 直接搜索单元 tape 负载而不校验 ELF 头/节表。预期：`not an ELF unit object` / `truncated object` 具名拒绝。
绿：截到 100 字节对象、tapebin 翻字节/截断/坏 minor、未知操作码 tape、二进制垃圾 tape 均具名拒绝。
