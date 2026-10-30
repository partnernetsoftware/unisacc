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

## 第 1 轮结果（2026-10-30）

- 实现：`tools/refactor.cx`（2c209dca54），0 个 include，0 处直接 libc 调用。规则文件 `tools/refactor/strcmp-eq.tsv`。
- 实践：`!strcmp` 改成 `tx_eq`，在 `check-source-frame.cx`、`check-stream-resource.cx` 两个文件里共改 12 处，JOBS=2。两个文件都 OK，自身运行的输出改前改后逐字节相同，墙钟 7 秒（user 9.4 秒，两个验证并行）（080927178b）。
- 失败路径：
  - 会改变语义的规则（`!tx_eq`）被判 REVERT，文件逐字节还原；
  - 文件漂移时判 SKIP；
  - 保留区（`cx/cx0/bundle.cx`）和缺软链（`out/rf-nolink`）的候选，find 都标成 skip。

## 发现

1. 能用强验证的文件太少，强验证指不带参数就能运行、改前改后输出逐字节可比。`tools/`、`tests/` 里这样的已登记批次只有 49 个，其中还有 `!strcmp` 的只剩 2 个。
2. 退一步只验证"能编译"也不行：已装产品的 `-c` 和 `-S` 不认转发给宿主 C 库的函数（`setpgid`、`clock_gettime` 等），报"undefined function"，只有 `-run` 能用。所以凡是间接用到 `cx_runv_timeout` 的文件，都没法单独编译检查。这是产品缺陷，已转给 grk。
   补记（2026-10-30，候选 `d903d225` 和已装产品都实跑过）：这是设计，不是缺声明。`-S`、以及不带 `-b` 的 `-c`，留在 tape，不建宿主桩，所以报 `undefined function`。单独编译检查用 `-c -b lnx/x86_64 -o OUT.o`。这样能编过，`setpgid` 在对象里是未定义符号；再拿这个对象去链接会报 `undefined reference`。能跑的程序仍从源码 `-o` 或 `-run`。见 plans/host-forward-compile-2026-10-30.md。

## 第 2 轮计划

- **验证来源：** apply 增加 `--verify-from tests/batches.tsv`，按文件去找登记它的批次，用批次登记的 entry 和 args 运行。批次输出里会带 `{out}` 这类每次都不同的路径，再加 `--mask REGEX`，比较前先把匹配的部分替换掉。
- **规则：** 增加"后缀条件"（例如 `strcmp(a,b) == 0`）和"参数类型提示"（只接受字面量或标识符），覆盖面更大，同时保持安全。
- **规模：** 在不同目录各选一批，单次 apply 控制在 55 秒内，用 JOBS 并行把墙钟压下来。

## 第 2 轮结果（2026-10-30）

- **新增 `--verify-from tests/batches.tsv`（cb3a800fa4）：** 按文件找到登记它的 SOURCE 批次，用登记的入口和参数作为它的检查，`{root}`、`{candidate}`、`{out}` 都会被替换。可验证的目标从 2 个扩大到 88 个。
- **抓到一个危险的假 OK，已修。**
  - 经过：`{out}` 的编号每次运行都从 1 开始，被测程序要求输出目录必须是新的，于是每次都以同样的 rc=2 拒绝运行。改前改后两次失败一模一样，被当成了"输出不变"。
  - 修法：
    - 输出目录名加上进程号，保证唯一；
    - 基线失败（rc≠0）一律判 SKIP；
    - 批次模式下，基线必须打印登记的 rc0 标记；
    - 基线同时跑两次，两次不一样的（例如输出里有耗时）判 SKIP；
    - 每个检查都套 `tools/bound 50`；
    - 改写前把原文件备份到 `out/refactor-verify/backup/`；
    - `REFACTOR_DEBUG` 环境变量会打印每次检查的命令和结果。
  - 误判为 OK 的两个文件已从备份还原。
- **实践：** 13 处，`tests/resources.cx` 和 `tools/formats-macho-pe.cx`（a77eb778a1）。对应的批次 `resources`、`ri-trace`、`formats-macho-pe` 已用 run-batch 复验，全部 PASS。`phase-timing-stamps`（检查不稳定）和 `negative-hostile`（改之前就失败）判 SKIP，没有改动。
- **规模：** 4 个文件、JOBS=4，墙钟 41 秒；user 时间 96 秒，说明确实在并行。每个文件要跑三次检查（两次基线加一次改后），所以一次调用只能带 2～4 个文件。

## 第 3 轮计划

- 增加 `restore` 子命令：按 backup 目录把被中途杀掉的运行恢复回去。
- 自动分批：plan 子命令按登记的 budget 把文件分组，保证每组在 55 秒内跑完，再一组一组调用 apply，每组完成后核对结果。
- 规则：再加 `strcmp(a, b) == 0` 和 `strlen(x)` 改成 `tx_len(x, CAP)`。后者只在结果用于比较或赋值给 long 时才算 safe。

