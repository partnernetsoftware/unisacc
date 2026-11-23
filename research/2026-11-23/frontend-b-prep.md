# 0.0.99 前端 B 备料(只读,未改仓)

2026-11-23,omp-go。基线 main 头 `a5169cbf21`。补丁与试应用全部在 `git archive HEAD` 干净副本上实测;未起版本分支。

补丁目录:`~/.local/share/unisacc/research/frontend-b-prep/`

## 1. 3775a03c71 补丁与试应用

`3775a03c71.patch`(13049 B,`git show --format='From %H%nSubject: %s%n'`)。

| 文件 | + | - | 性质 |
|---|---|---|---|
| `exec/parse2/control-result.tsv` | 2 | 2 | 第 1181 行 `$fn_function0_759_FN_r_1 -> GV.compat.fnchk`;第 1767 行 `GV.link.done -> GV.compat` 并补 `LDI gc_gmark` |
| `exec/parse2/tentative-result.tsv` | 35 | 0 | 原第 64 行之后追加 `GV.compat*` 网 |
| `tests/check-global-redecl-conflict.cx` | 86 | 0 | 新增 |
| `tests/manifests/c99-global-redecl-conflict.tsv` | 3 | 0 | 新增(batch/coverage/packages) |
| `tests/batches.tsv` | 1 | 0 | 登记行 |
| `tests/coverage.tsv` | 1 | 0 | 登记行 |

逐文件 `git apply --check`(main 头干净副本):

| 文件 | rc |
|---|---|
| `exec/parse2/control-result.tsv` | 0 |
| `exec/parse2/tentative-result.tsv` | 0 |
| `tests/check-global-redecl-conflict.cx` | 0 |
| `tests/manifests/c99-global-redecl-conflict.tsv` | 0 |
| `tests/batches.tsv` | 1 |
| `tests/coverage.tsv` | 1 |

整份补丁 rc=1,报错原文:

```
error: patch failed: tests/batches.tsv:1927
error: tests/batches.tsv: patch does not apply
error: patch failed: tests/coverage.tsv:1555
error: tests/coverage.tsv: patch does not apply
```

与 A 备料同一原因:hunk 锚在分支表尾(`ckreceipt` 行),main 表尾已不是该行;纯位置漂移。做法同 A:产品表与新增文件直接应用,两张登记表不应用补丁而尾追三行。

## 2. 声明符三项

三项都已实跑确认现状拒绝(已装产品 0.0.93,`-c -b lnx/x86_64`)。

### 2.1 t04 指针数组的数组 `int (*q[2])[4];`

| 项 | 值 |
|---|---|
| 复现 | `tests/notcovered-cases/t04_arr_of_ptr_to_arr.cx` |
| 现状 | rc=1,`t04_arr_of_ptr_to_arr.cx:1:41: error: not covered: array of pointers to arrays`(报在 `q[0]=m` 处,声明本身通过) |
| 拒绝点 | `exec/parse2/control-result.tsv:926` `PD.array.inarr\t1\tDEAD\t[["REJECT","not covered: array of pointers to arrays"]]` |
| 判定来源 | `exec/parse2/control-result.tsv:923` `PD.array.more * -> PD.array.inarr [["CMPI","fp_inarr",1]]`;`fp_inarr` 由 `dimensions` 段 `FPD.a`(`:359`)置 1,`FPSTART`(`:337`)清零 |
| 改法 | `PD.array.inarr` 的 `1` 分支改走 `PD.array`(即 `:927` 那条),让 `DIMS -> SH.NEW -> PD.arraydone` 承担数组形状;`fp_inarr` 仍需保留给 `fp_more` 判定 |
| A 类文件数 | 1 张产品表(`control-result.tsv`,约 3 到 6 行)+ 新 check + manifest + 2 行登记 = 4 到 5 个文件 |

### 2.2 t05 指向数组的指针的指针 `int (**pp)[4];`

