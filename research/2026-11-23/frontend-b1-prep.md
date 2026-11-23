# 0.0.99 前端 B1 预备核查与组合补丁验证(3775a03c71 + 7.2 不完整数组 + t10)

2026-11-23,agy-flow。版本分支 `v0.0.99`,基于 `main` 头 `d21f46dccf`。组合补丁已存于同目录 `research/2026-11-23/v099-b1-combined.patch`。

## 1. 三项状态机核查与原草案修正

### 1.1 3775a03c71(G-redecl 全局对象重声明冲突与函数/对象双向种类冲突)

- 改动范围:`exec/parse2/control-result.tsv`(+2/-2,第 1181 行 `$fn_function0_759_FN_r_1 -> GV.compat.fnchk`、第 1767 行 `GV.link.done -> GV.compat`)、`exec/parse2/tentative-result.tsv`(+35/-0,`GV.compat*` 网段)、`tests/check-global-redecl-conflict.cx`(+86,18 例正负控)、`tests/manifests/c99-global-redecl-conflict.tsv`(+3)。
- 登记表同步:`sh ./unisacc.com tools/reg-manifest.cx -- --write` 与 `--check` 实测 `REG_MANIFEST manifests=735 bad=0 hard=0`;`tests/check-global-redecl-conflict.cx` 经已装产品 `-c` 编译通过(`rc=0`)。

### 1.2 7.2 不完整数组(`incomplete array declaration before another definition` / `without an initializer`)

对 `~/.local/share/unisacc/research/frontend-b1-ready/DESIGN.md` 第 2.3 节草案做逐状态追踪后发现三处必须修正的关键问题:

(1) **`DM.cd` 与 `INITCOUNT` 的先后关系**:
- `exec/parse2/control-result.tsv:426-428,490` 中,`DM.o2` 先调用 `INITCOUNT` 再落入 `$dimension_state47 -> DM.cd`。
- `DM.cd` 本身位于 `INITCOUNT` 之后:其动作 `[["JUMP","dm_back"],["COPYW","nv","dm_n"],["LDI","drk",0],["LDI","prd",1],["PUSH",["constant","dimension_state71"]]]` 直接跳回原声明 `]` 位置 `dm_back`,把 `nv = dm_n` 写入 `TDIM[0]` 与 `prd`,随后看到原声明末尾的 `;` 即 `RET` 返回 `DIMS`,**不会**进入 `INITCOUNT` 索要 `{}` 初始化器。

(2) **`DM.tent.test4` / `DM.tent.test7` / `DM.tent.test12` 的 `*` 分支不能提前跳 `DM.cd`**:
- `DM.tent` 是在首个不完整数组声明(如 `static int t[];`)处启动的前瞻扫描器,用于在同一翻译单元后续 token 流中寻找同名 `dm_self` 的 `= { ... }` 初始化器以确定首处声明的真实元素数 `dm_n`(`GV.storage` 在首个非 `extern` 声明处一次性发射存储尺寸 `gsz`,同单元后续同名声明在 `control-result.tsv:1805-1807` 走 `GV.epoch -> GV.record` 不再重复发射 `GV.storage`)。
- 若在扫描中途遇到其它声明的 `;`(`DM.tent.test4 *`)、函数体或结构体 `{`(`DM.tent.test7 *`)或其它变量的 `=`(`DM.tent.test12 *`)时立刻以 `dm_n = 1` 跳回 `DM.cd`,则当后续真正出现 `static int t[] = {10, 20, 30};` 时,首处已按 1 元素(`gsz = 4`)落表,第二处(`gsz = 12`)会被 `3775a03c71` 的 `GV.compat.szcmp` 误判为尺寸冲突(`conflicting types for object (C99 6.7p4)`)。
- 正确改法:
  - `DM.tent.test4 * -> DM.tent.next []`(跨过中间普通声明的 `;` 继续前瞻)
  - `DM.tent.test7 * -> DM.tent.broin []`(递增 `dm_br` 跨过中间函数体/复合块 `{ ... }` 继续前瞻)
  - `DM.tent.test12 * -> DM.tent.nexteq []`(跨过其它变量的 `=` 初始化器继续前瞻)
  - 仅在扫描到文件末尾 EOF(`tk == 9 -> DM.tent.none -> DM.tent.test14 *`)仍未遇到同名初始化器时,按 C99 6.9.2p2 补 1 元素(`["LDI","dm_n",1]`)收敛到 `DM.cd`。

