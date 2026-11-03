# 补丁 F 精确落点分析(2026-11-03, 经端到端实测修正)

补丁 F 目标(pp-negative):`#define 3x 1` 必须拒(macro name shall be an identifier),`#define FOO 1` 与 `#include <stdio.h>` 正常。

## 端到端实测证据(完整构造链, 本会话真跑)

| 补丁方案 | #define 3x | #define FOO 1 | #include <stdio.h> |
|---|---|---|---|
| 无补丁(主树) | 接受(缺陷) | 正常 | 正常 |
| 方案a(DB_WS 48-57→DEAD + DBB) | 拒 | **误伤拒** | (未测) |
| 我的两处(DEFFN+DB_WS 48-57→DEAD) | 拒 | **误伤拒** | **误伤拒** |

结论:单点 `48-57→DEAD` 无论加在 DB_WS 还是 DEFFN 都会误伤宏值/头文件名,因为 DB_WS 的 `*→MDEF` 是「对象宏定义整行」入口(MARK VS + BLOBSAVE BODY VS LE,LE=行尾,保存宏名+宏值整行),48-57 无法区分「宏名首字符」vs「宏值中的数字」。

## 真实根因(纠正之前的 DEFFN 误判)

- `define/macro $entry 1 → DEF0`(directive-action-result.tsv:23),即 #define 的 body 入口是 DEF0。
- `DEF0 40('(')→DEFFN`(函数宏 #define FOO(x)),`DEF0 其他→DB_WS`(对象宏 #define FOO 1)。
- `DB_WS 9,32(空格)→DB_WS`(跳过),`DB_WS *(其他)→MDEF`(MARK VS + BLOBSAVE BODY VS LE + PUSH DB_R)。
- 宏名由 MDEF 读 NID(idclass 含 48-57 数字),不校验首字符,所以 #define 3x 的 3 被接受。
- **宏值 '1' 也回流经过 DB_WS**(实测 #define FOO 空宏正常、FOO 1 误伤),所以 DB_WS 48-57→DEAD 会同时误伤宏值。

## 正确方案(需新状态, 不是单点补丁)

在「空格跳过」之后引入新状态区分「宏名首字符」vs「宏名后续字符+宏值」:
- 首字符是数字(48-57)→ DEAD(拒)。
- 首字符是字母/下划线 → 进宏名扫描(MDEF),扫描期间数字(48-57)是合法后续字符,放行。
- 宏值数字(宏名后的空白之后)→ 放行。

具体落点:把 DB_WS 的「空格跳过」与「宏名首字符判定」拆成两个状态,首字符判定只在「刚跳过空白、尚未读宏名首字符」时生效一次,读过首字符后进入宏名后续扫描状态(该状态 48-57 放行)。方案a 的 DBB 尝试了这个方向,但 DBB 的 `*→MDEF` 未区分宏值回流,导致误伤。

## 待办(下一纪元)

按上述新状态方案实现,端到端实测三用例全部正确(3x 拒、FOO 1 正常、include 正常)+ pp-negative 10/10 + smoke 16/0 + ratchets 9/9。
