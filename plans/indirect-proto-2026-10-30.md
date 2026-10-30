# 经中间模块间接可见的函数没有原型，可变参数和 double 返回被调错（2026-10-30）

## 事实

- **复现**：out/indirect-proto/。csih 报告，cc 用已装产品 79fb84e6 和候选 out/grk-pe-cand/candidate 各复测一次，结果相同。
  - mid.cx use text.fmt 和 text.num；user.cx 只 use mid，却调用 tx_snprintf（可变参数）和 tx_strtod（返回 double）；main.cx use user。
  - 结果：两个产品都 Segmentation fault，rc=139。
  - 对照：user.cx 改为直接 use text.fmt、text.num 后，输出 `[ab-42] n=5 num=2.250000`，rc=0。
- **按 csih 的观察**：返回 long 或指针的函数，经间接可见调用时结果正常，所以这个问题平时不显形。
- **推断**：被调函数在链接集合里，但调用方单元里没有它的声明，前端按隐式声明处理（返回 int、不带 `...`），因此可变参数调用时 %al 没设，double 返回值没从 xmm0 取。

## 应有行为（编译器车道）

- 调用方单元里没有可见声明的调用必须报错，例如 `call to undeclared function tx_snprintf (declared in text.fmt, which user.cx does not use)`，不能静默按隐式声明去调。
- 另一种做法是给传递可见的函数补上完整原型。这等于把模块接口改成可传递导出，属于设计改动，本版先报错。
- 验收：
  - out/indirect-proto/main.cx 编译期报错，并点名 user.cx 和 tx_snprintf；
  - 直接 use 的写法仍然 rc=0；
  - runcache 12/0 不回退；
  - 用 `tools/refactor.cx sites` 加 `tools/affected.cx` 找出仓内依赖隐式声明的调用点，逐个补上 use。

## 过渡合同

模块里调用哪个 L0 函数，就直接 use 它所在的模块，不依赖经中间模块"顺带可见"。csih 已按这个办法处理。
