# D2：当前产品拒绝点抽样核验

本表是具名最小构造的实际核验，不是对全部拒绝字符串的穷尽清单。语法错误、容量保护和输入损坏不自动算语言缺口。产品经 sh 启动 APE；参考为同树私有 build_ref；均用 -S。系统 cc 使用 -std=c99 -fsyntax-only，所以 extern 的接受不证明链接成功。

| 构造 | 产品拒绝位置/文本 | 参考接受 | 成本与处理 |
|---|---|---|---|
| bighex | /tmp/cdx-d2/bighex.c:6:109: error: this is not the start of an expression /       printf("%d %d %d %d\n", p <= 0xAB4624F099C83E0CLL, p <= 0xAB4624F099C83E0C, 0xFFFFFFFFFFFFFFFF > 0, p < 01777777777777777777777); | 是 | 待定位：当前拒绝指向八进制部分，不能笼统称十六进制缺口 |
| complex | /tmp/cdx-d2/complex.c:1:32: error: expected ';' /   int main(void){double _Complex z;return 0;} | 否 | 高：类型、运算、ABI 全链路；两侧共同缺口 |
| extern_array | 接受 | 否 | 不列拒绝：当前 -S 已接受；链接定义缺失另属链接契约 |
| sigaction | /tmp/cdx-d2/sigaction.c:2:33: error: not covered: incomplete struct /   int main(void){struct sigaction sa; return sizeof(sa)==0;} | 否 | 中高：三平台结构布局、sigset_t 与宿主函数，不能照搬单平台布局 |
| struct_ret | UNCOVERED e3 - - /tmp/cdx-d2/struct_ret.c:1:101 expr.call.ret-struct struct return expression outside local lvalue / /tmp/cdx-d2/struct_ret.c:1:101: error: not covered: struct return expression outside local lvalue | 是 | 高：E3 结构返回表达式与值栈语境；需 δ |
| tdarray_extra | /tmp/cdx-d2/tdarray_extra.c:1:22: error: not covered: array typedef with extra suffix /   typedef int A[3]; A a[2];int main(void){return sizeof(a)!=24;} | 否 | 中：声明符形状组合；需 δ |
| ttyname | unisacc: error: undefined function 'ttyname' / 1 error generated. | 否 | 低：POSIX 原型+宿主转发验证；需 include/kernel，非仅 src |
| vla_inner | /tmp/cdx-d2/vla_inner.c:1:33: error: not covered: nonconstant bound /   int main(void){int n=2;int a[2][n];return sizeof(a)==0;} | 否 | 高：运行期行步长/sizeof；两侧共同缺口 |

## 证据及边界

原始三方退出码与诊断：/tmp/cdx-d2-evidence.json；最小输入：/tmp/cdx-d2/。diag.com.knownfail 当前无有效条目，product-refusals.knownfail 仅 a_bighex。清单不能替代阶段拒绝点覆盖。ttyname 和 sigaction 的已知缺口得到确认；struct_ret 是产品独有覆盖缺口。

本轮按 cc 补充约束不改 exec/、facts、清单和 exporter。上述低成本用户项 ttyname 需要头文件及生成物；只改 src 不会让产品取得该原型，故未冒充为两侧修复。建议 T1c 结束后先做 ttyname，再评估数组 typedef 附加维度；sigaction 需单独跨平台 ABI 设计。没有产品图变更。
