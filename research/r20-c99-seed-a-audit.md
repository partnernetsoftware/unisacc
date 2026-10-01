# R20-1 A：旧 C 构造器复测（2026-10-02）

范围是 `iterate/construct/construct.c` 和 `iterate/kernel/genmodel.c`；二者都是旧 18 表 UNS2 / 经典内核的开发工具。此处没有验证当前 δ 网络、P3 包或 APE seed。

| 输入与命令 | 结果 | 解释 |
|---|---|---|
| `STAGES=prec GLOBAL=0 python3 tests/bound.py 58 ./iterate/construct/check.sh ./unisacc.com` | rc 0；`prec` 的文本 855 B、UNS2 出货段 83 B 与 Python 相同；19 键全域 roundtrip；cc/unisacc/UBSan 均通过 | 单表构造、旧包编码与验证器可复用 |
| `python3 tests/bound.py 58 ./iterate/construct/check.sh ./unisacc.com` | rc 142；超时前已见 `abi` 决策规则超过 96，`isel` 的 `symbol` 类别 107 超过 96，`combo` 与 Python 的组/规则不同 | 整轮既不满足时限，也不满足当前表；未观察到的项目不能记通过 |
| `CHECKS='order build region' python3 tests/bound.py 58 ./iterate/kernel/check.sh ./unisacc.com` | rc 1，`lexcls.tsv` 的字段值 257 超过 `genmodel.c` 的 256 上限 | 现行输入闭包已超出旧内核工具的固定容量 |

`construct.c` 的界限包括 `MAXF=3`、`MAXV=128`、`MAXC=96`、`MAXR=96`、`MAXU=200`。`genmodel.c` 固定 `NST=18`、`MAXV=256`，输出的是 `kernel/unisa_model.inc` 的经典段。两者的 TSV 解析、有限规则构造、UNS2 写出及按名核对可以作为 C99 新实现的参考；固定容量和旧格式不能直接作为 P3 的数据契约。

下一片先清点当前 `exec/` 的 δ 生成器、包组装与 APE seed 的输入闭包和输出格式，再选一个完整 δ 做 C99 字节对拍。保持 Python 默认构造路径，直到 A–D 的验收分别成立。