(3) **`tsv-lint` 跨表 `unresolved` 计数零增长收敛**:
- `tentative-result.tsv` 由 `gen2parts-manifest.tsv` 按静态 section `main` 安装,其中每条 `next` 指向 `control-result.tsv` 状态(如 `DM.cd`)的行都会计 1 条 `unresolved`。若把 `DM.tent.test14 *` 直接改成 `next = DM.cd`,会使 `unresolved` 从基线 `10764` 增至 `10765` 触发 `tsv-lint --check` 失败。
- 正确改法:在 `tentative-result.tsv` 内增加本表汇聚态 `DM.tent.cd`:
  ```
  main	DM.tent.test14	1	DM.tent.cd	[]
  main	DM.tent.test14	*	DM.tent.cd	[["LDI","dm_n",1]]
  main	DM.tent.cd	*	DM.cd	[]
  ```
  保持全表指向 `DM.cd` 的跨表边仍为唯一 1 条,实测 `tsv-lint --check` `unresolved=10764 baseline=10764 errors=0`。

(4) **与 `0.0.98` 前端 A(`c99-extern-array-incomplete`)的跨版夹具冲突预警**:
- `0.0.98` 合入的 `tests/check-extern-array-incomplete.cx` 第 140、143 行含两条旧负控:
  - `AN1-static-incomplete`:`"static int a[];\nint b[4];\n"`(`expect_rc0 = 0`)
  - `AN4-non-extern-tentative`:`"int a[];\nint b[4];\n"`(`expect_rc0 = 0`)
- 这两条负控原本用于证明 `0.0.98` 的 `f7ce6cf561` 只放行 `extern int a[];` 而不改变非 `extern` 暂定数组;当 `0.0.99` 落地 7.2 后,这两条在文件末尾补为 1 元素转为 `rc = 0`(与 `t72a.c`、`t72d.c` 同形)。因此 `0.0.99` 合入 `0.0.98` 后须同步将 `check-extern-array-incomplete.cx` 中 `AN1`/`AN4` 的 `expect_rc0` 更新为 `1`(或改为 `int a[]; double a;` 类型冲突负控)。

### 1.3 t10 数组形参元素为 typedef 数组(`FN.array.typedefcheck`)

对 `DESIGN.md` 第 3.3 节草案(`FN.array.typedefcheck *` 直接照抄 `1` 分支做 `ALUI add td, td, 1`)做跨表追踪后发现一处静默错码缺陷并给出零新增行修法:

(1) **为什么不能在 `FN.array.typedefcheck *` 再做 `td += 1`**:
- 当解析 `typedef int row_t[4]; static int h(row_t rows[2])` 时,在读到形参名 `rows` 进入 `FN.pid -> FN.type`(`exec/parse2/shape-result.tsv:81-88`)时:
  - `FN.alias` 已调用 `SH.TYPECOPY` 将 `row_t` 的维度拷入 `TDIM[0] = 4, drk = 1`;
  - 因 `td == 0 == type_base_depth`,走 `FN.aliasarray`(`shape-result.tsv:88`),**已经执行过一次 `ALUI add td, td, 1` 与 `COPYW parrank, drk`**(此时 `td == 1, parrank == 1`),且**未调用 `SH.PREFIX`**(因为若后面没有 `[2]`,单层 `row_t r` 退化为 `int *`,不需多维 shape)。
- 当后面紧跟 `[2]` 进入 `FN.array -> FN.array.typedefcheck`(`fn_alias_arr != 0`,走 `*` 分支)时:
  - 若按草案再做一次 `ALUI add td, td, 1`,则 `td` 会变成 `2`(`int **rows` 而非 `int (*rows)[4]`),且 `parrank` 仍为 `1`(`FN.shape` 在 `shape-result.tsv:104` 要求 `CMPI parrank, 1 == 2` 才调用 `SH.NEW` 保存多维形状),导致 `rows[1][0]` 步长丢失或类型不匹配!
  - 对比 `row_t *rows`(`FN.aliasptr`,`shape-result.tsv:89-91`)与 `int rows[2][4]`(`FN.dimensions`,`shape-result.tsv:92-94`):两者都在 `TDIM[0]=4` 基础上调用 `SH.PREFIX`(把 `TDIM[0]=4` 右移到 `TDIM[1]=4`,置 `TDIM[0]=0, drk=2, parrank=2`),且最终 `td == 1`。

