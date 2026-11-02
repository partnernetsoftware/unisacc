# 装机批候选后验收清单（putc + R15 + UNISA_MAXSTEPS）

日期：2026-11-02，omp-ds。用途：bdy 的 W5 纪元出候选后立刻跑，确认三条"源码已修、现装产品仍红"的修复真正生效。
不在 `plans/w5-capacity-2026-11-02.md` 里合写的原因：该文件此刻有 bdy 的 staged 大改（81/-50），追加会形成
索引/工作区混叠；合并 W5 时把本文件当附节引用即可。

## 一、命令与期望（每条 ≤60 s）

| # | 命令 | 现装基线（产品 sha256 `d93c1b24…`） | 候选期望 |
|---|---|---|---|
| 1 | `sh ./unisacc.com tools/check-libc-probes.cx -- ROOT` | `CHECK_LIBC_PROBES failed=2`，红行是 `NOTE putc rc 1 FAIL putc/c`、`FAIL putc/cx` | `failed=0`，且 `PASS putc/c`、`PASS putc/cx` 两行在列；其余六例（sigusr/nan/scanset/pctn/scanfn/strtod）保持 PASS |
| 2 | `sh ./unisacc.com tools/check-run-tape.cx` | `CHECK_RUN_TAPE failed=5` | `CHECK_RUN_TAPE failed=0`（18/18）；磁带分支 argv 真绑定 |
| 3 | `sh ./unisacc.com tools/check-budget.cx -- ROOT` | `CHECK_BUDGET failed=2`（`FAIL bad-value-refused` + `NOTE bad exit 0`） | `failed=0`；`UNISA_MAXSTEPS=abc` 与 `=0` 具名拒绝 rc2，缺省回落 4e11 |
| 4 | `sh ./unisacc.com tools/check-stdio.cx` | `failed=0` | `failed=0`（回归：头文件重生成不得动标准流错误位） |
| 5 | `sh ./unisacc.com tools/check-errno.cx -- ROOT` | `failed=0` | `failed=0`（#8 已装机，作回归） |

第 1 条的门是名字表：`putc` 必须随新 P3 包进 prelude（`exec/facts/pp-autoinc-gen.tsv` 的 `keys` 有 `putc`、
`bodies` 有 `__UN_putc`；`facts-gen --check` 应 SAME）。只重编对象、把旧 P3 包原样附在尾部的候选仍会红——
这正是 `b39f3460…` 的失败方式。

## 二、字节等价（候选 vs 现装，同输入各跑一次比 SHA）

1. **合法镜像字节不变**：`int main(void) { return 3; }` 用 `-o a.img` 各跑一次，SHA256 必须相同。
   本轮三项都不许改编码字节：R15 只改运行期 argv 绑定、MAXSTEPS 只读环境、putc 只加头内函数。
   唯一允许的语义变化是原先编译失败的 `putc` 单元开始成功，不是同一单元的字节差异。
2. **磁带路线**：`tools/check-tapebin.cx` 与格式批保持 `BYTE-IDENTICAL`/行数不变；`-S` 产物对固定输入
   与现装逐字节相同。
3. **棘轮 9/9**：`sh ./unisacc.com tools/ratchets.cx`。关注两点——`cxapis` 导出 319/calls 与 `coreapi`
   26 模块 193 导出不得变（本批没有新增导出符号）；`putc` 出现在 prelude 名字表是模型输入，若相关计数
   变化必须在提交说明里单列，不得混在"不变"里。
4. **落尾回归**：`sh ./unisacc.com tools/tape-fallthrough-cases.cx -- ROOT /新目录` 7/7 PASS（现装已 7/7，
   候选不得回退；argv 绑定改动触及磁带运行路径，这条要跑）。
