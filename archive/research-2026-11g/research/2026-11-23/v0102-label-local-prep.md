# 0.0.102 标号函数局部化第一阶段 (Label-Local Phase 1) 开版前核查报告

日期: 2026-11-23
分支与工作树: `v0.0.102` (`.claude/worktrees/v0.0.102`, 基于 `origin/main` `430e3595e3`)
依据: `build/version-seq.tsv:37`、`build/owner-reqs.tsv:51` (`R60`)、`plans/v0.0.95.md:57-66`、`prd.md:743`、`research/2026-11-23/label-local-phase1.md` (`0ac364aabc`) 及本次对 `exec/parse2/*`、`exec/facts/*`、`tools/facts-gen.cx`、`cx/compiler/tapelink.cx` 的逐行核查。

---

## 一、先前未查项补齐与关键机制核实

在 `research/2026-11-23/label-local-phase1.md` 第 6 节列出的 3 项未查项已全部查清:

1. **全仓 `exec/parse2/*` 中 `"lab"` 与 `"rl"` 触点普查**:
   - `"lab"` 递增点共 **19 个状态行**(含单行双递增/三递增共 26 次 `["ALUI","add","lab","lab",1]`):
     - `exec/parse2/control-result.tsv` (16 个状态行):
       * `:30` (`if $state23`), `:39` (`if S.else`)
       * `:54` (`if-warnings $state41`), `:63` (`if-warnings S.else`)
       * `:69` (`loops S.while`, +2), `:115` (`loops F.ready`, +3), `:159` (`loops S.do`, +3)
       * `:249` (`switch S.sw`, +2), `:308` (`switch $case_unique_store`), `:314` (`switch $default_scope_check`)
       * `:878` (`ladder-and $ladder_fresh2`), `:885` (`ladder-or $ladder_fresh8`, +2)
       * `:1413` (`function8-plain FN.go`, `lab+=1; rl=lab`), `:1450` (`function8-warnings FN.go`, `lab+=1; rl=lab`)
       * `:2204` (`sizeof3 SZ.lvalue`, `lab+=1`), `:2245` (`ordinary-string-common U.str.advance`, `lab+=1`)
     - `exec/parse2/membercontrol-result.tsv` (1 个状态行): `:190` (`part2.all AS.large.init`, +2: `as_top`, `as_end`)
     - `exec/parse2/return-result.tsv` (1 个状态行): `:142` (`qt0 D1.qt.original`, +2: `a`, `b`)
     - `exec/parse2/callcontrol-setjmp-template.tsv` (1 个状态行): `:9` (`save-entry {sj.entry}`, +2: `sj_resume`, `sj_end`)
   - `"lab"` 初始化点共 **2 处**: `exec/parse2/startup-actions.tsv:2` 与 `exec/parse2/gen2-manifest.tsv:107` (`startup_data` 的 `["LDI","lab",0]`)。
   - `"rl"` 引用点共 **8 处**:
     - 赋值 2 处: `exec/parse2/control-result.tsv:1413` 与 `:1450` (`FN.go`: `["COPYW","rl","lab"]`);
     - 发射返回标号 4 处: `exec/parse2/control-result.tsv:1423` (`FN.tl`), `exec/parse2/return-result.tsv:10,36,67` (`S.rvc`, `$f14`, `S.rj`);
     - `goto` 函数归属键 2 处: `exec/parse2/gotolabels-result.tsv:6,8` (`GL.def` 与 `GL.end`: `glc_fn = (unit_epoch << 32) + rl + 1`)。
2. **`{@name}` 的真实展开机制**:
   - `seed/gen.c` 并不在运行期做 `{@name}` 模板替换;`exec/parse2/tape-templates.tsv:40` 的 `statement_label_prefix = "u_{@name}."` 是由 `exec/parse2/control-text.tsv:18,53` 按 `{...}` 切分为 fragment `0` (`text16 = "u_"`) 与 fragment `2` (`label_separator = "."`),而在状态机 `exec/parse2/control-result.tsv:236,243` (`S.lab` / `S.gt`) 中显式执行 `[["@","text16"],["SPAN2","fns","fne"],["@","label_separator"],...]` 从源码 token 区间 `[fns, fne)` 输出当前函数名。
   - 重要发现: 全编译器所有 `__unisacc_L<n>` 与 `__unisacc_R<n>` 的发射点,均在输出前缀(`"__unisacc_L"` 或 `"__unisacc_R"`)并设置 `["COPYW","n",<reg>]` 后**统一跳转到 `exec/parse/numeric-result.tsv:11` 的 `PRN` 状态**打印十进制数 `n`。