(2) **零新增行修法(不增加 `control-result.tsv` / `shape-result.tsv` 行数与 `tsv-lint` `unresolved` 计数)**:
- 在 `exec/parse2/shape-result.tsv:103` 的 `FN.shiftdone`(`SH.PREFIX` 唯一返回点)末尾补 `["COPYW","parrank","drk"]`:
  ```
  parameter-dimensions	FN.shiftdone	*	RET	[["LDI","dk",0],["STX","dk",["constant","TDIM"],"dk"],["ALUI","add","drk","drk",1],["COPYW","parrank","drk"]]
  ```
- 在 `exec/parse2/control-result.tsv:1254` 将 `FN.array.typedefcheck *` 原位改为:
  ```
  function2-common	FN.array.typedefcheck	*	SH.PREFIX	[["LDI","fn_alias_arr",0],["ALUI","sub","td","td",1],["PUSH","FN.array"]]
  ```
- 执行轨迹:`fn_alias_arr != 0` 时先清零 `fn_alias_arr`、将 `FN.aliasarray` 预加的 `td` 减回 `0`,调用同一 segment(`seg_ns-function-function2`)内的 `SH.PREFIX` 完成 `TDIM` 右移(`TDIM=[0,4], drk=2, parrank=2`)并返回 `FN.array`;二次进入 `FN.array` 时 `fn_alias_arr == 0` 走 `1` 分支(`td` 加回 `1`,步进 `NEXT` 进入 `FN.aqual` 解析 `2]`),最终在 `FN.bind -> FN.shape` 产出与 `int rows[2][4]` 完全一致的 `td=1, parrank=2, DIM=[0,4]`。

## 2. 组合补丁实测证据(`.claude/worktrees/v0.0.99`)

在 `.claude/worktrees/v0.0.99` 应用 `research/2026-11-23/v099-b1-combined.patch` 后实跑:

| 检查命令 | 实测结果原行 |
|---|---|
| `sh ./unisacc.com tools/reg-manifest.cx -- --check` | `REG_MANIFEST	manifests=735	bad=0	hard=0` |
| `sh ./unisacc.com tools/tsv-lint.cx -- --check` | `tsv-lint: ok: unresolved=10764 baseline=10764 ceiling=13260 errors=0` |
| `sh ./unisacc.com tools/notcovered-census.cx -- --check` | `notcovered: ok: rows=277 measured=34` |
| `sh ./unisacc.com -c tests/check-global-redecl-conflict.cx -o /tmp/chk-gredecl.o` | `rc=0` |

## 3. 版本分支提交门禁说明

当前 `main` 头 `d21f46dccf` 含提交 `9cf59a4cae` 写入的 `build/freeze.tsv`(`0.0.94-FREEZE ... cc 1795402800 src cx ext exec include seed kernel tests release`)。因 `tools/precommit.cx` 的 `freeze_gate()` 同时读取 `HEAD:build/freeze.tsv` 且未对非 `main` 的未来版本分支(`v0.0.NN`)做豁免,在不伪造 `UNISACC_INTEGRATOR`、不绕过门禁的前提下,本轮将已验证的完整组合补丁 `research/2026-11-23/v099-b1-combined.patch` 与本核查报告先行提交于 `v0.0.99` 分支;待 `0.0.94` 解冻或 `freeze_gate` 放行未来版本分支后直接 `git apply research/2026-11-23/v099-b1-combined.patch` 即可落表。

## 4. 三份可独立回退的提交草案序列(`research/2026-11-23/v099-b1-series/`)

已将组合补丁拆为三份按序正交、可独立 `git apply -R` 回退的补丁序列,并附合入 `0.0.98` 后对 `tests/check-extern-array-incomplete.cx` `AN1`/`AN4` 负控预期的同步补丁:

| 序号 | 补丁文件 | 行数 | 触及产品表(行号) | 配套夹具与清单 |
|---|---|---|---|---|
| 1 | `0001-b1-3775a03c71-gredecl.patch` | 184 | `exec/parse2/control-result.tsv`(1181, 1767), `exec/parse2/tentative-result.tsv`(65..99 追加 `GV.compat*`) | `tests/check-global-redecl-conflict.cx`, `tests/manifests/c99-global-redecl-conflict.tsv` |
| 2 | `0002-b1-72-incomplete-array.patch` | 165 | `exec/parse2/tentative-result.tsv`(43, 49, 59, 63..65), `tests/notcovered.tsv`(278 -> 277) | `tests/check-tentative-array-incomplete.cx`, `tests/manifests/c99-tentative-array-incomplete.tsv` |
| 2b | `0002b-after-v0098-an1-an4.patch` | 20 | 无(仅在合入 `0.0.98` 后更新 `tests/check-extern-array-incomplete.cx` 第 142/147 行 `AN1`/`AN4` 的 `expect_rc0` `0 -> 1` 并写明理由注释,不删断言) | `tests/check-extern-array-incomplete.cx` |
| 3 | `0003-b1-t10-array-param-typedef.patch` | 189 | `exec/parse2/control-result.tsv`(1254), `exec/parse2/shape-result.tsv`(103) | `tests/check-array-param-typedef.cx`, `tests/manifests/c99-array-param-typedef.tsv` |

