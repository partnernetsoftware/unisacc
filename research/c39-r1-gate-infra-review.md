# R1 compilercheck 独立内容审核（只审不裁）

审核范围5d9a549b→28633d5e；七exec-driver套件。独立读取历史diff、gatedeps family/suite声明、compilercheck.sh/py及compiler.c。未执行七套件、未刷新审核戳。本结论供机房主任裁定，不自动结清R1。

闭包核对：七命令均不变；共同family compilercheck声明的直接文件/guard与该区间变更交集为空，reviewed src树唯一变更src/version.h。整个仓区间还变seed/gen.c等，不能说全仓只有两文件；这些额外文件不在本次所核compilercheck声明src/直接输入变化中。prd.md新增两段流程状态，不改验收条件；compilercheck执行代码未读prd。它的审核guard摘要变更应真实审核，但不能据此推出执行契约改写。

版本0.0.37→0.0.38确实改变--version输出、所编driver字节及source身份；不能说行为/工件字节完全不变。compiler.c从src/version.h打印版本，core-modes比较同源UA而非固定旧版本字符串。核心命令/断言/预算未变，重新生成同源参考是版本变更后的必要前提；旧二进制不能免验证。

| 套件 | 内容审核结论 | 理由与限定 |
|---|---|---|
| exec-driver-core-build | 契约未改 | 网络构建driver与UA构建driver逐字节比较、输出与同源UA比较的断言不变；版本改字节，但双方须同源重建。未本窗复跑。 |
| exec-driver-core-contracts | 契约未改 | compatibility/stdin/IO/undefined-function检查及输入不变；版本常量不改变这些断言。 |
| exec-driver-core-dependencies | 契约未改 | 实际include读取、MD/MMD/MF及依赖/token输出断言不变；版本头会改变构建输入身份，未改变依赖规则。 |
| exec-driver-core-modes | 契约未改，预期版本输出更新 | --version/-version仍rc0、无stderr、与同源UA输出相同；未硬编码0.0.37。E/S/b各优化级对照条件不变。 |
| exec-driver-language-1 | 契约未改 | 隔离包与语言分片输入/断言未变，版本仅同源构建常量。 |
| exec-driver-language-2 | 契约未改 | 同上，分片集合及检查条件未变。 |
| exec-driver-resources | 契约未改 | macros/headers/printf及隔离container检查不变，版本仍需进入重建工件身份。 |

UNKNOWN/不做的声称：未独立复跑真实七套件；无法凭内容审阅认PASS、证明外部工具/env未变化或所有未声明动态输入完整。此内容审核也不证明28633d5e之后当前main闭包不变。证据包gate-infra-recheck rc0仅证明检查器运行，并非先前已完成内容审核；本回执是本次新增独立内容审核。

建议交裁：在上述限定下，所核版本/状态变化未改七套件验收契约；审核人cdx2已读并逐项说明，可作为R1真实重审材料，由机房主任决定审核戳/归因结语。未改仓、不代裁、不跑重活。
