# c43 已知缺陷登记（cc，2026-10-28 起）

## link.undef-function-refusal — 未定义外部函数链接成崩溃镜像（开放）

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
