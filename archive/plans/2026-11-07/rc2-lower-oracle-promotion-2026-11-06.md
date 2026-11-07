# lower 推广清单:11 批逐批要改什么

样板已成立(`stream-lower-independent`,20/0)。本文件只列**要改什么**,不改批次。
所有实测在同一候选(aa1250448d)、同一材料(`/tmp/wB/mat2/streams` 或按 target 现造)上做。

渲染器现状:`tests/lib/gen-fallguard-tail.cx` 只支持 `lnx/x86_64` 的 ABI(写死
`gate form=syscall`、`sysno` 取 write/exit 行、参数用 `mem`/`imm` 两种形态)。

变化规律(实测确认):**尾部长度随 target 变,但用户代码段与尾部起点都在
`ret form=<arch>` 之后**;尾部内容三种形态:
- `lnx/*`:1551 B,`setreg <reg>, imm <sysno> role=sysno` + `gate form=syscall`
- `osx/*`:1603 B(sysno 写成 `33554436` = 0x2000004 比 `1` 多 7 字节,`carry=true` 比 `false` 少 1 字节,净 +52),`sysno='0x2000004`
- `win/*`:1569 B,`winsave 456` / `winrest 456, rax` 包住 `setreg rcx/rdx/r8` + `gate form=winapi`、`sysno='none`、`retconv=wcount`

---

## 1) stream-lower-domain-format

- **当前红点**:`assertions=51 failed=11`,其中 10 条 `*.target-text-exact`、1 条 `zeros-not-stored`。
- **渲染器要补**:无(全 lnx/x86_64);复用样板。
- **改法**:把 `accepted()` 的整段 `target_equal` 拆成「用户代码段用现有 `want` + 尾部用渲染结果」,与样板同构。`zeros-not-stored` 是独立断言,不在尾部,需单独看(疑与 sparse 数据布局有关,非本次范围)。
- **预估**:约 25 行改动(改 `accepted` 一处即可覆盖 12 个 run)。

## 2) stream-lower-domain-input

- **当前红点**:`assertions=29 failed=8`,其中 4 条 `target-text-exact`、4 条 `reject-named`。
- **渲染器要补**:无。
- **改法**:同上(4 条走 `accepted`);剩下 4 条 `reject-named` 是拒绝措辞,与尾部无关,**要单独核**是不是同一批 F 行里的措辞偏差。
- **预估**:约 20 行 + 4 条拒绝措辞需确认。

## 3) stream-lower-domain-refusals

- **当前红点**:`assertions=115 failed=22`,19 条 `target-text-exact` + 3 条 `reject-stderr-exactly-declared-name`。
- **渲染器要补**:无。
- **改法**:19 条走 `accepted`(样板同构);3 条拒绝措辞同 #2,单独核。
- **预估**:约 25 行 + 3 条措辞。

## 4) stream-lower-domain-layout-plain

- **当前红点**:`assertions=778 failed=258`(run 数 258)。
- **渲染器要补**:无,但**这个模式的 run 数最多**,每个 run 都会调 `accepted`。
- **改法**:同 #1,改一处 `accepted` 即覆盖 258 个 run。
- **预估**:约 25 行;跑时长会明显增加(样板 12 个 run 约 12 秒,258 个 run 估计 3-4 分钟),需确认批次 55 秒预算够不够——**这可能要拆批**。

## 5) stream-lower-domain-layout-redef

- **当前红点**:`assertions=1516 failed=504`。
- **渲染器要补**:无。
- **改法**:同 #4。
- **预估**:约 25 行;504 个 run,**同样面临 55 秒预算问题**,大概率要拆批。

## 6) stream-lower-target-lnx-arm64

- **当前红点**:`assertions=17 failed=5`,4 条 `ret/registers/relocs/sparse.tape.target-text-exact`(第 5 条是 accept)。
- **渲染器要补**:`arm64` 的 ABI 行 —— 需要确认 arm64 的 sysno 寄存器与 `gate form`(表里有,本版没接),以及 `spinit` 是否带第二参数(targets 的期望里有 `spinit r7, 66592` 这种形态)。
- **改法**:`run_case` 的整段比较换成「用户段 + 渲染尾部」。
- **预估**:渲染器 +40 行(支持 arm64),批次 +20 行。

## 7) stream-lower-target-osx-x86_64

