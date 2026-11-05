# RT-SHARED 进度报告（omp-go，仓外 /tmp/rtw）

最后更新：wip9 之后（判据偏移已修，六项验收已跑）

## ① 旧 .o 能否被识别（判据逐字）

判据 = 对象 tape 里存在以 `.bss g_` + probe 符号名 + 空格开头的定义行。

对象 tape 里的符号写作 `g_` 加 C 名，所以 `_u_st_fd` 写成 `g__u_st_fd`，定义行是：

```
.bss g__u_st_fd 32
```

- 旧产品（主树已装）编出的 `OLD.o`：`strings -a OLD.o | grep -E '^\.bss g__u_st_fd [0-9]+$'` → 命中。
- 新候选按 RT_EXTERN 编出的对象：只有 `U _u_st_fd` 这种引用，不命中。
- 单源（默认 static）对象：命中，但只有一个对象，不报警。

**符号名不写死**：`_u_st_fd` 这一段来自 `exec/facts/rt-shared.tsv` 的 `probe_symbol` 列，
经 `tools/rt-shared-gen.cx` 生成 `cx/compiler/rtsharedname.cx`，`src/drivelink.cx` 与
`src/stages.cx` 都从 `rts_at()` 读，代码里没有符号字面量。

## ② env-state

`include/stdlib.h:892` 的 `static char **_unisa_envheap; static __u_word _unisa_envcap;`
已列入 `exec/facts/rt-shared.tsv` 的 `env-state` 组（同一组宏）。

## ③ 三态与初值（`_U_ZERO`）

`include/sys/_exit.h` 与 `include/errno.h` 用同一套三态：

| 宏状态 | `_U_SHARED` | `_U_ZERO` |
|---|---|---|
| 无宏（默认） | `static` | `= 0` |
| `RT_UNIT` | 空（本单元出实体） | `= 0` |
| `RT_EXTERN` | `extern` | 空（extern 不能带初值） |

默认分支保留 `= 0` 是**单源字节等价的关键**：上一版把初值去掉后，单源 tape 里多出一条
`.st` 清零指令，`-S` 产物不再逐字节相同。

`errno` 同一套三态（`RT_UNIT` 定义 / `RT_EXTERN` 声明 / 默认 `static int errno = 0`）。

## ④ 生成器与表外 static 扫描

`tools/rt-shared-gen.cx`：读 `exec/facts/rt-shared.tsv`，写 `cx/compiler/rtsharedname.cx`，
`--check` 还扫 `include/*.h` 与 `include/sys/*.h` 里**列 0 的** `static` 声明（文件作用域），
逐个核对是否在 `state_symbols` 或 `exempt_symbols` 列里：

```
RT_SHARED_GEN	SAME	groups=2	bytes=2315	unlisted=0
```

上一版漏掉 `_u_st_wlen` / `_u_st_ro` / `_u_st_n` 就是这套扫描第一次跑出来的（当时 135 条，
其中 45 条是真漏、其余是函数内局部；加"只看列 0"后收敛到 0）。

豁免列（第 7 列）写着理由：函数内临时量与缓冲、ffi 桥句柄表。

## ⑤ 驱动按单元注入

- `src/driveunits.cx`：进逐单元循环前 `st_units(nsrc)`。
- `src/stages.cx`：`st_pub()` 里对每组按 `unit == provider` 追加 `unit_macro`，否则 `extern_macro`；
  单单元（`nunit <= 1`）不加任何宏。define 记录带结尾 NUL（与 `-D` 经 `cxa_record` 的形态一致）。
- 实测：不带 NUL 时产品报 `reject: not covered: invalid CLI macro name`。

## ⑥ -c 链接诊断（选 A：警告，rc 不变）

警告行原文（逐字）：

```
unisacc: warning: 2 objects each carry a private stdio table (cross-unit FILE state is not shared; compile the program in one invocation or link the RT_UNIT entity)
```

## 验收实测（rev a9b32741f7，含判据修正）

| # | 项 | 结果 |
|---|---|---|
| 1 | 三场景 + devfull 批次 | `STDIO_CROSSUNIT assertions=9 failed=0` |
| 2 | 混链出告警 | `rc=0`，stderr 一行 `private stdio table`（a_old.o + b_new.o + m_new.o） |
| 3 | 单对象不告警 | `missing _start entry in a_old.o`，无告警 |
| 4 | 未用 stdio 的多对象不告警 | n1.o + n2.o → 无告警 |
| 5 | 单源 `-o` 产物 cmp | 逐字节相同 |
| 6 | 系统 ld 不劣化 | 新旧对象多重定义集合都是 `{__init_u}`，无新增 |
| 7 | 65/72/129 | 65 build rc0 / run rc0；72 build rc0 / run rc0；129 rc1 `too many sources (limit 128)` |
| 8 | smoke | 4/5（唯一红是 `cx-tool-srcshape`：基线上调问题，见下） |
| 9 | install-fixes | `failed=0` |
| 10 | ratchets | 7/12（红：srcshape、toolsloc） |
| 11 | rt-shared-gen --check | `SAME groups=2 bytes=2315 unlisted=0` |
| 12 | entry.elf 字节 | 改前 3438193B `29b44d17…`；改后 3448676B `3516c093…`（**+10483B**） |

## 落主树前要裁定的两件事

1. **toolsloc +294**（72625 → 72919）：其中 `tools/rt-shared-gen.cx` 282 行，超过"单笔 ≤150 行自批"。
2. **srcshape 上浮**：`src/` 5774 行（基线 5705，+69），逐文件超基线的是
   `drivelink.cx 277/236`、`driveunits.cx 451/449`、`stages.cx 421/395`。
   srcshape 的规矩是"只降不升"，所以要么裁代码，要么由督导批一次上浮。
   `smoke 5/5` 里那一项就是它。
