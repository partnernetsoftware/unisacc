# 数组指针成员写入：经典自举误编案

实际跑过；语言缺陷未修。驱动规避提交 ecdbd98d 不关闭本案。
待审独立探针：`tests/c/candidates/array-pointer-member-store.c`，未新增套件或登记进 gatedeps；放在 candidates 子目录以免现有 tests/c/*.c 通配套件自动执行。

源码完整见上述文件（SHA256 `922a4a76811394b44d61db23ea99ba97d6dcdc0da4bbeec60253ac31f866c999`）。
要点是 `struct Slots { unsigned char (*dl)[8]; };`，以真实四行数组初始化成员，
循环 `store(c.dl[j], j+1)`，再检查四行首字节。合法控制应输出
`array member writes: pass\n`、退出 0。

## 实测分账

同一源码，Linux x86_64，`-O2`；每次编译/执行各限 15/10 秒。

|编译路径|操作|退出码|stdout / stderr|
|---|---|---:|---|
|系统 cc|生成镜像|0|均空|
|系统 cc|执行镜像|0|stdout 为上述 pass；stderr 空|
|经典参考/自举 ua|`-run`|−11（SIGSEGV，shell 139）|均空|
|经典参考/自举 ua|`-o` 编译|0|均空|
|经典参考/自举 ua|执行其镜像|−11|均空|
|模型产品 fb68a994|`-run`|1|stdout 空；具名欠覆盖|
|模型产品 fb68a994|`-o`|1|stdout 空；具名欠覆盖，不产生镜像|

经典工具为 fresh 构建目录的 `ua`，摘要
`bb9126656a4dba266874a496661368795d198c55e9bdc915a00dfac50573d934`。
模型产品摘要
`fb68a994f1e6e10b654591528ba3bae9771918be4d46a10192ef6f5f7158ff7d`。
仓外原始记录：`/tmp/c43-memory-fix/minimal-results.json`；该路径仅定位观测日志，不是合同身份。
模型诊断为 `not covered: array typedef pointer member`，源码行 2。
模型路径的具名拒绝不能计为该 C 程序产品绿，也不能计为模型接受后的误编。

## 涉及阶段：仅定位线索，未修未裁

经典前端 `src/front_parse.c:4213` 经 `fpdecl()` 读到 `fpadim`，
但成员的 `mdim2` 初始化和 `src/front_parse.c:4337` 的成员维度存储值得核对。
`src/front_parse.c:1605` 的成员下标状态依赖 `mbdim2`；
疑点是“指针指向一行数组”的维度/衰变/步长身份未完整传递，进而把行指针当标量加载。
这是待追踪假说，不声称已确定具体错误指令，也不先改 lower/enc。
模型前置拒绝见 `exec/parse2/shape-reject.tsv:4`。

驱动仓外插桩实际停在绑定后的 `resource_u64(c->dl[j], ...)`；
同源码同模型包，系统 cc 驱动 return7 正常，自举驱动崩溃。
把适配器成员改成 `unsigned char *dl`、以 `dl+8*j` 明确字节偏移后，
自举驱动的 `-run` 与默认运行 return7 均恢复。这只是规避见证。
后续语言修复应先按上述探针重现实测，再补成员数组指针读取、写入、下标/跨行步长的合同，
由主人另钉文件域；本批未修改经典前端或核心模型。

## 复核（2026-10-22 14:0x AWST，实际跑过）

- `array-pointer-member-store.c`：经典参考 UA（HEAD 同源 build_ref）、根部产品 `unisacc.com`（32dd5156）、Python 参考三者 `-run` 均输出 `array member writes: pass`、退出 0——本案原缺陷在当前树上**已不复现**（修于何提交未追）。
- **新缺陷（产品静默误编，接受即错）**：同目录 `array-pointer-arith.c` 在产品 `-run`/`-o` 下退出 **10**（cc、经典参考、Python 参考均 `pass`）。定位：`int (*ip)[3]` 下 `sizeof *(PTR±n)` 产品得 8（指针宽），应为 12；`sizeof(*(ip+2))`、`sizeof *(ip+2)`、`sizeof(*(2+ip))`、`sizeof(*(ip-0))`、`sizeof(*(ints+1))` 均错，`sizeof(*ip)`、`sizeof(ip[2])`、`sizeof(ints[1])`、`sizeof(*ints)` 正确；经 `*(ip+2)` 的读写本身正确。即产品 sizeof 对「指针算术结果再解引用」丢了数组指向类型。`unsigned char (*)[8]` 恰因 8==指针宽而被掩盖。修向：parse2 sizeof 段表刀（另派，验证中）；修前该探针仍不入闸。
