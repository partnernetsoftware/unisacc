# 0.0.108 剩余清单(S0 入第一车之后;只列不做)

依据: plans/v0.0.108.md、build/version-seq.tsv 第 50 行、tests/diag-contract.tsv、research/ccogweco/v0108-pick.md、v0.0.108 分支日志(origin/main..v0.0.108 只剩 S0 系列与 c16 prep)。

## 已入第一车(不在剩余内)

- S0 表改 exec/parse2/forward-result.tsv;表级用例 check-forward-limit-table 与批次;覆盖/文档行(见 v0108-pick.md 第 1-4 步)。
- 未入车且不入: check-forward-count.cx 三档自红(8191/8192/8193 产品级不可达)与其夹具 gen_forward_count.cx;ff24782、b7449bb、6c3ae17。

## 剩余件(全部未入车)

| 记号 | 内容 | 合同行(tests/diag-contract.tsv) | 现状 | 备考 |
|---|---|---|---|---|
| c15 | 未定义函数前移编译期(undefined function helper) | c15 x1_undef.cx:4, KNOWN-GAP 到期 0.0.97 | 未开工:分支无对应提交 | 0.1.x 再做(R30) |
| c16 | no file 候选路径诊断(searched/package-root) | c16 k_use.cx:1、x3_modseg.cx:1, KNOWN-GAP 到期 0.0.97 | 仅准备提交 2644f21ff0(cx/cx0/plan.cx 增 searched 与 package-root/no-cxroot;投影与 cx0-three-source 重生成),提交自标 UNVERIFIED until 0.0.108 chain,未经 gatekeeper,不入任何车 | 0.1.x 再做(R30);若要复活,先过审并要求在新产品上实跑两条合同行 |
| c17 | does not export 带 use 行诊断 | c17 q_sym.cx:1, KNOWN-GAP 到期 0.0.97 | 未开工:分支无对应提交 | 0.1.x 再做(R30) |
| 覆盖警告 | cx file.cx -o OUT 误覆盖源文件诊断(-o 目标为已有 .cx/.ts/.文本时告警) | 无合同行(需新增 group) | 未开工:src/ 与 tools/ 无该警告实现,分支无提交 | 从 0.0.113 并入本版;0.1.x 再做(R30) |
| 验收 | tools/check-diag-contract.cx 合同夹具与 KNOWN-GAP 计数消减 | 当前 DC_GAP_BASELINE=15 | 未动:上面四件都没有把任何 KNOWN-GAP 转 PASS | 与 c15/c16/c17/-o 同批做,每转一行才降一次基线 |

## 登记状态

- 上述四件在 build/version-seq.tsv 第 50 行 0.0.108 行内,标注"0.1.x 再做(R30)";没有任何一件登记为新批次或覆盖行。
- v0.0.108 分支当前只剩 c16 prep(2644f21)未处置:它不在任何入车名单,建议冻结期后由编排者决定丢弃或转 0.1.x 时重做,不并入第一/第二车。