- **当前红点**:`assertions=17 failed=4/5`(随材料而定)。
- **渲染器要补**:**osx 的 ABI**:`sysno=33554436`、`carry=true`、`sysno='0x2000004`。表里有(`lower-abi.tsv` 的 osx 行),渲染器要读 `form`/`carry`/`sysno` 三个字段而不是写死。
- **改法**:同 #6。
- **预估**:渲染器 +60 行(osx 分支),批次 +20 行。

## 8) stream-lower-target-osx-arm64

- 与 #7 同,但 arm64 的寄存器与 `spinit` 参数形态不同。渲染器 +60 行(osx + arm64 组合)、批次 +20 行。
- **注意**:osx/arm64 的组合分支表里未必有现成行,可能要**新增 ABI 行**(那是产品侧事实表改动,要走 cc 批,不是测试能决定的)。

## 9) stream-lower-target-win-x86_64

- **当前红点**:`assertions=17 failed=4`,尾部 1569 B。
- **渲染器要补**:**win 形态大改**——`winsave 456` / `winrest 456, rax` 包住每个 gate;参数寄存器是 `rcx/rdx/r8` 不是 `rdi/rsi/rdx`;`gate form=winapi`、`sysno='none`、`retconv=wcount`。这些都在 `lower-abi.tsv` 的 win 行(`form=winapi`、`retconv=wcount`、`winimp`),但 `winsave`/`winrest` 的**位置规则**要从 `exec/lower/README.md` 的说明 + `enc-x86win-arity.tsv` 推,不是纯表驱动。
- **改法**:渲染器需要新增「win gate 包一层」的逻辑。
- **预估**:渲染器 +90 行,批次 +20 行。**这是最难的一批。**

## 10) stream-lower-target-win-arm64

- 同 #9,再加 arm64 寄存器。渲染器 +100 行,批次 +20 行。
- **注意**:win/arm64 的组合行在表里存在,但 `winsave` 的栈偏移可能不同(win/arm64 的 data-backed stack),要实测确认。

## 11) properties-cases-lower

- **当前红点**:`pass=9 fail=3`(不是 `assertions` 口径,是 property 口径)。
- **渲染器要补**:无,但**它跑的是 properties-cases-gen.cx 生成的用例**,不是直接跑 stream**。要改的是生成器输出的期望值,不是批次。
- **改法**:在 `tools/properties-cases-gen.cx` 的 lower 分组里,把期望值同样拆两段。这会让 toolsloc 涨,需 cc 批。
- **预估**:生成器 +30 行(toolsloc +30,需批),批次本身不改。

---

## 汇总

| # | 批次 | 当前红 | 渲染器增量 | 批次增量 | 难点 |
|---|---|---|---|---|---|
| 1 | domain-format | 11 | 0 | ~25 | zeros-not-stored 是独立断言 |
| 2 | domain-input | 8 | 0 | ~20 | 4 条拒绝措辞 |
| 3 | domain-refusals | 22 | 0 | ~25 | 3 条拒绝措辞 |
| 4 | domain-layout-plain | 258 | 0 | ~25 | **258 run,55 秒预算可能不够** |
| 5 | domain-layout-redef | 504 | 0 | ~25 | **504 run,几乎肯定要拆批** |
| 6 | target-lnx-arm64 | 5 | ~40 | ~20 | arm64 ABI 行 |
| 7 | target-osx-x86_64 | 4-5 | ~60 | ~20 | osx sysno 0x2000004 + carry |
| 8 | target-osx-arm64 | 4-5 | ~60 | ~20 | 组合行可能要先加表 |
| 9 | target-win-x86_64 | 4 | ~90 | ~20 | **winsave/winrest 包裹,最难** |
| 10 | target-win-arm64 | 4 | ~100 | ~20 | 同 + 栈偏移 |
| 11 | properties-cases-lower | 3 | 0 | 0(改生成器 ~30) | toolsloc 涨需批 |

**渲染器总增量**:约 350 行(其中 250 行是 osx/win/arm64 的 ABI 分支)。
**批次总增量**:约 190 行。
**必须先定的两件事**:
1. layout-plain/redef 的 258/504 个 run 在 55 秒预算下要不要拆批(不拆就是超时 UNKNOWN,不是 FAIL)。
2. target 各批的尾部能否**表驱动**(目前看 win 的 winsave/winrest 位置需要额外规则;若不能纯表驱动,要接受"渲染器里有一段显式规则"并写明)。
