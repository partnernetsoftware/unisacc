# NEG · CI 诊断门控（与实现同刀）

| case | 操作 | 期望 |
|---|---|---|
| N1 门控关 | unset UNISACC_DIAG_OUT；跑 bootstrap | rc/判定与改前一致；无 diag 目录创建 |
| N2 门控开·成功 | UNISACC_DIAG_OUT=DIR；bootstrap 绿 | DIR 可有 copy；rc=0 不变 |
| N3 门控开·失败 | 同 N2 但 B 支失败（若可注入） | DIR 含 B.log 等；suite 仍非 0 |
| N4 artifact 步 | ci fragment 在 linux 或 success-only | upload 条件 failure&&macOS → 不误传 linux 包 |

NEG 红 ≠ 产品 PASS。