| 项 | 值 |
|---|---|
| 复现 | `tests/notcovered-cases/t05_ptr_ptr_arr.cx` |
| 现状 | rc=1,`t05_ptr_ptr_arr.cx:1:57: error: not covered: pointer to pointer to array`(报在 `(*pp)[1][2]`) |
| 拒绝点 | `exec/parse2/control-result.tsv:922` `PD.array.more\t1,2\tDEAD\t[["REJECT","not covered: pointer to pointer to array"]]` |
| 判定来源 | `exec/parse2/control-result.tsv:921` `PD.tail.test @token27 -> PD.array.more [["CMPI","fp_more",1]]`;`fp_more` 每遇一个 `(` 递增(`dimensions` 段 `FPD.moreptr`,`:344`) |
| 改法 | `PD.array.more` 的 `1,2` 分支改走 `PD.array`,由 `DIMS` 处理最内层 `[4]`,`fp_more` 退化为只影响 `td` 前进量(`PD.fpmore` `:933` 已在做 `td += fp_more`) |
| A 类文件数 | 1 张产品表(与 t04 同表,合计约 6 到 10 行)+ 同一 check 里加 2 例 = 4 到 5 个文件(t04 与 t05 可合一个 check) |

注意:t04 与 t05 的拒绝点同在 `parameter-declarators` 段的 `PD.array.more`/`PD.array.inarr` 两行,一个 check 覆盖两例即可。

### 2.3 t10 数组形参元素为 typedef 数组

| 项 | 值 |
|---|---|
| 复现 | `tests/notcovered-cases/t10_array_param_typedef.cx` |
| 现状 | rc=1,`t10_array_param_typedef.cx:2:24: error: not covered: array parameter whose element is a typedef array` |
| 拒绝点 | `exec/parse2/control-result.tsv:1254` `FN.array.typedefcheck\t*\tDEAD\t[["@","fn_reject_array_typedef"]]` |
| 文案 | `exec/parse2/control-reject.tsv:38` `fn_reject_array_typedef\tnot covered: array parameter whose element is a typedef array` |
| 判定来源 | `exec/parse2/control-result.tsv:1249` `FN.pid * -> NEXT [["COPYW","fn_alias_arr","type_shape"],...]` 记下形参形状;`:1253` `FN.array * -> FN.array.typedefcheck [["CMPI","fn_alias_arr",0]]`,非 0 即拒 |
| 改法 | `FN.array.typedefcheck` 的 `*` 分支不再 DEAD,改为与 `1` 分支同路(`td += 1` 后进 `FN.aqual`),让 `FN.aqual/FN.abound` 的 `[`/`]` 解析承担 `rows[2]`;`fn_alias_arr` 只用于决定 `td` 步长(typedef 数组元素已是完整类型,不需再展开) |
| A 类文件数 | 2 张产品表(`control-result.tsv` 约 4 行、`control-reject.tsv` 删 1 行或保留给别的路径)+ 新 check + manifest + 2 行登记 = 5 个文件 |

## 3. 17 条 cap 逐项清单

`tests/notcovered.tsv` 里 `kind=cap` 共 18 行:17 行 `UNMEASURED`、1 行 `ACCEPT`(`more than 6 arguments`,复现 `r14_seven_args`,已接受,不在本次范围)。

