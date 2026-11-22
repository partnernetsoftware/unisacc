# Darwin harness 增量检查

结论：新增 `tests/runner-platform.cx` 默认-run 实编并实际执行，17/17 断言通过；源码冻结、未提交。已有 runner-faults 正式批仍有 5 个失败，不能宣称整个 harness 通过。

新检查无 out 材料依赖，argc 为 0 或 1（可选仓根）。建议登记 interfaces / osx/arm64 / daily / 55 秒，marker `RUNNER_PLATFORM`。内部每个子进程最多 15 秒，整体内部 deadline 45 秒。非 osx/arm64 宿主明确 NOT_RUN、退出 3。

检查实编当前 run-batch，临时 fixture 完整复制并绑定当前产品。真实验证 Darwin arm64 标签与既有 lnx/x86_64 标签均执行通过；win/arm64、osx/x86_64、win/x86_64 拒绝且不创建批次输出；核产品未改变。

产品 SHA256：f069d4c92b7f5ede508809aa3abd21e326cbecadb2fb9eccf0bd1ac7004c3e82。

实际终态：session 93158 exit 0。日志 `runner-platform-check.log`、`runner-platform-check.stderr`，末行 assertions=17 failed=0。保留 fixture `/private/tmp/runner-platform-onesjf`。

## 既有 runner-faults

父最终 harness `out/c48-x5-formal/run-batch` 执行正式 runner-faults，session 52245 实际 exit 1，assertions=7 failed=5。control 与 descendant-cleanup 两项通过；timeout UNKNOWN、flood UNKNOWN、flood walltime、descendant UNKNOWN、signal 命名五项失败。完整输出保留 `runner-faults-formal.log` 与 `.stderr`。未改原断言、fixture 或预算。

对同保留 fixture，使用旧 harness `out/c48-no-line-limits/integrated/run-batch` 和父最终 harness 分别运行四个失败作业，输出 parent 独立，均实际终态 exit 1：

| 作业 | 旧句柄 | 新句柄 | 旧、新共同结果 |
|---|---:|---:|---|
| sleeper | 96419 | 67865 | FAIL rc=0 transport=-1，stderr 明确 2 秒 TimeoutExpired |
| chatty | 51649 | 38081 | FAIL rc=0 transport=-1，stderr 明确 2 秒 TimeoutExpired |
| grand | 99151 | 3946 | FAIL rc=0 transport=-1，stderr 明确 2 秒 TimeoutExpired |
| crash | exec 直接 exit1 | 1132 | FAIL rc=139 transport=0；原断言期望 rc=-11 |

对照日志为 `faults-{sleeper,chatty,grand,crash}-{legacy,diagnostic}.{log,stderr}`。每次外 bound15，fixture 原批次预算保持 2/5 秒。所有 stderr 均有 no child-subreaper。此处实际发生 timeout，而非源码编译失败。旧、新共同的 timeout 分类与 shell signal 返回码问题已存在于旧 Darwin 路径；本次平台许可补丁未引入这些观测差异。未记录每个直接对照作业的独立墙时，故不声称已对 flood 的墙时失败完成前后定量归因；具体 timeout residue/清理原因也未插桩证明。

本次只新增 runner-platform 源码与 out 证据；未修改 runner-faults、run-batch、注册表、PRD 或 typed-materials，未扩大平台接受范围。
