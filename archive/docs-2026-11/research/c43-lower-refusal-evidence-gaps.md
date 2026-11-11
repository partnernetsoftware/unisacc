# lower 拒绝域：现有检查的证据缺口（只读对照，2026-10-14）

性质：只读核对源码与现有检查脚本，没有新跑模型或参考；不改产品、不入 gatedeps、不裁 A/B。
对照对象：`research/c43-lower-refusal-domain-proposal.md` 的族 4（目标能力）与库绑定拒绝。

| 拒绝点 | 参考位置 | 现有检查怎么验 | 缺口 |
|---|---|---|---|
| 目标无对应导入（Windows） | `unisa/lower.py:232` | `exec/lower/fullcheck.py:104–114`：参考抛出的消息逐字比对；两个执行器 rc1、stdout 空、stderr 以 `reject: not covered:` 开头 | 无。这是三项齐全的样板 |
| 目标无系统调用号（非 Windows） | `unisa/lower.py:244` | 未找到按此消息验的检查 | 未核实是否有负例；待查 |
| 栈上传参 | `unisa/lower.py:266–268` | 未找到负例 | 同上 |
| `.hostaddr` 索引越界（不在 0..3） | `unisa/lower.py:327–329` | `exec/lower/libraryexitcheck.py:64` 用索引 0，`tests/unisatapecheck.sh:90` 用索引 2，都是合法值 | **没有任何检查喂非法索引**。模型对 `.hostaddr r0, 4` 是拒绝还是接受，没有实证 |
| 库绑定格式非法 | 库模式绑定资源 | `exec/lower/libraryimportscheck.py:39–47` 的 `check()`：拒绝时只断言 rc1 | **不验 stdout 为空，也不验 stderr 的具名原因**。拒绝却写出部分输出、或以别的原因拒绝，都能通过 |

## 建议（只是设计，待授权后落在 tests/）

1. 非法 `.hostaddr` 索引补一条负例，按 `fullcheck.py:104–114` 的三项形式：参考消息逐字、rc1、stdout 空、具名 stderr。若模型接受，它与 NEG-001 同类，一并登记待裁，不当场修。
2. `libraryimportscheck.py` 的拒绝断言补上“stdout 空 + 具名 stderr”。该文件在产品源闭包之外（`*check.py` 被排除），改它不变候选身份；但它是现有 gate 套件的一部分，改断言可能把现在的绿变红，所以要先得到授权。
3. `lower.py:244`、`:266–268` 两处先查清有无负例，再决定是否补。

未做：cdx2 的四族索引原文我没有拿到文件，本表按巡检转述的位置（`lower.py:232/325`、fullcheck、sparsecheck、libraryimportscheck）自行核对；`sparsecheck.py:34` 的溢出拒绝已是三项齐全（rc1、stdout 空、stderr 含 `tape data directive`）。