| # | cap 文案 | 守卫行(产品表:行号) | 上限来源 | 复现 |
|---|---|---|---|---|
| 1 | `#error text longer than 283 bytes` | `exec/pp/directive-scan-result.tsv:12` `CMPI t 283`(t=LE-NS);拒绝 `:13` | 字面量 283,无常量 | 无 |
| 2 | `more than 4000 #include splices` | `exec/pp/directive-body-byte.tsv:43,46` `CMPI NINCL 4000`;拒绝 `directive-body-result.tsv:16` | 字面量 4000 | 无 |
| 3 | `more than 4096 continuation lines in headers` | `exec/pp/directive-body-byte.tsv:79,84` `CMPI NISP ["constant","MAXISPL"]`;拒绝 `directive-body-result.tsv:5` | `MAXISPL`=4096 | 无 |
| 4 | `constant beyond int` | `exec/parse2/unarycontrol-result.tsv:177` 进 `DEAD.big`;拒绝 `:185` | 无字面量,是 `C64` 溢出判定 | 无 |
| 5 | `enum descriptor pool exhausted` | `exec/parse2/enumtypes-result.tsv:8,41` `CMPI en_base/en_code ["constant","ENUM_LIMIT"]`;拒绝 `:16` | `ENUM_LIMIT`=128(`exec/facts/enumtypes.tsv:4`) | 无 |
| 6 | `function signature pool exhausted` | `exec/parse2/functiontypes-result.tsv:28` `CMPI fs_code ["constant","SBB"]` | `SBB`=8192 | 无 |
| 7 | `host-forwarded call count exceeds 8192` | `exec/parse2/forward-result.tsv:17` `CMPI fw_count 8192` | 字面量 8192 | 无 |
| 8 | `object relocation capacity` | `exec/enc/x86object-result.tsv:31` `CMPI obj_nrel 262144`;拒绝 `:32` | 字面量 262144 | 无 |
| 9 | `ordered source layout facts limit or resource` | `exec/parse2/layoutfacts-result.tsv:4-6`(`RLD lf_flag` 0-1 才继续)与 `:11` `CMPI lf_sid 128` | 字面量 128 与资源标志 | 无 |
| 10 | `recursive signature equality limit` | `exec/parse2/functiontypes-result.tsv:388` `CMPI fs_eqdepth 31`;拒绝 `:393` | 字面量 31 | 无 |
| 11 | `setjmp inside an expression nested too deeply` | `exec/parse2/setjmpcontrol-result.tsv:86` `CMPI sj_depth 40`;拒绝 `:88` | 字面量 40 | 无 |
| 12 | `setjmp temporary depth` | `exec/parse2/setjmpcontrol-result.tsv:89` `CMPI sj_depth 0`;拒绝 `:91` | 字面量 0(下溢) | 无 |
| 13 | `shape descriptor capacity` | `exec/parse2/shape-result.tsv:121` `CMPI shape_next ["constant","POSSPAN"]`;拒绝 `:122` | `POSSPAN`=67108864 | 无 |
| 14 | `signature graph equality budget` | `exec/modelgraphequality-result.tsv:12` `CMPI mg_next 17409`、`:23` `CMPI mg_framelevel 66`;拒绝 `:2` | 字面量 17409 与 66 | 无 |
| 15 | `structure id capacity` | `exec/parse2/control-result.tsv:510` `CMPI nsid ["constant","STRUCT_LIMIT"]`;拒绝 `:512` | `STRUCT_LIMIT`=2049 | 无 |
| 16 | `structure member capacity` | `exec/parse2/control-result.tsv:575` `CMPI t 512`(t=`SMN[sid]`);拒绝 `:577`;槽位 `:578` `sid*512+i` | 字面量 512(与 A 版库模式 64 是两套) | 无 |
| 17 | `token input exceeds position domain` | `exec/parse2/control-result.tsv:2232` `CMPI toklen ["constant","POSSPAN"]`;拒绝 `:2233` | `POSSPAN`=67108864 | 无 |

读取点分布(13 张 result 表,不是 11 张;cc 的 11 可能按 notcovered 的 `exec/...` 前缀去重后少算了 `pp/directive` 的三张与 `control` 的两段):

`exec/pp/directive-scan-result.tsv`、`exec/pp/directive-body-byte.tsv`、`exec/pp/directive-body-result.tsv`、`exec/parse2/unarycontrol-result.tsv`、`exec/parse2/enumtypes-result.tsv`、`exec/parse2/functiontypes-result.tsv`、`exec/parse2/forward-result.tsv`、`exec/parse2/layoutfacts-result.tsv`、`exec/parse2/setjmpcontrol-result.tsv`、`exec/parse2/shape-result.tsv`、`exec/modelgraphequality-result.tsv`、`exec/enc/x86object-result.tsv`、`exec/parse2/control-result.tsv`。

另有三处 REJECT 文案住在 manifest 而非 result:`exec/parse2/enumtypes-manifest.tsv`(`mapseq.capacity`)、`exec/parse2/functiontypes-manifest.tsv`(`mapseq.reject`)、`exec/parse2/shape-reject.tsv`(`reject5`)。改上限时 result 里的守卫改常量、manifest 的文案不变。

A 类文件数(17 条):13 张 result + 3 处 manifest/reject + `exec/facts/k2-caps.tsv` + `tools/facts-gen.cx` + 1 个 check + manifest + 2 行登记,约 **20 到 22 个文件**。这是 A 类输入改源,必须慢车道。

## 4. k2-caps.tsv 设计草案

### 4.1 为什么单独一张表

现在这些上限散在两种形态:

