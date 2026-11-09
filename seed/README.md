# C99 seed constructor experiments

`net.c` is a standalone C99 implementation of the current flat-table to
threshold-network step. Since 0.0.25 (self-hosting road B1) the gates build it with the previous release,
`unisacc.com`, not the system cc; any C99 compiler still works:

```sh
sh ./unisacc.com seed/net.c -o seed-net
./seed-net stage.tbl stage.net
```

The input is the `T/S/Q/R` table produced by `exec/c/tbl.py`; the output is
the `N/S/Q/H` network currently produced by `exec/c/net.py`. It reuses the
runtime's declared action arities in `exec/c/core.h`. It is not yet wired into
`buildcompiler.sh`, and generating the table, P3 package and APE seed still
requires Python. `tests/seedconstructcheck.py` checks the boundary probes;
`tests/seedconstructmatrix.py` independently regenerates and byte-compares the
30 networks used by the current compiler package (seven shared, seven auxiliary,
twelve target, four Linux object). The nine bounded gate jobs split those
checks by model family and target. This establishes `.tbl -> .net` parity only;
package and seed construction remain separate work.

The source identity (`seed/ident.c`) hashes complete source files, including
`src/drive.cx`, `src/version.h`, and `exec/facts/limits.tsv`. Release versions
are metadata in the single data row of `release/candidate.tsv`, outside that
closure. Compilation emits the fixed 32-byte version slot; packaging replaces
it without rebuilding NN/context/kernel/whole materials. Changing the release
version must leave the source SID unchanged; changing any slot declaration or
other source semantics must change it. Packaging and platform-format checks
remain separate from source identity and do not transfer native qualification.

## Source package identity migration

现行 C 身份权威为 `seed/ident.c`。默认 `[ROOT]` 仍只打印一个小写
64hex SID 和换行，但算法已经切换，**不保留旧 SID**。历史投影仅通过
`ROOT legacy-projection`（摘要）与 `ROOT list-legacy`（旧文件表）显式访问；
归档 Python 实现不作为现行资格。

组合 SID 是以下规范表的 SHA256（全部为 ASCII/LF，无空行）：首行
`source-packages-v2\n`，随后固定 12 行 `包名\t64hex\n`，顺序为
`cx/sys,cx/mem,cx/text,cx/core,cx/driver,cx/runtime,cx/compiler,src,exec,seed,weights,include`。
`ROOT packages` 原样输出此表，不夹说明。

每包预映像为 `package-files-v1\n`、按真实 repo 相对路径字节序排序的
`物理路径+NUL+64hex文件SHA256+LF`、`package-aliases-v1\n`，最后为按
逻辑路径再真身路径字节序排序的 `逻辑路径+NUL+真身路径+NUL` 拓扑事实。
不将目标内容摘要放入别名事实：改目标内容只影响其物理 owner 包。
新增别名可影响声明别名的包，但不重复计入内容。
拓扑仅记录叶条目自身确为 symlink 的声明；声明路径用父目录真身加原 basename 规范化。
目录别名下的普通子文件不生成隐式逐文件 alias，例如 `src/sysx -> ext/sysx`
保持不变时，增改 `ext/sysx` 真身文件只影响 `cx/sys`，不额外改变 `src`。
语义后缀过滤仍按逻辑输入路径：`src/a.cx -> ext/sysx/target.dat` 会纳入该真身内容，
不因 target 的 `.dat` 后缀漏哈希，内容 owner 仍依真实路径映射为 `cx/sys`。