3. **为什么 `gotolabels-result.tsv` 必须用独立单调寄存器 `fn_seq` (新发现的负控漏洞)**:
   - 当 `FN.go` 将 `lab` 重置为 `1` 后,同一单元内每个函数的 `rl` 都等于 `1`。
   - 若 `gotolabels-result.tsv:6,8` 继续用 `rl` 计算 `glc_fn = (unit_epoch << 32) + rl + 1`,则同单元内所有函数的 `glc_fn` 完全相同:
     * 当 `f1` 定义了标号 `L_only_f1:`, `LABEL_DEFINED["L_only_f1"]` 被写入 `glc_fn`;
     * 紧接着 `f2` **未定义** `L_only_f1` 却写了 `goto L_only_f1;`, `f2` 在 `GL.end` 检查 `LABEL_DEFINED["L_only_f1"] == glc_fn` 时会因两函数 `glc_fn` 相同而**漏报未声明标号错误**(编译期 `e3` 放行非法跨函数 `goto`)!
   - 因此 `FN.go` 必须递增独立单调寄存器 `fn_seq`(`startup_data` 初始化 `LDI fn_seq 0`,`FN.go` 执行 `ALUI add fn_seq fn_seq 1`),并把 `gotolabels-result.tsv:6,8` 的 `rl` 换为 `fn_seq`。

---

## 二、涉及文件清单与 A 类上限核算 (`a_files_cap = 16`)

经核对 `build/lane-inputs.tsv`,下列 `exec/parse2/*`、`exec/facts/*`、`cx/compiler/tapelink.cx` 均属 **A 类产品输入**:

### 方案甲: 按模板与文本表拆分 (14-15 个 A 类文件, 对 `a_files_cap=16` 余量 1-2)

| 序号 | 文件路径 | A 类归属 | 改动内容 | 估计涉及行数 |
|---|---|---|---|---|
| 1 | `exec/parse2/control-result.tsv` | A | `FN.go` (`:1413,:1450`) 重置 `lab=1` 并递增 `fn_seq+=1`; 标号发射点输出 `SPAN2 fns fne` + `.` | ~25 行 |
| 2 | `exec/parse2/membercontrol-result.tsv` | A | `:190` `AS.large.init` 标号输出接函数名 | 4 行 |
| 3 | `exec/parse2/return-result.tsv` | A | `:10,:36,:67` `jump __unisacc_R` 与 `:143..192` 标号输出接函数名 | 10 行 |
| 4 | `exec/parse2/callcontrol-setjmp-template.tsv` | A | `:19..23` `sj_*` 标号输出接函数名 | 4 行 |
| 5 | `exec/parse2/gotolabels-result.tsv` | A | `:6,:8` `GL.def` 与 `GL.end` 中 `rl` 改为 `fn_seq` | 2 行 |
| 6 | `exec/parse2/tape-templates.tsv` | A | `ret_int`、`fn_tail`、`label_a..e`、`jump*`、`and_skip`、`or_skip`、`switch_compare_prefix` 模板更新 | 13 行 |
| 7 | `exec/parse2/callcontrol-text.tsv` | A | `sj_lea`、`sj_middle`、`sj_resume_label`、`sj_label_prefix` | 4 行 |
| 8 | `exec/parse2/membercontrol-text.tsv` | A | `text14..17` | 4 行 |
| 9 | `exec/parse2/return-text.tsv` | A | `text0` | 1 行 |
| 10 | `exec/parse2/startup-actions.tsv` | A | `:2` 新增 `["LDI","fn_seq",0]` | 1 行 |
| 11 | `exec/parse2/gen2-manifest.tsv` | A | `:107` `startup_data` 新增 `["LDI","fn_seq",0]` | 1 行 |
| 12 | `exec/facts/k2-control.tsv` | A | 同步 `=textrows` | 1 行 |
| 13 | `exec/facts/k2-call.tsv` | A | 同步 `=seqtexts` | 1 行 |
| 14 | `exec/facts/k2-membercontrol.tsv` | A | `tools/facts-gen.cx -- --write` 自动刷新 `# input` sha256 头 | 2 行 |
| 15 | `cx/compiler/tapelink.cx` | A | `:435-444` `tl_loc` 支持 `[__u<N>_]__unisacc_[LR][<fn>.]<digits>` | 8 行 |

### 方案乙 (推荐优化): 共享标号数字打印入口 `PRNL` (收敛至 8-9 个 A 类文件, 余量 7-8)

