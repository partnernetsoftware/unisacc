# 前端覆盖三期:范围评估与重排点定位(只读,未改代码)

2026-11-23,omp-go。依据 `plans/notcovered-plan.md:155-159`、`tests/notcovered.tsv`、`build/pkg-gaps.tsv:75,102`、`build/version-seq.tsv:32-39`、`plans/roadmap-0.0.83-0.0.99.md:95-98`,以及本文件的实跑实测。

## 1. 三期要做什么(逐项带现状实测)

### 1.1 声明符长尾(3 项,全部仍红)

实跑 `tests/notcovered-cases/` 四个复现,四条全部 REJECT:

| 复现 | 文案 | 判定位置 |
|---|---|---|
| t04_arr_of_ptr_to_arr | `not covered: array of pointers to arrays` | `exec/parse2/control-result.tsv:926` `PD.array.inarr` |
| t05_ptr_ptr_arr | `not covered: pointer to pointer to array` | `exec/parse2/control-result.tsv:922` `PD.array.more` |
| t10_array_param_typedef | `not covered: array parameter whose element is a typedef array` | `exec/parse2/control-reject.tsv:38` `fn_reject_array_typedef` |
| x16_block_extern_decl | `not covered: block-scope extern object without a visible file-scope declaration` | `exec/parse2/control-result.tsv:2415` `global5-common` `BX.unknown` |

### 1.2 18 条硬上限

`tests/notcovered.tsv` 的 `kind=cap` 实测 18 行,其中 `more than 6 arguments` 已是 ACCEPT,实际待处理 17 行,分布在 11 个表:

| 表 | cap 文案 |
|---|---|
| `exec/pp/directive-scan-result.tsv` | `#error text longer than 283 bytes` |
| `exec/pp/directive-body-result.tsv` | `more than 4000 #include splices`、`more than 4096 continuation lines` |
| `exec/parse2/unarycontrol-result.tsv` | `constant beyond int` |
| `exec/parse2/enumtypes-manifest.tsv` | `enum descriptor pool exhausted` |
| `exec/parse2/functiontypes-manifest.tsv` | `function signature pool exhausted` |
| `exec/parse2/functiontypes-result.tsv` | `recursive signature equality limit` |
| `exec/parse2/forward-result.tsv` | `host-forwarded call count exceeds 8192` |
| `exec/parse2/layoutfacts-result.tsv` | `ordered source layout facts limit or resource` |
| `exec/parse2/setjmpcontrol-manifest.tsv`、`-result.tsv` | `setjmp temporary depth`、`setjmp inside an expression nested too deeply` |
| `exec/parse2/shape-reject.tsv`、`gen2-actions-manifest.tsv` | `shape descriptor capacity` |
| `exec/modelgraphequality-result.tsv` | `signature graph equality budget` |
| `exec/parse2/control-reject.tsv` | `structure id capacity`、`structure member capacity`、`token input exceeds position domain` |
| `exec/enc/elfobject-result.tsv`、`x86object-manifest.tsv`、`x86object-result.tsv` | `object relocation capacity` |

新表 `exec/facts/k2-caps.tsv` **尚不存在**(实测 `ls` 失败)。

**关键范围收窄**:全仓 `CMPI` 有 1299 处 / 143 文件 / 1806 调用,但需要读表的只有上述 17 条 cap 判定所在的约 11 张表;其余 `CMPI` 是普通比较(如 `["CMPI","drk",8]` 之类),不属「上限文案」范围。

### 1.3 A 类文件估计与单位成本

按文件计(含新表与常量基址):约 **20 到 24 个产品输入文件**

- 网表 11 张(上表)
- 声明符与作用域:control-result、control-reject、shape-result、tentative-result
- 常量基址:k2-control.tsv、k2-gen2.tsv
- 新表:k2-caps.tsv

plan 写的「A 类 9-15 个」是按概念计;按文件数约 20-24,多出来的主要是 cap 分散在 11 张表而非 1 张。

单位拆解(plan 估 6 小时 = 2 单位):

| 项 | 估时 |
|---|---|
| t04、t05、t10 三项声明符 | 各约 1 小时,共 3 |
| x16 块作用域 extern | 1 |
| 17 条 cap 改读 k2-caps(含逐条实测可达性) | 2 |
| k2-caps 生成器与消费者遍历验收 | 1 |

