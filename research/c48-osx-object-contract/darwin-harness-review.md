# X5 Darwin arm64 harness 最小追加支持审查

结论：可只在 `tests/run-batch.cx` 引入已有 `sys.hostid/tn_self()`，保留原lnx/x86_64字面路径，再仅接受真实宿主为osx/arm64且selected.platform与该宿主相同的行。无需上层平台#if，不接受win或把Rosetta的x64目标算作Intel真机。精确可审补丁：`out/c48-darwin-harness/additive-hostid.patch`；本轮只写out，main未改。

核心逻辑：

```c
#pragma unisa use sys.hostid
static int batch_platform_allowed(const char *p){
 if(tx_eqz(p,"lnx/x86_64"))return 1;
 const char *host=tn_self();
 return host&&tx_eqz(host,"osx/arm64")&&tx_eqz(p,host);
}
```

将现有唯一 `if(!tx_eqz(selected.platform,"lnx/x86_64"))die(...)` 条件换为该函数，其余参数、材料、预算、输出排他、候选前后SHA及PASS/UNKNOWN/FAIL判定不变。tn_self未知时新Darwin路径拒绝；原lnx字面放行行为不变。本轮初稿曾按请求先以平台宏实现私有版本，但收到父要求后已替换为hostid；实际下述probe使用最终hostid二进制，不将旧宏编译当验收。

## 实际私有验证（已完成）

最终私有源码 `out/c48-darwin-harness/run-batch.cx` 经已装产品实编session32252 rc0；外bound40。父源码未被改。真实小fixture使用复制的完整f069产品、同一有效覆盖映射、四条12秒预算注册行及会打印PLATFORM_TINY的实际.cx程序。probe默认-run session48262实际exit0，外bound55、每次harness调用内bound15。

| 实际调用 | 退出 | 观察 |
|---|---:|---|
| 原正式harness + osx/arm64行 |2|具名another native host拒绝，复现原门缺口|
| 最终私有harness + osx/arm64行 |0|实际编译运行tiny，唯一BATCH_RUN PASS/rc0/transport0|
| 最终私有harness + lnx/x86_64行 |0|原字面路线仍实际PASS|
| 最终私有harness + win/arm64行 |2|another native host拒绝|
| 最终私有harness + osx/x86_64行 |2|another native host拒绝；未贴Intel原生资格|

两次PASS的candidate都为f069d4c92b7f5ede508809aa3abd21e326cbecadb2fb9eccf0bd1ac7004c3e82。日志 `out/c48-darwin-harness/{probe.log,probe.stderr}`。Darwin子subreaper不可用的既有诊断保留，没有改清理合同。没有重建产品、运行逻辑代理的objects文件、或执行六平台资格。

## 应修改与验证的现有入口

1. 正式最小源码只需 `tests/run-batch.cx`。本机实际执行的两个osx目标矩阵行均应登记platform=osx/arm64，目标ISA由args/materials表达；x86_64目标实际通过Rosetta运行时另明确translation=Rosetta，不登记成osx/x86_64原生Intel宿主。
2. 新增一个独立harness平台检查，保留本轮五个实际控制；若注册成批次先查新名历史。无需改现有Linuxfixture默认平台，不能把全部fixture批量换Darwin掩盖旧合同变化。
3. `tests/runner-faults.cx`（已有runner-faults正式55秒批）应对集成harness重跑：保留ok、超时UNKNOWN、洪泛、子孙清理、crash FAIL全部断言。它的私有注册行为lnx标签继续用于旧路线回归；可额外单独Darwin正例，不删原断言。此轮未运行该完整检查，不宣称绿。
4. `tests/typed-materials.cx`：prepare/verify和实际wave/split两行合同不必为此修改；原lnx标签/类型/绑定拒绝应保留。若已有同输入有效fixture/session，复用材料并用新实编runner执行verify；否则按文件头分相位真正prepare→调度/运行→verify，不能只运行verify或伪造session。每步bound55。至少保留sha256/path类型、缺参/短hex/路径冒充、ENOENT/ENOTDIR与literal不被当路径的真实拒绝。可新增Darwin行绑定同材料作追加验证。本轮未实际重跑这些材料相位。
5. 注册后 `tests/run.cx -- --list` 核元数据，再跑所新增Darwin矩阵批。tests/run的smoke本来明确platform_claim=NOT_QUALIFIED，不能以其绿替代新矩阵执行。

## 另一个明确边界

`tools/all-waves.cx:243/355` 还有两处只接受lnx/x86_64的调度门。单独修改run-batch足以执行直接正式Darwin批次，但**不等于all-waves调度已经支持Darwin**。若X5要从该调度器执行Darwin行，需父另分配同样hostid追加逻辑及其独立控制；本轮没有改它、没有扩大为六平台。`tools/cxlib/allsession.cx` 记录平台字段，本轮未见其同类放行门。

所有本轮实际句柄已终态。精确补丁只包含run-batch的hostid导入、一个小帮手及唯一条件替换；无上层#if债、无Linux资格迁移、无主树变更。
