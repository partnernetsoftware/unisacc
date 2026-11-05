# 分段字节流用例(ST4)

检查器:`sh ./unisacc.com tests/check-stages.cx`,批次 `stages`。每段一个目录:`args`、`in`、`expect`,可选 `bad`、`bad.err`、`bad.args`。
`args` 里写 `--source a.c` 这类相对标签,运行时 cwd 是该段的目录。
链式约束:image 管道 e2→e1→e3→sidecar→prune→lower→elf 中,每段的 `in` 等于上一段的 `expect`,检查器逐字节核对。
锚点:elf/expect 与 `unisacc -o` 编 e2/a.c 的产物逐字节相同。
一个段没有拒绝用例就记 GAP(uncovered),检查器 rc 非零,这个段不算通过。

## 人工核对记录(2026-11-05,已装 0.0.44:entry 29b44d17 / pkg 8a974386)

| 段 | 输入 → 输出 | 人工核对要点 | 拒绝用例 |
|---|---|---|---|
| e2 | a.c 26B → 53B | 头部是 `UNIPP1\0`,长度 0x1a=26,之后是 a.c 原文 | `#if` 没有表达式 → `reject: not covered: #if expression` |
| e1 | 53B → 211B | `USLATTRT` 属性头 + `UNITOK1` 头;e2 的正文原样保留;token 依次为 type=int、id=main、`(`、type=void、`)`、`{`、return、num=7、`;`、`}`、eof,末尾 `11 tokens` 与 11 个 token 相符 | 垃圾输入 → `malformed preprocessing locations` |
| e3 | 211B → 639B | 带 USLFW1 side-car;tape 里有 `_start`、`call main`、`main:`,main 里有 `imm r0, 7` | bad.c 里 `return x`(x 未定义)→ `bad.c:1:23: error: unknown identifier`,列 23 正是 x |
| sidecar | 639B → 608B | 去掉 31B 的 USLFW1 side-car(没有 host slot),只留下 tape | 垃圾输入 → `forwarding side-car missing` |
| e4(O1) | b.c 的 tape → 981B | b.c 是 `int a=2; return a+5;`,e4 会改变 tape;接着走 prune/lower/elf,终点与 `-O1 -o` 逐字节相同(构造用例时实测) | **GAP**:垃圾输入 rc0,原样透传 |
| prune | 608B → 608B | 单个 main 没有可剪的东西,输出等于输入 | **GAP**:垃圾输入 rc0,原样透传 |
| lower | 608B → 2764B | 头部依次是 `@target lnx/x86_64`、`@data -`、`@src_os lnx`、`@bss 0`;有 `spinit rsp`,`imm r0,7` 落成 `rax` | **GAP**:垃圾输入 rc0,还会输出 1663B 启动代码 |
| elf | 2764B → 4096B | 运行后退出码 7;与产品 `-o` 逐字节相同(锚点) | 垃圾输入 → `an op outside the first encoder slice` |
| tapecheck | tape route → 592B | 输出 `_start:` 文本 tape,与 `-S` 同一路由 | 垃圾输入 → `unknown opcode` |
| units | u0.c+u1.c 两单元帧 424B → 500B | 两个单元按顺序合成一个 multi 输入 | 垃圾输入 → `malformed unit frame` |

e4、prune、lower 三段遇到不合法的输入时 rc0 并继续输出,坏输入要到 elf 才被拒。这是产品缺口,不是缺用例:产品补上拒绝之后再加 `bad`。