- 在 `exec/parse/numeric-result.tsv` (或 `control-result.tsv`) 增加共享入口 `PRNL`: 先输出 `["SPAN2","fns","fne"],["OUT",46]` (当 `fns < fne` 时输出 `<fn>.`), 再直落 `PRN` 打印 `n`。
- 这样所有 `"__unisacc_L"` / `"__unisacc_R"` 字面量前缀保持原样不拆, **无需改动** `tape-templates.tsv`、`callcontrol-text.tsv`、`membercontrol-text.tsv`、`return-text.tsv`、`k2-call.tsv`、`k2-membercontrol.tsv` 共 6 个 A 类文件, A 类总数降至 **9 个**(对 `a_files_cap=16` 余量 7)。

---

## 三、与 `0.0.99` (B1) / `0.0.100` (B2) / `0.0.101` (x16) 的交集核查

| 对比版本 | 文件交集 | 行段与 Section 交集 | 并行与合入判定 |
|---|---|---|---|
| `0.0.100` (前端 B2: 17 上限入 `k2-caps.tsv`) | **0 个文件** (`0.0.100` 不碰任何 `exec/parse2/*` 表) | **0** | 完全正交, 可全并行 |
| `0.0.99` (前端 B1: `3775a03c71` + `7.2` + `t10`) | **1 个文件**: `exec/parse2/control-result.tsv` (`0.0.99` 另改 `objectdefinitions-result.tsv`、`tentative-result.tsv`、`gen2-actions-manifest.tsv`、`k2-gen2.tsv`, 与 `0.0.102` 不交) | **0 行重叠**: `0.0.99` 仅改 `control-result.tsv` 的 `function10` (`:1495-1505`, `FN.array.typedefcheck` / `FN.shiftdone`); `0.0.102` 改 `if`/`loops`/`switch` (`:30-314`)、`ladder` (`:878-885`)、`function8` (`:1413,:1423,:1450`)、`sizeof3`/`ordinary-string` (`:2204,:2245`) | 快车道在各自工作树可并行开发预演 (`git apply` 零冲突, 最近行距 >45 行); 慢车道按版本序 `0.0.99 -> 0.0.100 -> 0.0.101 -> 0.0.102` 串行合入 `main` |
| `0.0.101` (前端 C: `x16` 块作用域 `extern`) | 预计共触 `exec/parse2/control-result.tsv` (局部声明段 `local` `:1600-2100`) | 分属 `function8` (`:1413-1450`) 与 `local` (`:1600+`) 不同 section | 快车道并行、慢车道按 `0.0.101 -> 0.0.102` 顺序合入 |

---

## 四、最小先红用例实测 (`probe-label-local.cx`)

已在 `.claude/worktrees/v0.0.102/research/2026-11-23/v0102-label-local-prep/probe-label-local.cx` 构造包含三条核心断言的最小先红探针:

1. **`L1_F2_INVARIANCE` (在现 `0.0.94` 产品上稳定先红 `FAIL`)**:
   - 同一源码单元含 `f1`、`f2`(内含 `if` + `goto L; L:`)、`main`;
   - 样本 A 中 `f1` 含 1 个 `if`, 样本 B 中 `f1` 含 2 个 `if`, `f2` 源码一字未改;
   - 断言: (a) `f2` 的 `-S` 汇编切片在 A 与 B 下必须逐字节相同; (b) `f2` 的局部标号名含函数名限定 `__unisacc_Lf2.` / `__unisacc_Rf2.`;
   - 现产品实跑结果: **先红 `FAIL`** (`f1` 增加一个分支导致 `f2` 的标号从 `__unisacc_L4/__unisacc_R3` 漂移为 `__unisacc_L5/__unisacc_R4`, 且 `fn_qualified=0`):
     ```text
     L3_NEG_CROSS_FN_GOTO	PASS	rc=1
     L1_F2_INVARIANCE	FAIL	f2a_bytes=546	f2b_bytes=546	fn_qualified=0
     ```
2. **`L2_SAME_GOTO_LABEL` (双函数同名 `goto L; L:` 正控)**:
   - `f1` 与 `f2` 均使用同名用户标签 `L:`, 编译运行返回 `0` (`u_f1.L` 与 `u_f2.L` 已带函数名前缀, 加上 `fn_seq` 后 `gotolabels` 键亦不冲突)。
3. **`L3_NEG_CROSS_FN_GOTO` (跨函数未声明标号负控, 防 `rl=1` 撞键漏放行)**:
   - `f1` 定义 `L_only_f1:`, `f2` 不定义该标号而直接写 `goto L_only_f1;`;
   - 断言: 必须非零退出并报 `use of undeclared label 'L_only_f1'` (若只重置 `lab=1` 而不把 `gotolabels-result.tsv` 的 `rl` 换为 `fn_seq`, 此负控会因 `f1` 与 `f2` 的 `glc_fn` 同为 `unit_epoch<<32 + 2` 而被误判通过)。
