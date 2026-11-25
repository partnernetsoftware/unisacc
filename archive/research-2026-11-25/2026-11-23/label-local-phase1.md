# 标号函数局部化一期:只读评估(改动面、串行关系、夹具、收益)

2026-11-23,omp-go 只读评估,未改任何代码。依据:`prd.md` 候选池 R60 条目(在 `plans/v0.0.95.md:48-57` 有完整评审结论)、`prd.md` U7 实验结论条、本目录 `u7-experiment.md` 的实测。

## 1. 现状事实(逐条带出处)

- 标号是**全程序全局计数**:`exec/parse2/control-result.tsv:1413` 与 `:1450` 在函数入口 `ALUI add lab lab 1` 后 `COPYW rl lab`;`lab` 只在 `exec/parse2/startup-actions.tsv:2` 与 `exec/parse2/gen2-manifest.tsv:107` 的 `startup_data` 里 `LDI lab 0` 初始化一次。
- 生成的标号名形如 `__unisacc_L<n>` / `__unisacc_R<n>`(`exec/parse2/tape-templates.tsv:15-28`,`label_a..label_e` 与 `ret_int`/`fn_tail`)。
- 多单元时前端给单元加前缀 `__u<N>_`,见 `cx/compiler/tapelink.cx:435-444` 的 `tl_loc`:`[__uN_]__unisacc_L<n>` / `R<n>`。也就是说**单元间已经不撞**,撞的是同单元内不同函数之间(编号本身全局唯一,所以现在不撞,但编号随引用集变,不能做缓存键)。
- `goto` 的函数归属用另一套键:`exec/parse2/gotolabels-result.tsv:3-6` 注释写明 `function key = unit_epoch * 2^32 + rl + 1`,`rl` 是每定义一个的返回标号。**rl 每函数不同,正好可以当函数序号**;但评审结论指出「必须新增独立函数序号寄存器给 goto 键用,否则 rl 每函数相同会撞键」——这条与代码不符:当前 `rl` 已经来自全局递增的 `lab`,所以每函数唯一。若一期把 `lab` 改成函数内计数,`rl` 就失去唯一性,必须另给 goto 键一个不随引用集变的函数序号。

## 2. 要改的表与单元(A 类估计)

### 2.1 表(全部在 `exec/` 下,进产品输入)

| 表 | 现状 | 一期要改什么 | 规模 |
|---|---|---|---|
| `exec/parse2/control-result.tsv` | 16 处 `ALUI add lab lab 1`(全局递增),`FN.go` 里 2 处 `COPYW rl lab` | 函数入口保存/清零 `lab`、尾恢复;`rl` 改为独立函数序号 | 2471 行,涉 18 行 |
| `exec/parse2/membercontrol-result.tsv` | 1 处递增 | 同上 | 220 行,涉 1 行 |
| `exec/parse2/return-result.tsv` | 1 处递增 | 同上 | 227 行,涉 1 行 |
| `exec/parse2/callcontrol-setjmp-template.tsv` | 1 处递增 + 3 处 `__unisacc_L` 字面量 | 同上 + 标号名带函数名 | 涉 4 行 |
| `exec/parse2/tape-templates.tsv` | `label_a..e`、`ret_int`、`fn_tail` 共 7 条含 `__unisacc_L/R`;已有先例 `statement_label_prefix = "u_{@name}."` | 标号模板改成 `__unisacc_L{@name}.{a}` 形态(或等价),沿用既有 `{@name}` 替换 | 88 行,涉 7 行 |
| `exec/parse2/callcontrol-text.tsv` | 4 条 `sj_*` 字面量含 `__unisacc_L` | 跟随模板改 | 涉 4 行 |
| `exec/parse2/membercontrol-text.tsv` | 4 条 | 同上 | 涉 4 行 |
| `exec/parse2/return-text.tsv` | 1 条 `jump __unisacc_R` | 同上 | 涉 1 行 |
| `exec/parse2/gotolabels-result.tsv` | 函数键 `unit_epoch<<32 + rl + 1` | 换独立函数序号寄存器 | 涉 2 行 |
| `exec/parse2/gen2-actions-manifest.tsv` | 1 处 `__unisacc_L` 字面量 | 跟随 | 涉 1 行 |
| `exec/parse2/startup-actions.tsv` | `LDI lab 0` | 保留(函数入口清零) | 5 行 |
| `exec/parse2/gen2-manifest.tsv` | `LDI lab 0` | 保留 | 970 行,涉 1 行 |
| `exec/facts/k2-control.tsv` | 2 处提及 `lab`(寄存器清单) | 加函数序号寄存器 | 6 行 |
| `exec/facts/k2-call.tsv` | 2 处 | 同上 | 14 行 |

合计 **11 张表 + 2 张 facts**,涉行约 **35 行**。

### 2.2 单元(编译器侧)