正向依次应用与逆向逐个回退实测(在干净 `.claude/worktrees/v0.0.99` 上):
`APPLY_0001=OK -> APPLY_0002=OK -> APPLY_0003=OK -> REVERT_0003=OK -> REVERT_0002=OK -> REVERT_0001=OK`(回退后工作树零残留差异)。此外,`0003` 与 `0002` 的产品表改动完全不相交(`0003` 只动 `control-result.tsv:1254` 与 `shape-result.tsv:103`;`0002` 只动 `tentative-result.tsv:43..65` 与 `notcovered.tsv`),任意一项可单独回退而不破坏另一项的状态机闭环。

### 4.1 提交草案 1:`0001-b1-3775a03c71-gredecl.patch`

- 提交说明草案:`0.0.99 B1-1 (3775a03c71): 全局对象重声明类型/尺寸冲突与函数/对象双向种类冲突修复及夹具登记`
- 最小正负用例(18 例:8 正例 `P1..P8` + 10 负控 `C1..C10`)与已装产品 `0.0.93` 先红实测原行(`9/18`):
  ```
  GREDECL_CASE	P1-ext-incomp-complete	PASS	want_rc0=1	got_rc=0	err=-
  GREDECL_CASE	P2-ext-same-arr	PASS	want_rc0=1	got_rc=0	err=-
  GREDECL_CASE	P3-ext-same-scalar	PASS	want_rc0=1	got_rc=0	err=-
  GREDECL_CASE	P4-def-then-ext-incomp	PASS	want_rc0=1	got_rc=0	err=-
  GREDECL_CASE	P5-tentative-same	PASS	want_rc0=1	got_rc=0	err=-
  GREDECL_CASE	P6-ext-same-ptr	PASS	want_rc0=1	got_rc=0	err=-
  GREDECL_CASE	P7-tentative-arr	PASS	want_rc0=1	got_rc=0	err=-
  GREDECL_CASE	P8-typedef-arr-same	PASS	want_rc0=1	got_rc=0	err=-
  GREDECL_CASE	C1-ext-arr-size	FAIL	want_rc0=0	got_rc=0	err=-
  GREDECL_CASE	C2-ext-arr-double	FAIL	want_rc0=0	got_rc=0	err=-
  GREDECL_CASE	C3-ext-scalar-double	FAIL	want_rc0=0	got_rc=0	err=-
  GREDECL_CASE	C4-def-arr-size	FAIL	want_rc0=0	got_rc=0	err=-
  GREDECL_CASE	C5-ext-arr-elemtype	FAIL	want_rc0=0	got_rc=0	err=-
  GREDECL_CASE	C6-ext-ptr-vs-int	FAIL	want_rc0=0	got_rc=0	err=-
  GREDECL_CASE	C7-ext-then-static	PASS	want_rc0=0	got_rc=1	err=...: error: static declaration follows a non-static declaration of this name (C99 6.2.2p7)
  GREDECL_CASE	C8-fn-obj-same-name	FAIL	want_rc0=0	got_rc=0	err=-
  GREDECL_CASE	C9-typedef-arr-conflict	FAIL	want_rc0=0	got_rc=0	err=-
  GREDECL_CASE	C10-obj-then-fn	FAIL	want_rc0=0	got_rc=0	err=-
  GREDECL_SUMMARY	passed=9	failed=9	total=18
  ```
- 候选重建后预期转绿原行:`GREDECL_SUMMARY	passed=18	failed=0	total=18`(`rc=0`)。

### 4.2 提交草案 2:`0002-b1-72-incomplete-array.patch`(+ 合入 0.0.98 后附 `0002b-after-v0098-an1-an4.patch`)