合计约 7 小时 ≈ 2.3 单位,略高于 plan 的 6 小时。

## 2. 与 0.0.98 前端三项修复、硬上限评估的关系

### 2.1 三项修复的现状(重要更正)

三个提交 **都不在 main**:

```
ffbb722538 in-main=no   3775a03c71 in-main=no   f7ce6cf561 in-main=no
```

只在 `v0.0.94` 与 `v0.0.96` 分支;`tests/check-paren-lvalue-postinc.cx`、`check-global-redecl-conflict.cx`、`check-extern-array-incomplete.cx` 在 main 上均不存在。撤出提交 `3970fc6d5a` 的说明写的是「前端三项由前端类候选**编译覆盖缺口第二期**承接」——指派给第二期,不是第三期。

三项各改一张表:

| 提交 | 改的表 |
|---|---|
| ffbb722538 括号内可修改左值后缀 | `exec/parse2/unarycontrol-result.tsv`(+4/-4) |
| 3775a03c71 全局对象重声明冲突 | `exec/parse2/tentative-result.tsv`(+35) |
| f7ce6cf561 不完整 extern 数组前瞻早退 | `exec/parse2/tentative-result.tsv`(+4/-4) |

**与三期的重叠**:`tentative-result.tsv` 被 f7ce6cf561 与 3775a03c71 两提交都改,而三期的不完整数组拒绝也在 `tentative-result.tsv:43,49,59`。三张表(control-result、tentative-result、unarycontrol-result)三期与三项都碰,合入必须串行,不宜并行。

### 2.2 能否合成同一前端版

**能,而且应当**。理由:三类改动同属「前端网类(模型与网)」,符合「一个候选只含一类大改」;三项目前无归属(撤出后未排),单独占一版会付一次完整冻结成本。

但单位会超:三项修复已写代码与夹具(约 3 小时,只差合入与核验)+ 三期 7 小时 ≈ 10 小时 ≈ 3.3 单位,超过单版 1.5 单位上限。**建议拆两版**,或把三期里的 cap 读表拆到后续小版。

### 2.3 硬上限评估:实际范围比 pkg-gaps 小

`build/pkg-gaps.tsv:75` 写「成员上限 64」,**已过时**:

- 默认路径实测上限已是 **512**:`exec/parse2/control-result.tsv:575` 是 `["CMPI","t",512]`;
- 270 成员结构体 rc=0;511 成员 rc=0;520 成员报 `structure member capacity`(第 514 行);
- SQLite 的 sqlite3_api_routines 270 成员在默认路径**已能过**。

残留的 64 只剩**库模式**一处:`exec/parse2/librarytypes-result.tsv:69` 的 `["CMPI","d_members",64]`,正是 pkg-gaps 说的「可能两条路径并存」,已确认。

Lua 不完整数组:`static int t[];` 报 `not covered: incomplete array declaration without an initializer`(`tests/notcovered.tsv:124`,`kind=diag`,`sites=3`),判定在 `exec/parse2/control-reject.tsv:60` `extern_array_later`。按 notcovered 规则 (5),diag 类只需去 `not covered: ` 前缀;但 Lua 需要的是**能编过**,所以仍是真实能力缺口。

**结论**:硬上限评估的实际范围只剩两处(库模式成员 64、不完整数组),不是「5 条 ABI 加 2 条上限」的大包。

## 3. 串行依赖与版本落点建议

### 3.1 串行链

```
前端三项修复(3 表) -> 前端覆盖三期(同 3 表 + 11 张 cap 表) -> 标号函数局部化一期(control-result 同表) -> U7 后端按单元复用
```

三期与标号一期同动 `exec/parse2/control-result.tsv`,必须串行;三项与三期同动 `tentative-result.tsv` 与 `unarycontrol-result.tsv`,也必须串行。

### 3.2 版本落点

现序(`build/version-seq.tsv:32-39`)里 0.0.96 是进程生命周期、0.0.97 驱动判定迁事实表、0.0.98 K2 核心层强制第二步、0.0.99 诊断定位、0.0.100 .unit 带行号。**前端三期在现序里没有槽位**(roadmap 原 0.0.96 是前端三期,重排后 0.0.96 已换)。

建议:

