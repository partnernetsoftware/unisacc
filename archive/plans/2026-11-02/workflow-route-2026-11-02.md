# 工作流与推进路线(2026-11-02;主人诊断:缺好工作流、缺好路线)

## 证据(今天实际发生的,不是感想)
- 纪元当天才暴露的错误:pack.cx 身份口径漏同步(3 处)、rebuildcontext rb_record 口径漏同步(2 层)、补丁 07 误拒全部预处理、release-build 要 footer。全是"构造链上相互抄的代码、没有被持续运行"的腐烂,发现点却在慢车道纪元里,一次耗数小时。
- 93 个提交里 44 个纯文档;顶层思维树(prd 9.1)停在 11-01,各窗口各干各的;今天 0.0.44 第一优先级(提速)0 进展。
- 代理上下文反复耗尽,同一问题多轮往返(tx_find 参数、body[65] 缓冲)。

## 工作流修正(可执行,逐条能验)
1. **慢车道持续预演**:写 `tools/dry-epoch.cx`——对当前 HEAD 在私有根里跑 runbook 第 7-12 步(已有 bound),只在构造输入(tools/ exec/ cx/ src/ seed/)有提交后触发;红了立刻回到该提交的作者。目标:腐烂在提交后分钟级暴露,不再等纪元日。
2. **同一口径只有一处**:身份/canonical 收敛(plans/identity-unify)优先于新功能;在此之前,凡改身份口径必须 grep 全部读写点并附对照表(已写入审计要求)。
3. **补丁/改动 = 执行验证,不是 apply --check**:提交请求必须带"执行了什么、输出什么";没有输出的请求退回。
4. **任务单元收小**:派活信封 = 一个文件域 + 一个可执行验收命令;>1 个窗口上下文装不下的任务拆开或交子代理(已用 ElfTextExtractor 验证有效)。
5. **文档不单独提交**(AGENTS.md 已改);思维树每个里程碑更新一次,不每轮。

## 推进路线(按 0.0.44 主人定序,每项有闸门)
| 阶段 | 内容 | 闸门(可测) |
|---|---|---|
| R0 | 收完本纪元(步12-14)+ 提交合并 | 两代固定点、--all rc0、ratchets 9/9 |
| R1 | dry-epoch 工具 + 身份口径单一来源(identity-unify 第 1-3 步) | 改一个构造输入后 10 分钟内 dry-epoch 出结论 |
| R2 | C44-SPEED-01:补测 67 对象 → 先砍 lower 重放 | 同输入整程序 -o 五片 ≤40s(基线 drive.cx 8.14s) |
| R3 | arity 检查 + F(pp-negative) 重做(同一次纪元,P3 压缩口径 B 同窗) | 复现夹具转拒绝;pp-negative 10/10 |
| R4 | FFI arm64 CI + W6 余项 | CI arm64 四向量绿 |

R2 在 R0 完成后立即开始,不等 R1(提速分析与 dry-epoch 互不依赖,可并行)。

## 暂停快照(2026-11-02,omp-go)

未提交:tools/speed-probe.cx、tools/check-speed-probe.cx、tests/batches.tsv +2 行、tests/coverage.tsv +1 行、
tools/check-cx-layers.cx(2 行 fixture 路径)、plans/{shrink,speed-hotspots,ffi-bridge-image,layering-refactor} 文档、
out/dry-epoch/dry-epoch.cx(草稿,不入库)、out/ 下缓存与夹具。未完成:dry-epoch 接线与全量试跑、67 对象补测。
回滚点:HEAD 未动;任何未提交改动都在工作区,delete 即回退。位置:无进程在跑。
