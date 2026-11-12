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

## 实测（2026-10-30，候选，未装）

- 提交：`b2f2e45ef6`（cxmod 报错）、`f781d1ca4d`（调用点改成直接 use）、`376c2af9fa`（`src/drive.cx` 对 `sy_env_vec` 直接 `use sys.env`，去掉函数里的局部原型）。
- 候选 `out/grk-iproto/candidate`，3025940 字节，sha256 `4ac3c2d4fea590600e84b4001799eb7041baaf96ca2f9352a2e78a6225135ddd`。P3 仍是 `out/grk-entry-build-14/pkg.bin`。没有安装。已装产品仍是 79fb84e6。
- `out/indirect-proto/main.cx`：rc=1。标准错误两行，点名 `user.cx`、`tx_snprintf`（text.fmt）和 `tx_strtod`（text.num）。没有改 `out/indirect-proto/`。
- 直接 use 的对照在 `out/grk-iproto-direct/`：rc=0，标准输出 `[ab-42] n=5 num=2.250000`。
- runcache：`out/grk-iproto-rc5`，12/0，冷 536 ms，命中 40 ms。
- 报错规则：调用方单元里没有声明、另一个单元又定义了这个函数，就报错；同一文件里的调用都报出来。一个单元最多 48 个 use。没有任何单元定义的名字仍留给后面的 undefined function。本版不把原型顺着 use 传出去。
- `symindex undeclared --tracked` 的 reach=transitive：archive 15 处不改；`tools/symindex/fixture/proto/user.cx` 4 处是故意的夹具，`-cx-plan` 仍报这 4 个函数；`tests/cxclash/main.cx` 的 `m_n` 先报两个 `m` 都导出。`src/drive.cx` 已不在这张表里，`-cx-plan` rc=0。补过 use 的那 20 个文件 `-cx-plan` 都是 rc=0，含 `apps/csih/tui.cx`（40 个 use）。