| 版 | 内容 | 单位 | 前置 |
|---|---|---|---|
| 0.0.98 | 前端 A:三项修复合入 + x16 + 库模式成员 64 与不完整数组评估 | 2 | 无(三项在分支上待合入) |
| 0.0.99 | 前端 B:声明符长尾 t04/t05/t10 + 17 条 cap 读 k2-caps | 2 | 0.0.98 |
| 0.0.100 | 标号函数局部化一期(使能项) | 2 | 0.0.99 |
| 0.0.101 | U7 后端按单元复用(标号局部化后 e4 可缓存) | 2 | 0.0.100 |

原 0.0.98 的 K2 第二步顺延;`build/version-seq.tsv:36` 的 0.0.101 已写「前端硬上限由前端组评估后插到本版之前」,与本建议方向一致。

**总增 4 单位、4 版**;若必须压版数,可把 0.0.98 与 0.0.99 合成一版 4 单位,但违反单版 1.5 上限,需主调度豁免。

## 4. 风险与未查项

风险:

1. 三项修复在分支上基于旧表(control-result 在 `3970fc6d5a` 之后又改过),合入 main 会冲突,需重做补丁而不是 cherry-pick。
2. `tentative-result.tsv` 被两提交都改,合入顺序必须先 3775a03c71 后 f7ce6cf561 或反之,需逐行核。
3. 17 条 cap「按需放宽」每条要评估内存与时间,不是纯文案;`structure member capacity` 从 512 再放宽会撞 `MEMBER_STRIDE=4096` 与 `SMEM` 槽宽(`sid*512+i`),见 pkg-gaps:102 的结构体编号撞键缺陷。
4. `k2-caps.tsv` 是新 A 类输入,进源 SID 会改身份,需慢车道重建候选。

未查项:

- Lua 到底用哪种不完整数组形状(realprog 源不在仓内,只有 `tools/realprog-fetch.cx`);
- 库模式 64 的全部消费者:**已核**(2026-11-23)。见第 5 节补充。
- x16 的第二条 repro `v03_unclosed_main_fs`:**已造并实跑**,原包 0.0.93 拒绝。见第 6 节。
- shape 网 30 处 `CMPI` 与三项声明符的逐条对应未核;
- 17 条 cap 的「实际可达性」全部 UNMEASURED,未造最小复现。

## 5. 0.0.98 前端 A / 0.0.99 前端 B 的定稿拆法(cc 2026-11-23 采纳)

| 版 | 内容 | 触及产品表 |
|---|---|---|
| 0.0.98 前端 A | f7ce6cf561(不完整 extern 数组前瞻早退)+ ffbb722538(括号内可修改左值后缀)+ x16 块作用域 extern + 库模式成员 64 -> 512 | `tentative-result.tsv`(头部)、`membercontrol-result.tsv`、`membercontrol-manifest.tsv`、`unarycontrol-result.tsv`、`control-result.tsv`(BX 段)、`librarytypes-result.tsv` |
| 0.0.99 前端 B | 3775a03c71(全局对象重声明冲突)+ 不完整数组 `static int t[];` + 声明符三项 t04/t05/t10 + 17 条 cap 读 k2-caps | `control-result.tsv`(1181/1767/922/926)、`control-reject.tsv`(38/60)、`tentative-result.tsv`(尾部追加)、`unarycontrol-result.tsv`(185 常量上限)、另 10 张 cap 表 |

两版不共行:

| 表 | A 的行 | B 的行 |
|---|---|---|
| `tentative-result.tsv` | 第 2 行改写 + 新增 `DM.tent.ext` 2 行 | 原第 64 行之后追加 `GV.compat*` 35 行 + `DM.tent.test14` 分叉 |
| `unarycontrol-result.tsv` | 第 48、50 行 | 第 185 行 `DEAD.big` |
| `membercontrol-result.tsv` / `membercontrol-manifest.tsv` | 全部 | 无 |
| `control-result.tsv` | 2410-2432 行 BX 段 | 1181、1767、922、926 行 |
| `control-reject.tsv` | 无 | 38、60 行 |
| `librarytypes-result.tsv` | 69 行(64 -> 512) | 无 |

补丁落点实测(main 头 `998dc44aa6` 的 `git archive` 干净副本,`git apply --check`):

