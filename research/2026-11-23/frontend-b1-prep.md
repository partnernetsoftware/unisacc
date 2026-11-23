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