- 提交说明草案:`0.0.99 B1-2 (7.2): 文件域不完整数组暂定定义跨定义前瞻与末尾 1 元素补齐及夹具登记`
- 最小正负用例(10 例:7 正例 `P1..P7` 含 Lua 跨函数前向声明 + 3 负控 `N1..N3`)与已装产品 `0.0.93` 先红实测原行(`2/10`):
  ```
  TENTARR_CASE	P1-static-before-global	FAIL	want_rc0=1	got_rc=1	err=...: error: not covered: incomplete array declaration before another definition
  TENTARR_CASE	P2-static-eof	FAIL	want_rc0=1	got_rc=1	err=...: error: not covered: incomplete array declaration without an initializer
  TENTARR_CASE	P3-tentative-eof	FAIL	want_rc0=1	got_rc=1	err=...: error: not covered: incomplete array declaration without an initializer
  TENTARR_CASE	P4-static-before-main	FAIL	want_rc0=1	got_rc=1	err=...: error: not covered: incomplete array declaration before another definition
  TENTARR_CASE	P5-tentative-use-write	FAIL	want_rc0=1	got_rc=1	err=...: error: not covered: incomplete array declaration before another definition
  TENTARR_CASE	P6-static-forward-init-after-fn	FAIL	want_rc0=1	got_rc=1	err=...: error: not covered: incomplete array declaration before another definition
  TENTARR_CASE	P7-static-init-then-global	PASS	want_rc0=1	got_rc=0	err=-
  TENTARR_CASE	N1-tentative-type-conflict	FAIL	want_rc0=0	got_rc=1	err=...: error: not covered: incomplete array declaration without an initializer
  TENTARR_CASE	N2-static-init-size-conflict	FAIL	want_rc0=0	got_rc=0	err=-
  TENTARR_CASE	N3-zero-size-array	PASS	want_rc0=0	got_rc=1	err=...: error: array size must be greater than zero (C99 6.7.5.2p1)
  TENTARR_SUMMARY	passed=2	failed=8	total=10
  ```
- 候选重建后预期转绿原行:`TENTARR_SUMMARY	passed=10	failed=0	total=10`(`rc=0`,且 `N1` 改由 `GV.compat` 报 `conflicting types for object (C99 6.7p4)` 命中 `want_err="conflicting types"`);合入 `0.0.98` 后应用 `0002b-after-v0098-an1-an4.patch` 使 `c99-extern-array-incomplete` 保持 `A0_SUMMARY	passed=14	failed=0	total=14`(`rc=0`)。

### 4.3 提交草案 3:`0003-b1-t10-array-param-typedef.patch`

- 提交说明草案:`0.0.99 B1-3 (t10): 元素为 typedef 数组的数组形参退化与 SH.PREFIX 维度平移及夹具登记`
- 最小正负用例(9 例:5 正例 `P1..P5` 含运行期步长校验 `rows[1][0]+rows[1][3]==19` + 4 负控 `N1..N4` 含 `t04`/`t05` 守卫保持拒绝断言)与已装产品 `0.0.93` 先红实测原行(`5/9`):
  ```
  ARRPARAM_CASE	P1-t10-bounded-2d-run	FAIL	want_rc0=1	got_rc=1,-1	err=...: error: not covered: array parameter whose element is a typedef array
  ARRPARAM_CASE	P2-typedef-arr-incomplete-param	FAIL	want_rc0=1	got_rc=1,-1	err=...: error: not covered: array parameter whose element is a typedef array
  ARRPARAM_CASE	P3-typedef-arr-1d-param	PASS	want_rc0=1	got_rc=0,0	err=-
  ARRPARAM_CASE	P4-typedef-arr-ptr-param	PASS	want_rc0=1	got_rc=0,0	err=-
  ARRPARAM_CASE	P5-plain-2d-param	PASS	want_rc0=1	got_rc=0,0	err=-
  ARRPARAM_CASE	N1-param-neg-bound	FAIL	want_rc0=0	got_rc=1	err=...: error: not covered: array parameter whose element is a typedef array
  ARRPARAM_CASE	N2-param-unclosed-bracket	FAIL	want_rc0=0	got_rc=1	err=...: error: not covered: array parameter whose element is a typedef array
  ARRPARAM_CASE	N3-t04-still-rejected	PASS	want_rc0=0	got_rc=1	err=...: error: not covered: array of pointers to arrays
  ARRPARAM_CASE	N4-t05-still-rejected	PASS	want_rc0=0	got_rc=1	err=...: error: not covered: pointer to pointer to array
  ARRPARAM_SUMMARY	passed=5	failed=4	total=9
  ```
