# U4 裁定与 GCC 边界实测

`u4-plan.md` 是拆片原稿的逐字节副本。本页记录后续裁定，作为开工合同的更新。

(1) 汇编标签只支持 `__asm__` 与 `__asm`。声明位置用裸 `asm("name")` 时，建议沿用 U0 风格具名拒绝，例如 `not covered: bare asm label spelling; use __asm__ or __asm`。普通 C 标识符 `asm` 保持有效，补变量声明和读取正控。语句级汇编继续拒绝。

(2) osx/win 四格对合法形状的汇编标签统一具名拒绝，核心文案为 `asm labels are not supported on target <t>`，其中 t 是实际选择的目标。文案走事实表，`-c` 和 `-o` 各断言非零、无输出产物、正确目标与源码位置。对照正控在两个 lnx ELF 目标上走同一源码。

(3) 本机 GCC 11.4.0，边界以 `gcc -std=c99 -pedantic-errors -Wall -Wextra -c` 逐例实测。下面的 U4 预期未以 GCC 退出码自动重定基线。

| 样例 | GCC 观察 | U4 取舍 |
| --- | --- | --- |
| a05 不同标签重声明 | rc0，警告忽略后一个标签，定义符号仍为 a1 | 若维持具名拒绝，这是与 GCC 的有意差异 |
| a06 定义后才给标签 | rc0，警告忽略改名，定义符号仍为 f | 若维持具名拒绝，这是与 GCC 的有意差异 |
| a07 块内非 static 变量标签 | rc0，警告忽略标签 | U4 不支持变量改名，范围拒绝与 GCC 不同 |
| a08 非字符串 | rc1，expected string literal | 与 U4 拟定拒绝一致 |
| 块内 extern 函数标签 | rc0，对象实见 UND u4_block | 若把块作用域整体排除，这是第四处兼容差异 |
| 同标签重复声明 | rc0，定义符号 u4_same | 建议作为接受正控 |
| 标签直接连到函数定义体 | rc1，expected comma or semicolon before brace | a01 仍按先声明标签、后定义继承验收。直接贴定义体可补拒绝例 |
| 裸 asm | 严格 C 的变量正控 rc0，裸标签和裸语句各 rc1。GNU99 裸标签 rc0 | U4 取严格 C 边界，保留标识符，不扩展第三种拼写 |

实测日志保存在 `~/.claude/jobs/2798e440/tmp/u4-slices/gcc-boundaries/`。仓外设计目录 `a1a-design/` 和所列五个命名原稿均未找到，本次归档已找到的重建稿及此页。
