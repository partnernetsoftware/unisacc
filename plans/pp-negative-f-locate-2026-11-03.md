# 补丁 F 精确落点分析(纠正之前 ledger 根因, 2026-11-03)

补丁 F 目标(pp-negative):`#define 3x 1` 必须拒(macro name shall be an identifier, C99 6.10.3),`#define FOO 1` 与 `#include <stdio.h>` 正常。之前方案(a)改 `DB_WS` 误伤宏值,已回滚记 BLOCKED。本文件给出纠正后的根因与精确落点,供下一纪元重做。

## 铁证(展开后状态机 gen-shared-e2/result.json, 非纯静态推理)

- `DEF0`: 40(空格)→`DEFFN`,其余所有字节→`DB_WS`。
- `DEFFN`: **所有字节(含 48-57 数字)→`MDEF`**,无首字符校验。
- `DP0`: **48-57(数字)→`DEAD`**(已有正确拒数字逻辑),65-90/95/97-122(字母/下划线)→`DPID`,空格→`DP0`。
- `DB_WS`: 9,32→`DB_WS`,其余→`MDEF`。全状态机里**只有 `DEF0` 转到 `DB_WS`**(无宏值回流 DB_WS)。
- `MDEF` → `MD1`(macro-byte/macro-result):宏名重定义检测,非字符循环;宏名字符扫描是 `@name` 操作,只认 identifier class(数字也匹配),不校验首字符。

## 根因(纠正)

`DEFFN` 对宏名首字符不做合法性校验,直接进 `MDEF`(由 `@name` 读整个宏名,而 `@name` 只认 identifier class、数字 `3` 也匹配),所以 `#define 3x 1` 的 `3` 被当合法宏名首字符接受。

之前 ledger 记的「DB_WS 是宏名扫描循环、空格后无法区分宏名首字符 vs 宏值」**不准确**:DB_WS 只在 `DEF0`(非空格,即 `#define` 直接跟字符无空格)时进入,宏值(`#define FOO 1` 的 `1`)走 `DP0→DBF→HSCAN`,不经过 DB_WS。

## 精确落点

在 `exec/pp/directive-body-byte.tsv` 的 `DEFFN` 状态加一行,让数字首字符直接报错(复用 `DP0` 已有的数字拒绝动作):

```
DEFFN	48-57	DEAD	[["REJECT","macro name must be an identifier"]]
```

或复用 `DP0` 的报错动作(`[["LDI","t",2],["STX","EA",["constant","F_FN"],"t"],["JUMP","LS"]]`,F_FN=2 标记错误)。

## 为什么这个落点不误伤宏值

- `#define FOO 1`:宏名 `FOO` 走 `DEFFN→MDEF`(首字符 `F` 是字母,不匹配 48-57,走 `*→MDEF` 不变),宏值 `1` 走 `DP0→DBF→HSCAN`,不经 `DEFFN`,所以不加行也安全、加行后更安全。
- `#define 3x 1`:宏名首字符 `3`(51)匹配新增的 `48-57→DEAD`,直接拒。
- `#define FOO(x) 1`:函数宏名 `FOO` 首字符 `F` 正常,括号走 `DP0`(已正确),不受影响。

## 待实测(下一纪元)

改表后走完整构造链(seed/gen.c 重新生成 pp 表 → net 构造 → 执行器),实测:
1. `#define 3x 1` → reject(`macro name must be an identifier`)。
2. `#define FOO 1` → rc=0 正常。
3. `#define FOO(x) 1` → 正常。
4. `#include <stdio.h>` → 正常。
5. pp-negative 10/10、smoke 16/0、ratchets 9/9。

## 之前方案(a)失败原因

方案(a)改 `DB_WS 48-57→DEAD` + 引入 `DBB` 状态。展开后状态机证明 DB_WS 只有 DEF0 进入、无宏值回流,改 DB_WS 本不该误伤 `#define FOO 1`;之前实测「#define FOO 1→reject」应是 `DBB` 状态或别的改动引入,而非 DB_WS 本身。正确落点是 `DEFFN`(宏名首字符),不是 `DB_WS`(空宏定义/无空格入口)。