- 三份补丁的 `exec/parse2/*` 产品表与三份新增 check/manifest **全部 rc=0**;
- `tests/batches.tsv` 与 `tests/coverage.tsv` **全部 rc=1**,原因是 hunk 锚在分支表尾(`uaf-census`/`ckreceipt`/`core-abi-gen`),main 表尾已是 `e2steps`;纯位置漂移,不应用补丁而改尾追三行。
- 三份补丁按 f7ce -> 3775 -> ffbb 顺序生成(ffbb 与 3775 的 coverage hunk 上下文已含前两者的行),单独应用会因上下文缺行失败。
- 按 f7ce -> 3775 -> ffbb 依次 `git apply --include='exec/parse2/*'` 三次全 rc=0;最终 `tentative-result.tsv` 101 行 = 64 + 2 + 35,行区间不相交。

备料产物(不入仓):`~/.local/share/unisacc/research/frontend-a-prep/` 下三份 `.patch` 与 `README.md`。

### 5.1 库模式成员 64 的消费者(已核)

结论:**库模式成员存储与默认路径共用同一槽位布局 `sid*512+i` 与同一 `SMEM`/`SMN` 常量,上限 64 只是一处纯判定,改 512 不需要改存储路径。**

证据行:

| 位置 | 原文片段 |
|---|---|
| `exec/parse2/librarytypes-result.tsv:69` | `[["CMPI","d_members",64]]`,b18==2 -> `LX.fail` |
| `exec/parse2/librarytypes-result.tsv:90` | `[["ALUI","mul","d_key","d_sid",512],["ALU","add","d_key","d_key","d_i"],["LDX","d_key","d_key",["constant","b.SMEM"]]]` |
| `exec/parse2/control-result.tsv:578` | `SB.memberput` 尾部 `[["ALUI","mul","u","sid",512],["ALU","add","u","u","t"],["STX","u",["constant","SMEM"],"k"]]` |
| `exec/parse2/control-result.tsv:575` | 默认路径判定 `[["CMPI","t",512]]` |
| `exec/facts/gen2-constants.tsv:52,53` | `SMN 40000000`、`SMEM 41000000` |

因此 A 版只改 `librarytypes-result.tsv:69` 一处 64 -> 512,不需要动 `librarytypesv3-*`。

**但有一处仍需注意**:`librarytypesv3-result.tsv:12` 另有一条 `[["CMPI","d_members",["constant","LF.ENTRY_LIMIT"]]]`,上限来自 `exec/facts/layoutfacts.tsv:23` `ENTRY_LIMIT 128`。这是 layoutfacts 路径的独立上限(128,不是 64),不属 A 版 64->512 范围;若 Lua 走 v3 路径,128 是否够需单独评估(**未核**)。

## 6. x16 第二条复现 `v03_unclosed_main_fs`(已造,原包实跑)

文件内容(1 行,已存 `~/.local/share/unisacc/research/frontend-a-prep/x16/v03_unclosed_main_fs.cx`,不入仓):

```c
int main(void){ extern int nowhere_defined; nowhere_defined = 3; printf("%d\n", nowhere_defined); return 0; }
```

与 `tests/notcovered-cases/x16_block_extern_decl.cx` 的区别:x16 的文件域**有** `int shared_v = 5;` 但声明在函数之后(不可见);v03 的文件域**完全没有**该对象,是「真正无文件域声明」的最简形。

已装产品 0.0.93 实跑:

```
$ sh ./unisacc.com -c -b lnx/x86_64 v03_unclosed_main_fs.cx -o v03.o
rc=1
stdout: (空)
stderr: v03_unclosed_main_fs.cx:1:43: error: not covered: block-scope extern object without a visible file-scope declaration
          int main(void){ extern int nowhere_defined; nowhere_defined = 3; ...
                                                      ^
        1 error generated.
产物: 无 v03.o
```

拒绝点 `1:43` 是 `nowhere_defined = 3` 的赋值位置(声明本身通过,使用处拒绝),与 `control-result.tsv:2415` `BX.unknown` 的语义一致:声明扫描网走完 `BX.token/equal/comma/end` 后落到 `BX.unknown`。

因此 x16 修复后这两条都应转为接受(只要文件域存在可见声明);v03 这种「确实没有文件域声明」的用例按 C99 6.2.2p2 应当是合法 extern 声明,链接期才失败 —— 故修复后 v03 也应编译通过,链接时才报 undefined。这是 A 版验收必须覆盖的一条边界。