- `cx/compiler/tapelink.cx:435-444` `tl_loc`:若标号名改成 `__unisacc_L<fn>.<n>`,这里是唯一解析器,必须同时接受新形态(否则 `tl_dce` 把所有新标号当普通标签,死代码删除会失效——`tl_loc` 的返回值决定「这个标签是编译器局部标号还是用户标签」)。
- `seed/gen.c`:46 处 `install_section_classes`,其中 `exec/parse2/control-result.tsv` 3 处(`:5539`、`:5620`、`:7698`)、`return-result` 1 处(`:5713`)。这些是「把表装进图」的调用点,表结构不变时不需要改;若新增寄存器名,只需 facts 表加行。**估算改 0 到 2 行**。
- 生成的汇编里标号名变长(`__unisacc_L` 已 10 字节,加函数名会明显变长):评审结论要求「标号名变长增加带子字节(需缩短 `__unisacc_` 前缀对冲)」。实测 `root.s` 里 1832 处标号引用,若每条多 8 字节,`one.s`(854 KB)约增 15 KB,可接受;但 `tapelink` 与 `elf` 段的符号表会变大。

**A 类文件数估计:11 张表 + 2 张 facts + 1 个编译器单元(`tapelink.cx`)= 14 个文件**;与评审结论的「A 类 8-10 个」同量级,略多是因为我按「表文件」而非「表概念」计数。

## 3. 与前端覆盖三期同动 `parse2` 表的串行关系

`plans/notcovered-plan.md:155-159` 的第三期改 **`exec/parse2/control-result.tsv` 与 `shape` 网**(声明符长尾、块作用域 extern、18 条硬上限改读 `exec/facts/k2-caps.tsv`)。与标号一期**同动 `control-result.tsv`**。

结论:**必须串行**。理由:两期都改 `control-result.tsv` 的行,`install_section_classes` 按 section 装图,同一 section 的 `state`/`observation` 冲突时后者覆盖前者(表是覆盖语义),并行改会互相吞行。串行顺序建议:先三期(它引入 `k2-caps.tsv` 这一新事实表并改 18 处上限文案,面积更大),再标号一期(只动 `lab`/`rl` 与模板)。

另:三期如果新增 `exec/facts/k2-caps.tsv`,标号一期新增的函数序号寄存器也应走同一模式(facts 表 + 生成器),不要写死在表里。

## 4. 先红后绿夹具思路

**必做用例(评审结论点名)**:`goto` 双函数同名标号撞键。

```c
int f1(void){ int x=1; goto L; L: return x; }
int f2(void){ int y=2; goto L; L: return y; }
int main(void){ return f1()+f2()-3; }
```

现状(`sh ./unisacc.com -o /tmp/goto2 /tmp/goto2.cx`):rc=0,运行 rc=0,**已经正确**(全局 `lab` 计数让两函数的 `L` 得到不同编号 `__unisacc_L2`/`__unisacc_L4`)。所以这个用例现在是绿的,一期改成函数内计数后它会**变成红的**(两函数都是 `__unisacc_L0` 或类似),这正是先红后绿要抓的点。

夹具设计:

1. **红**:一期补丁先只改表、不改 goto 键,上面程序应编译失败或运行错(标号撞键)。
2. **绿**:补上独立函数序号寄存器后,该程序 rc=0 且 `-S` 输出里两函数的 `L` 标号名不同(函数名在标号里),逐字节比对 `-S` 与 `-o`。
3. **对照**:`sh ./unisacc.com -S` 对同一程序,改前改后除标号名外**字节等价**(用本目录 `cmpfn.cx` 的 FLAT 口径,应 100%)。
4. **负控**:单函数内两个 `goto` 指向不同标号,应各自正确;嵌套可调用体(评审点名的「可调用体嵌套函数要不要成栈」)用一个带 `setjmp` 的函数验证。

## 5. 收益

- **一期自身不省毫秒**,是使能项(与 R60 结论一致)。
- 使能后:`e3`/`e4`/`lower` 可按单元独立(本目录 `u7-experiment.md` 实测:4200 行规模下 FLAT 口径 100%,RAW 口径 0%,差异全在标号全局编号与 `.unit` 行)。
- **e4 同字节率从 0% 到 100%**:这是 `u7-experiment.md` 的直接结论。当前按单元复用做不成,因为 RAW 口径 0%;标号局部化 + `.unit` 行归一是 RAW 口径达到 100% 的前提。
- 二期(串池、静态局部 `ls`/`__cl`)与缓存二期(补齐 `image` 路 e3 接受外部引用、`fwm_merge` 保留 `main`、`.unit` 源名帧归一)之后,评审估计缓存二期可省 e3 约 67 ms、e4 约 31 ms;对 43 秒的 devsvc-core 冷编是 0.1% 量级,真正收益在「改一个子单元后重编」走缓存。

## 6. 未查项

- `lab` 是否还有除 `control-result.tsv` 之外的递增点:`membercontrol-result`(1)、`return-result`(1)、`callcontrol-setjmp-template`(1),共 3 处已计;未逐行核 `printfcontrol`/`unarycontrol`/`ladder` 等同族表。
- 函数序号寄存器落在哪个寄存器文件(`k2-control.tsv` 的寄存器清单)未查;需与 `k2-call.tsv` 一起核。
- `{@name}` 在模板里的替换实现未定位到 `seed/gen.c` 的具体行(只有 `tape-templates.tsv:2` 的 `fn_head` 与 `:40` 的 `statement_label_prefix` 用到),需读 gen.c 的模板展开函数。