- 已进常量表:`MAXISPL`、`ENUM_LIMIT`、`SBB`、`POSSPAN`、`STRUCT_LIMIT`,但分散在 `gen2-constants.tsv`、`enumtypes.tsv`、`k2-control.tsv(=consts json)` 三处;
- 纯字面量写在动作里:283、4000、8192、262144、128、31、40、0、17409、66、512。

改一处上限要读网表找字面量,这正是本次要消灭的。

### 4.2 表形状

两种既有形态:

- 纯两列:`exec/facts/layoutfacts.tsv` 头是 `# name\tvalue`,一行一个名字;
- `=KEY json value`:`exec/facts/enumtypes.tsv` 的 `=ENUM_LIMIT\tjson\t128`。

`k2-caps.tsv` 选 **`=KEY\tjson\t<value>`** 形态,理由是 `k2-control.tsv` 的 `=consts` 已是这种绑定,manifest 里能直接写 `CAPS=@str:<key>` 或 `X=CAPS.X`。草案:

```
# cap limits read by parser/pp/encoder; one row per cap
# writer: tools/facts-gen.cx; inputs: this file
=ERR_TEXT_MAX	json	283
=INCLUDE_SPLICE_MAX	json	4000
=CONTINUATION_MAX	json	4096
=INT_CONST_BITS	json	32
=ENUM_POOL	json	128
=SIGNATURE_POOL	json	8192
=FORWARD_CALL_MAX	json	8192
=RELOC_MAX	json	262144
=LAYOUT_SID_MAX	json	128
=SIG_EQ_DEPTH_MAX	json	31
=SETJMP_EXPR_DEPTH_MAX	json	40
=SHAPE_SPAN	json	67108864
=MODELGRAPH_NODE_MAX	json	17409
=MODELGRAPH_FRAME_MAX	json	66
=STRUCT_ID_MAX	json	2049
=STRUCT_MEMBER_MAX	json	512
=TOKEN_POS_SPAN	json	67108864
```

要点:

- `SHAPE_SPAN` 与 `TOKEN_POS_SPAN` 现在同值(67108864)但是两个独立语义,必须两行,不能合成一个;
- `=SETJMP_TEMP_MIN`(0)这类下溢判定不建议进表:0 不是可调上限,改成表反而鼓励误调;
- `=INT_CONST_BITS` 是 `C64` 语义位宽,不是数量上限,若表里放它需要消费点从 `C64` 改成 `CMPI`,改动更大,建议单独评估。

### 4.3 接线

1. `tools/facts-gen.cx`:新增该表的 writer 登记(表头声明 writer、inputs、sha 前缀),`--check`/`--write` 覆盖到它;`--check` 必须能报出差异表名。
2. manifest 绑定:每张 result 表的 manifest 在 `bind` 列加 `CAPS=@str:k2-caps`(参照 `exec/parse2/gen2-manifest.tsv:36` 的 `extra=extra` 与 `POSSPAN=POSSPAN` 写法)。
3. result 守卫:把 13 张表里第 3 节的字面量/旧常量换成 `["constant","CAPS.<KEY>"]`。
4. 旧常量收敛:`gen2-constants.tsv` 的 `MAXISPL`、`enumtypes.tsv` 的 `ENUM_LIMIT`、`k2-control.tsv` 的 `POSSPAN`/`STRUCT_LIMIT`/`SBB` 是否删或保留别名,需一次决定;建议**先加新表、把消费者指向新表,旧常量保留一个版本**,避免同一次改动同时动两套绑定。

### 4.4 慢车道理由

新 facts 表是 A 类输入:它进 `build/table-index.tsv` 与 SID,整链身份变化;`facts-gen --check` 与 `derive --check` 都要重跑,`source-job-wave` 等构造链要重新取证。不能走快车道。

## 5. 与 A 版的行边界复核

### 5.1 实测

在 main 头干净副本上先应用 A 的两份补丁(`git apply --include='exec/parse2/*'`),再试 B 的 `tentative-result.tsv`:

```
A(f7ce + ffbb) 应用后 tentative-result.tsv = 66 行
B(3775) 的 tentative hunk 头 @@ -64,3 +64,38 @@
B 应用后 tentative-result.tsv = 101 行
B 应用后第 64-67 行:
  main\tDM.tent.test13\t*\tDM.tent.none\t[]
  main\tDM.tent.test14\t1\tDM.cd\t[]
  main\tDM.tent.test14\t*\tDEAD\t[["REJECT","not covered: incomplete array declaration without an initializer"]]
  main\tGV.compat\t*\tGV.compat.fnd\t[...]
```

