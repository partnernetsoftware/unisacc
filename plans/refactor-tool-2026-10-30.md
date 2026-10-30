# 快速重构工具 `tools/refactor.cx`（2026-10-30，cc）

主人的设想：分三步，找出可能要重构的地方，确认要改哪些，再并发批量修改。目标是把一半左右的机械工作量一次做完。

## 依据

今天手工做的几批改动（`!strcmp` 改成 `tx_eq`、去掉前导里已有的 include、给 memcpy 加 `cx_kept`），流程都是一样的：

1. 扫描器找出候选；
2. 人工确认写法是同一种；
3. 逐处改写；
4. 改前改后输出逐字节对比。

耗时的是第 2 步和第 4 步，所以工具的重点是两件事：把"确认"写成规则，把"验证并回退"做成自动。

## 三个子命令

```
sh ./unisacc.com tools/refactor.cx -- find RULES FILE...        > CANDIDATES.tsv
sh ./unisacc.com tools/refactor.cx -- apply CANDIDATES.tsv JOBS VERIFY...
```

- **find**：按词法扫描，跳过注释、字符串和字符字面量，按规则匹配函数调用并解析参数（括号嵌套到第 0 层的逗号）。每个候选一行：`file  offset  line  rule  class  original  replacement  use`。
  - class 分 `safe`、`review`、`skip`，并写明理由。下面几种情况降级为 `review` 或 `skip`：
    - 匹配跨行；
    - 参数里有宏、字符串或注释；
    - 文件在保留区；
    - 需要 use 某个模块，但文件旁边没有对应的软链。
- **确认**：编辑 CANDIDATES.tsv 即可。apply 只处理第一列为 `accept` 的行。`find --auto` 会把 `safe` 的行预填成 `accept`。
- **apply**：按文件分组，最多 JOBS 个文件并发处理。每个文件的流程：
  1. 跑 VERIFY 命令（`{file}` 会替换成文件路径），记下 stdout、stderr 和退出码，作为基线；
  2. 断言每处原文仍在记录的偏移上。不在就跳过整个文件并报告，防止和别人的改动撞车；
  3. 从后往前改写，需要时补上 `#pragma unisa use`；
  4. 再跑一次 VERIFY，三项必须逐字节相同；不同就把文件还原成原字节；
  5. 最后报告每个文件的结果：OK、SKIP 或 REVERT，以及改了几处。

## 规则文件（TSV）

`id  call  prefix  argc  template  use  class`

- `call`：被匹配的函数名。
- `prefix`：调用前必须紧挨着的字符，比如 `!`。没有就写 `-`。前缀会和调用一起被替换。
- `argc`：要求的顶层参数个数。
- `template`：改写结果，用 `$1`…`$9` 引用参数。
- `use`：需要的模块，没有就写 `-`。
- `class`：规则本身的安全级别。

## 保留区

`ujs/`、`apps/csih/`、`archive/`、`kernel/`，以及候选输入 `src/`、`cx/`。前四个由主人或其他车道负责，或者是生成物；后两个是 grk 正在编的候选。find 一律把这些路径记为 `skip`。

## 验收（实践）

- 第一批规则：`!strcmp(a, b)` 改成 `tx_eq(a, b, 1L << 20)`，需要 `text.text`。
- 跑在 `tools/`、`tests/` 里已登记成批次的检查文件上，VERIFY 用它们自己的运行命令。
- 要求：OK 的文件改前改后输出逐字节相同；断言失败的文件被跳过；故意构造一个会失败的改写，要确认能回退。
- 结果记在本文末尾。
