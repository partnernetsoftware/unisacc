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

## sizeof 误编根因（2026-10-22 15:5x AWST；定位实际跑过，补丁只是设计）

产品 `-run` 实测（32dd5156 根部 `unisacc.com`，对 gcc）：
`sizeof(*(ip))`=12 ✓，`sizeof(**(ip+1))`=4 ✓，`sizeof(ip+1)`=8 ✓，`(ip+1)[1][2]`、`(*(ip+1))[1]` 取值 ✓；
`sizeof *(ip+1)`、`sizeof((ip+1)[0])`、`sizeof *(q+0)`=8（应 12），`char(*)[5]` 得 8（应 5），
`int(*)[3][2]` 得 8（应 24），`sizeof *(a+1)`（`int a[4][3]`）得 8（应 12）。

- 求值侧没丢形状：`operator-pointer-add/sub`（`exec/parse2/control-result.tsv:833-852`）经 `lshapeok/lshape/lrank`
  把 `rkok/vid/rk` 还原；`UD.dn`（`shape-result.tsv:176`）在 `rkok=1, rk>1` 走 `UD.row`（`rk-1`，不加载、`vt` 不变）。
- 丢在 sizeof 总路径：`SZ.exprsize`（`control-result.tsv:2085`）只用 `ELSZ(td=vt, tb=vb)`，不看 `rkok/rk/vid`。
  行结果的 `vt` 仍是行指针的深度 → 指针宽 8。`sizeof(*ip)` 走 `SZ.star → SZ.shapecheck` 才对。
- 形状约定（由 `SZ.ar`、`SH.WIDTH` 的读法推出）：`int(*)[3]` 的形状 `ARR=2`、`DIM[1]=3`、`vt=1`。

候选补丁（只是设计，未建未测）：`SZ.exprsize` 前加一格——`rkok=1` 且 `1 ≤ rk < ARR[vid]` 且确为行
（经 `UD.row`/`PX` 得来）时，`sz = ELSZ(vt-1, vb) × Π DIM[vid*8+i]，i ∈ [ARR-rk, ARR)`。
陷阱：`UD.scalar` 不清 `rkok`，`**(ip+1)` 之后仍 `rkok=1, rk=1`，仅凭 `rk<ARR` 会把标量当行；需要
`UD.row` 置、`UD.scalar` 与各运算 `RET` 清的行标记，或在 `UD.scalar` 清 `rkok`（后者改求值面，须全差分）。
落地须重建产品并做 c989780c 同款 1480 行差分；当前宿主可用内存约 2G，pack-prep 会卡，故本刀停在定位。

## sizeof 行修复与 (**p)[N] 拒绝（2026-10-22 21:xx HKT，实际跑过；新宿主 Linux x86_64 16 核）

**根因修正**：仅凭 `rkok/rk/vid/vt` 无法区分「行」与「退化后的指针」——`*(ip+1)` 与 `*(ip+1)+1` 寄存器完全相同（指针加法原样拷回 rkok/rk）。所以设计稿的 `rk<ARR` 判据不可行，改为显式行标记：

- `UD.row`（`*P`）与 `PX` 行路径（`P[i]` 仍是行）置 `szrow=1`，并记 `OLEN→szrowlen`、`vt→szrowt`。
- `sizeof EXPR`：进入时清 `szrow`，操作数结束记 `OLEN→szend`；仅当 `szrow=1 ∧ szend=szrowlen ∧ vt=szrowt`（行之后没发射任何代码、类型未变）才按行算：`ELSZ(vt−1,vb) × Π DIM[vid*8+i], i∈[ARR−rk, ARR)`，与 `SZ.ar` 同式。
- 不发码也不改 vt 的两种形状显式清标记：强制转换（`CP` 操作数返回处）与逗号表达式（`CEXPR` 见过逗号时在 `CX.e` 清；`szcomma` 随 `recipe_retseqs_stack0/1` 入栈，嵌套括号不串）。注意 `exec/parse2/return-stack.tsv` 是死文件，真源是 `gen2-actions-manifest.tsv` 的 recipe。
- 实测（Python E3 模拟 + 产品）：16 行 50 余个 sizeof 形状与 gcc 全同（含 `int(*)[3][2]`、`int(*)[3][4]`、struct/double/char 行、`(P+n)[0]`、`*(A+n)`、`&*row`、`!row`、`(int*)row`、`(0,row)`、`?:`、嵌套 sizeof）。
- 差分：HEAD 干净导出构建（34448c83…）对候选（664be410…），647 个 `tests/`、`examples/` C 文件 `-S` tape+rc+stderr：**646 同、1 异**，唯一差异即 `tests/c/candidates/array-pointer-arith.c` 的 8→12；该探针基线 rc10、候选 `pass` rc0。

**新缺陷 J（产品静默误编，已拒绝）**：`int (**ipp)[3]` 的声明符在 `FPD.moreptr` 直接跳过多余的 `*`，形状只记一级指针（实测 vt=1，与 `int (*)[3]` 同），于是 `*ipp` 被当成行不加载：v0.0.42 产品上 `r = *ipp` 得到 `&ip`（`r - a` 输出 4，gcc 0）。修法按「接受即须同参考，否则具名拒绝」：`FPSTART` 清 `fp_more`，`FPD.moreptr` 置 1，`)` 之后的 `[`（`PD.tail.test→PD.array`）见 `fp_more=1` 即 `not covered: pointer to pointer to array`。`PD.tail` 只从 `FPD.c` 进入，`fp_more` 不会陈旧。函数指针形状不动：`void (**out)(…)` 形参探针（a_nested_fp_param）tape 逐字节不变。

**仍未修（登记）**：
- 局部 `int (**fpp)(int) = &fp; (*fpp)(21)`：v0.0.42 产品与经典参考都编译通过、运行段错误（同一 `FPD.moreptr` 丢级）；形参形状碰巧正确，故未一刀拒绝，待单独定界。
- 经典参考在多维指针到数组上静默错：`int (*mp)[3][2]` 的 `sizeof mp` 得 16、`*mp` 得 8；`int (**ipp)[3]` 上 `*ipp` 得 24、`(*ipp)[1][2]=7` 程序段错误；`&*(ip+1)` 得 12。探针 `tests/c/candidates/sizeof-rows-classic-gaps.c`（cc、Python 参考、候选同；经典异），修好经典后再移入 `tests/c/`。