- 候选重建后预期转绿原行:`ARRPARAM_SUMMARY	passed=9	failed=0	total=9`(`rc=0`,其中 `N1` 转报 `not covered: array parameter bound`、`N2` 转报 `not covered: expected ]`)。

## 5. 0.0.100 B2(17 个上限入 `exec/facts/k2-caps.tsv`)与 0.0.99 B1 的文件交集与并行开发判定

### 5.1 文件交集明细

| 类别 | 文件路径 | 0.0.99 B1 触及范围 | 0.0.100 B2 触及范围 | 是否交集与行距 |
|---|---|---|---|---|
| 产品表 | `exec/parse2/control-result.tsv` | 第 `1181` 行(`function0-common`)、第 `1254` 行(`function2-common`)、第 `1767` 行(`global5-common`),均为 `+1/-1` 原位替换 | 第 `510` 行(`structure` `STRUCT_LIMIT`)、第 `575` 行(`structure` `512`)、第 `2232` 行(`startup-guard` `POSSPAN`) | **交集 #1**:最小行距 **465 行**(`1767` vs `2232` 差 465 行;`575` vs `1181` 差 606 行),且 B1 行号零偏移,无 hunk 重叠 |
| 产品表 | `exec/parse2/shape-result.tsv` | 第 `103` 行(`parameter-dimensions` `FN.shiftdone`),`+1/-1` 原位替换 | 第 `121` 行(`descriptor-storage` `SH.NEW` `POSSPAN`) | **交集 #2**:相距 **18 行**(跨不同 section),且 B1 行号零偏移,远超 3 行 diff 上下文窗口,无 hunk 重叠 |
| 产品表 | `exec/parse2/tentative-result.tsv` | 第 `43, 49, 59, 63..65` 行 + 追加 35 行 `GV.compat*` | 不触及 | **不相交**(B1 独有) |
| 产品表/事实表 | `exec/facts/k2-caps.tsv`、`tools/facts-gen.cx` 及另外 11 张 `exec/` result/byte 表与对应 `*-manifest.tsv` | 不触及 | 新增 `k2-caps.tsv`、登记 `facts-gen.cx`、改 11 张表(`directive-scan-result`、`directive-body-byte`、`directive-body-result`、`unarycontrol-result`、`enumtypes-result`、`functiontypes-result`、`forward-result`、`layoutfacts-result`、`setjmpcontrol-result`、`modelgraphequality-result`、`x86object-result`)及 manifest `CAPS` 绑定 | **不相交**(B2 独有) |
| 棘轮基线表 | `tests/notcovered.tsv` | `rows 278 -> 277`(消去 `incomplete array declaration before another definition`) | 不触及(B2 仅把阈值字面量/常量迁入 `k2-caps.tsv`,不改拒绝文案) | **不相交**(B1 独有) |
| 登记表 | `tests/batches.tsv`、`tests/coverage.tsv` | 尾追 3 批(`c99-global-redecl-conflict`、`c99-tentative-array-incomplete`、`c99-array-param-typedef`) | 尾追 B2 检查批次 | **交集 #3**:纯表尾追加,由各自 `tests/manifests/*.tsv` 经 `tools/reg-manifest.cx -- --write` 自动重算,无手工合并冲突 |

### 5.2 并行开发判定

- **结论:快车道开发与补丁验证可完全并行;慢车道冻结与整链重建必须按版本序(`0.0.99` -> `0.0.100`)串行。**
- **依据**:
  - `(1)` **工作树开发零冲突**:在独立版本工作树 `.claude/worktrees/v0.0.99` 与 `.claude/worktrees/v0.0.100` 中并行开发时,仅有的两张交集产品表(`exec/parse2/control-result.tsv` 与 `exec/parse2/shape-result.tsv`)在 B1 中均为 `+1/-1` 原位替换(总行数不变),且与 B2 修改行的最小距离分别为 **465 行** 与 **18 行**,`git apply` / `git rebase` 零上下文冲突;两张登记表(`tests/batches.tsv`、`tests/coverage.tsv`)由各自独立的 `tests/manifests/*.tsv` 经 `reg-manifest.cx -- --write` 自动生成。
  - `(2)` **慢车道整链串行约束**:B1 与 B2 均修改 `exec/` 下 A 类产品输入(改变候选 SID,且 B2 新增 `exec/facts/k2-caps.tsv` 需 `derive --write` 更新 `build/table-index.tsv`),受「一个候选只含一类大改」与 `main` 单一冻结锁约束,必须待 `0.0.99` 合入 `main` 完成整链发布后,再将 `0.0.100` 变基合入 `main` 起链。