`source-packages-v2` 对 `seed/` 和 `include/` 整体保守纳入：未知后缀、
无后缀、私有头、源码分片和有序清单都按原始字节哈希。只排除明确的
`README.md`（说明文档）和 `.DS_Store`（Finder 元数据）两个 basename；
不凭 generated/cache/out 目录名或产物形后缀推断非源码，宁可过度失效。
保守两根先遍历，避免其它根的窄筛选目录 alias 抢先标记已访问导致漏输入。
其它根保留旧 keep 扩展、验证器过滤，并覆盖 `exec/c/asm/*.S`。
包内 `package-files-v1` / `package-aliases-v1` 编码不变；v2 是选择规则及
组合表的新版本。`changes` 只接受规范完整 v2 表，明确拒绝 v1，不能补字段或回退。
清单不经排序/解析/组装：同一实际文件集合的两行换序必须改变 seed 包及 SID，
其它包不变。这里只证明身份，不构造 H profile/assembler；逐边头闭包延期 0.0.50 G1。
目录和文件 symlink 均跟随，以 ROOT 内 realpath 规范相对路径全局去重，
不以 inode 作为身份。自然 12 根优先决定物理归属；cx 其余覆盖源码
（含 cx0/workspace）归 core support，kernel 生成物归 seed support，
`iterate/kernel/typekw.tsv` 归 exec support。旧 13 个 tools 支持输入全字节
保留，含无扩展的 bound/cxbin 以及 ape-pack/release-ape/releaseversion。
支持文件仍哈希真实 repo 路径，不虚构重定位源码。
上述 13 个 tools 输入及 `iterate/kernel/typekw.tsv` 是 mandatory 支持，
每项必须为可读常规文件；ROOT 内指向常规文件的 symlink 仍按真身与别名规则处理。
目录（包括指向目录的 symlink）、FIFO 和其他非常规文件一律拒绝，不递归收集或静默跳过；
类型错误退出码为 1，stderr 为 `ident: mandatory support is not a regular file: <逻辑路径>`。
无读权限或实际读取失败仍沿用既有错误诊断。
`ext/memx,textx,sysx` 分别归 mem/text/sys；`ext/crypto`、`ext/flownet`
及 `flownet` 归 core；其余从真实包 symlink 可达且不在自然根的源码
归 seed reachable-support。`package-inputs` 可机查上述固定归属。

目录祖先回链若能成功 realpath，记录拓扑后由全局目录已访问集终止；
realpath 自身的 ELOOP 则报错退出。悬空链接、逃出 ROOT、EACCES、
ENOTDIR、读目录或文件失败都明确拒绝，不能静默跳过。

接口（TAB 分列，无表头，除 packages 的规范首行）：

- `ROOT list-current`：物理路径、原文件 SHA256，固定包顺序及包内路径排序，仅真身一次。
- `ROOT list-aliases`：声明 owner 包、逻辑别名路径、规范真身路径。
- `ROOT package-inputs`：owner 包、`file` 或 `support`、规范物理路径、文件 SHA256；
  `file` 表示在 owner 自然根中，其他路径为 `support`。
- `ROOT changes OLD_TABLE`：严格要求旧表完整规范（12 行、小写 hex、固定顺序、无尾随字节）；
  每个变包一行 `包名\t旧摘要\t新摘要`，最后两行 `before\t旧组合SID`、
  `after\t新组合SID`。OLD_TABLE 路径相对调用目录，与 artifact 参数一致。
- `write/check/manifest` 的记录协议不变，source 字段与 stage key 使用新 SID。

L1–L3 与父任务 55s 验收命令（此处未执行；仅编译身份工具，不运行构造）：

```sh
timeout 55s sh -c '
  cc -O2 seed/ident.c -o out/ident-after &&
  out/ident-after "$PWD" &&
  out/ident-after "$PWD" packages > out/source-packages.tsv &&
  out/ident-after "$PWD" changes out/source-packages.tsv &&
  out/ident-after "$PWD" list-current > out/source-inputs.tsv &&
  out/ident-after "$PWD" package-inputs > out/package-inputs.tsv &&
  out/ident-after "$PWD" list-aliases > out/source-aliases.tsv
'
```

L1 核对规范表及组合 SHA256，L2 在隔离副本按 owner 修改源码核对变包，
L3 在隔离副本检查别名去重、目标修改不扰动 alias 包、循环/逃逸拒绝。
独立合同测试由测试任务维护和执行；不修改冻结 L0 或真实生成 kernel。