结论:**A 先落、B 后落,tentative 无冲突**。A 只改第 2 行并在其后插 2 行(`DM.tent.ext`),B 的 hunk 上下文是 `DM.tent.test13*/test14*` 三行,A 不碰这三行,git apply 按偏移 2 应用成功。B 的 35 行落在第 67 行之后,与 A 的头部区完全不相交。

### 5.2 7.2 的落点(与 A 仍不相交)

7.2 的拒绝文案实测是 **`incomplete array declaration before another definition`**,不是 `extern_array_later`。实测两个最小形都报这条:

```
static int t[];  + main()      -> rc=1  "incomplete array declaration before another definition"
int u[];         + main()      -> rc=1  同上
static int t[];  + t[0]=1 使用 -> rc=1  同上
```

该文案在 `exec/parse2/tentative-result.tsv:43`(`DM.tent.test4`)、`:49`(`test7`)、`:59`(`test12`)、`:64`(`test14`,`without an initializer`)。四处都在第 40 行之后,即 **B 的区域**;A 只在第 2 行附近动。故 7.2 归 B 成立。

`control-reject.tsv:60` 的 `extern_array_later` 是第二条路径,本次四个最小形都没走到它,需另造 `extern int t[];` + 后续定义/使用的形去确认;若确实可达,它也在 B 的 `control-reject.tsv` 区域(A 不碰该表),不改变拆法。

### 5.3 边界表(更新版)

| 表 | A(0.0.98) | B(0.0.99) |
|---|---|---|
| `exec/parse2/tentative-result.tsv` | 第 2 行改写 + 其后插 2 行 | 第 43/49/59/64 行(7.2)+ 第 67 行后追加 35 行(3775) |
| `exec/parse2/control-result.tsv` | 第 2410-2432 行 BX 段(x16)+ 922/926 行(t04/t05)+ 1254 行(t10)+ 1181/1767 行(3775) | 无 |
| `exec/parse2/membercontrol-result.tsv` / `-manifest.tsv` | 全部(ffbb) | 无 |
| `exec/parse2/unarycontrol-result.tsv` | 第 48、50 行(ffbb) | 第 185 行(第 4 条 cap) |
| `exec/parse2/librarytypes-result.tsv` | 第 69 行(64 -> 512) | 无 |
| `exec/parse2/control-reject.tsv` | 无 | 第 38 行(t10)、第 60 行(7.2) |

与 A 备料文档的边界表相比,变化只有一处:声明符三项(t04/t05/t10)按 cc 定稿归 B,因此 `control-result.tsv` 从「B 独有」变成 **A 与 B 同表**。但 A 在 `control-result.tsv` 只动 2410-2432(BX 段)与 69 行以外的库模式行,B 动 922/926/1254/1181/1767,行区间不相交;实测 B 的 `control-result.tsv` 补丁在当前头 rc=0,A 的 x16 与 t04/t05/t10 都是新增行,不删除 B 的锚点上下文。**风险点**:若 B 的 t04/t05 改动需要改 `PD.array.more` 与 `PD.array.inarr` 两行的观察列(它们正是 3775 hunk `@@ -1178,7` 的上下文邻域),则 B 内部要按「先 3775、再 t04/t05」的顺序做,不能同时改同一 hunk 窗口。建议 B 内顺序:3775 -> 7.2 -> t04/t05 -> t10 -> caps。

## 6. 未做/未核

- 未起版本分支,未提交任何文件。
- 17 条 cap 全部 `UNMEASURED`:没有一条有可达复现(只有 `more than 6 arguments` 是 ACCEPT 且有 `r14_seven_args`)。要证明「改读新表后行为不变」,得先造能触发每条上限的夹具;这本身是独立工作量,不在本次范围。
- `control-reject.tsv:60` `extern_array_later` 的触发条件未确认。
- `=INT_CONST_BITS` 是否该进表、`C64` 是否可改 `CMPI` 未评估。
- t04/t05/t10 的改法只做了网表阅读,未实改验证;`PD.array.inarr` 改走 `PD.array` 后 `SH.NEW` 是否接受「数组的数组」形状未验证。