## 第 3 轮结果（2026-10-30）

- **事故与修复：** 第一次扫描的 apply 在 58 秒时被外层 timeout 杀在半途，6 个文件改了一半，已从 backup 还原。之后 apply 加了内部 48 秒截止：到点不再开始新文件，剩下的报 `DEFER`；另加 `--range A N`，每次只处理候选里的第 A 到 A+N-1 个文件（7bc93735d9）。`restore` 子命令没做，backup 目录加手工还原已经够用。
- **扫描方式：** 在 `out/` 下写了一个薄驱动循环，每轮都是有界调用，最多 30 轮：
  1. 对还含 `!strcmp(` 的干净目标重新 `find --auto`；
  2. 对前 12 个文件跑 `apply 4 --range 0 12`；
  3. 检查命令是 `-c -b lnx/x86_64 {file} -o /dev/null`。
  目标是 223 个 tests/tools 文件，开工时已经 dirty 的文件排除在外。
- **结果：** 138 个文件改动，0 个 REVERT。分两批按路径提交：tests 79 个（d51282b23c），tools 59 个（c2b3d5bf26）。
  - 差异审查：每个改动行都只涉及 `strcmp` / `tx_eq(` / `#pragma unisa use text.text`，共 +688/−550，多出的 138 行正好是每个文件新增的一行 use。
  - 棘轮检查：cxraw ok。cxapis、cxtrunc、cxinclude 的超额全部在 apps/csih，与这 138 个文件没有交集。
- **SKIP（改之前编译就失败）：** tests/nets.cx、tools/cx0.cx、cx0-bundle.cx、cx-driver.cx、cx-pipeline.cx、check-cx-entry.cx、hostseed-step.cx、hostseed-step-nativefirst.cx。这些文件没有改动，需要单独排查编译失败的原因。
- **瓶颈：** 一轮 12 个文件，平均只完成 4～7 个；有 4 轮是 ok=0、defer=12，白跑。原因是每个文件要编译 3 次（两次基线、一次改后），单次 5～10 秒，而且在 4 个并发下会互相拖慢。30 轮共 138 个文件，墙钟约 15 分钟。
- **第 4 轮改进候选：**
  - 基线结果按（检查命令、文件 sha、编译器身份）缓存，只缓存改前基线，改后验证每次实跑；第二次基线也可以跳过。这算"有测量依据的热点缓存"，预计每个文件少 2 次编译。
  - grk 在 L0 新增 `tx_eqz` 并提交后，用一条规则 `tx_eq($1, $2, 1L << 20)` → `tx_eqz($1, $2)` 把这次扫描的结果折叠掉。这条规则本身也是对工具的二次实践。

## 第 4 轮结果（2026-10-30）

- **改动：** 剩下的 92 个目标又改了 17 个文件（a7bc3fc5cb），0 个 REVERT。之后 `find --auto` 已经没有可自动接受的行。
- **未改的 341 处，按原因分：**
  - 289 处判 skip，原因是文件旁边没有 `text/` 链接：
    - tools/cxlib 204 处（都是模块）；
    - tests/preprocess 45 处、tests/cases 34 处，都是材料或夹具目录，不动；
    - 其余 6 处。
  - 52 处判 review，原因是模块文件会因此新增 use。
- **review 判定是对的，已用实验证实：**
  - 已装产品在同一模块经两条不同的链接路径进入时，会报 `type TX_ZCAP from text.text and text.text is defined twice`，rc=1。例子：consumer 经 `X/text` 引入，而它 use 的模块经 `X/sub/text` 引入。
  - 同一条链接路径的菱形依赖没有问题。
  - grk 的候选 `out/grk-modid/candidate`（d903d225）同样输入能通过，输出 `1 1`，rc=0。
  - 最小复现在 `out/rf-dedupe/`。
- **第 5 轮的前置条件：** 去重修复进入已装产品，由慢车道安装。之后：
  1. 加 `tools/cxlib/text -> ../../cx/text` 链接；
  2. apply 增加 `{users}` 占位：模块文件改为编译它的全部消费者，因为单独编译模块会报 `no main`；
  3. 再放行 review 行。
- **另外 8 个基线编译失败的文件，都是 `-c -b` 检查本身覆盖不到：**
  - tests/nets.cx 依赖的 netcore 在材料准备时才放到它旁边；
  - tools/cx0.cx、tools/check-cx-entry.cx 报 `cc interop call signature` 未覆盖；
  - tools/hostseed-step.cx 报 `tape domain: unknown opcode`。
  这些文件改走 `--verify-from tests/batches.tsv`。
