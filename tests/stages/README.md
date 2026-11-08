# 分段字节流用例(ST4)

检查器:`sh ./unisacc.com tests/check-stages.cx`,批次 `stages`。每段一个目录:`args`、`in`、`expect`,可选 `bad`、`bad.err`、`bad.args`。
`args` 里写 `--source a.c` 这类相对标签,运行时 cwd 是该段的目录。
链式约束:image 管道 e2→e1→e3→sidecar→prune→lower→elf 中,每段的 `in` 等于上一段的 `expect`,检查器逐字节核对。
锚点:elf/expect 与 `unisacc -o` 编 e2/a.c 的产物逐字节相同。
一个段没有拒绝用例就记 GAP(uncovered),检查器 rc 非零,这个段不算通过。

拒绝用例文件:`bad`(主用例)、`bad.trunc`、`bad.magic`、`bad.rand` 各配同名 `.err`;
四者都必须 rc1 且 stderr 与该 `.err` 逐字节相同。

## 行尾截断负控

`trunc-at-eol` 保留历史上截在**换行处**的真实 tape 前缀，不补 `.end`。
检查器现在要求 rc1、stdout 为空、stderr 包含 `reject: tape truncated`，
不再记为 GAP。这些固定材料不能跟随正例重生而补尾，否则会丢掉负控。

历史全量实测曾在 10670 个行尾截断点里得到 3 个不能运行的产物；
该记录不是新产品验收结果。正例需要完整终止帧，缺尾、短读及终止后
多余内容均不能以默认值或隐式补尾修复后接受。

`sidecar` 在既有 USLFW1 外层长度拒绝之后，调用共享 `compiler.tapegate`
文本帧机制独立校验声明的 tape body；不依赖后接 prune 才拒绝缺尾或非法帧。
`check-stages.cx` 用准确长度的独立 USLFW1 材料检查完整 body、空名切换、
缺尾、短 end、早 end 后内容、短 HEX 和非法 unit 编号；外层错误优先级不变。

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

e4、prune、lower 三段曾遇到不合法输入 rc0 并继续输出；上表保留当时实测，
不是新候选的运行证据。当前源合同由 `exec/tapeframe/frame-{byte,result}.tsv`
统一校验完整终止与定位帧，阶段原入口的格式检查仍然保留。
缺尾先检查最后五字节快拒；存在尾部后再完整扫描，拒绝早先 `.end` 后的内容、
非法 `.unit` 边界。同一 TU 的重复帧可切换逻辑文件名；不对截断输入补尾。
