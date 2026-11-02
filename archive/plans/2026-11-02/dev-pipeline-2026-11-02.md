# 开发管道:用 omp-workflow 编排的 workflow + pipeline(2026-11-02)

## 原则(三条,其余皆由此推出)
1. **事实由命令产生,判断由模型产生**。构建/检查/字节比对/棘轮是命令节点,产出事实,不经模型转述;模型节点只做定位、评审、裁决,必须引用命令输出。
2. **闸门是可执行命令**。每个工作项开工前就写好验收命令;"完成"=命令在干净副本上输出约定结果,不是某个窗口说完成。
3. **改动先在私有副本,只有一个集成者碰 main**。实现节点产出 patch,不直接写共享检出。

## 工作项(唯一的输入单元,文件 `work/<id>.tsv` 一行一项,账本 `work/ledger.tsv` 只追加)
```
id  goal(一句)  domain(可写路径集)  gate(验收命令)  budget(token/分钟)  deps  owner-role
```
账本字段:`id state commit evidence(命令+输出哈希) 时间`。看板、todo、信封里的状态全部废止,状态只读账本。

## 节点类型
| 类型 | 谁执行 | 产出 |
|---|---|---|
| CMD | pipeline runner(`tools/pipeline.cx`,cx 实现,逐节点沿用 55/60s bound) | 事实:rc、输出、哈希 |
| JUDGE | `workflow` 调用(locator / reviewer / arbiter) | 固定 JSON 决策 |
| IMPL | `workflow` 调用(implementer,限 domain + gate) | patch(私有副本 diff) |
| INTEGRATE | 单一集成者(现在是我,以后是 runner) | main 上的提交 + 账本行 |

## 标准管道(一个工作项的生命周期)
```
INTAKE ──> LOCATE ──> [DECIDE]? ──> IMPL ──> VERIFY ──> REVIEW ──> INTEGRATE ──> RATCHET ──> DRY-EPOCH
 (写工作项)  JUDGE×k    JUDGE      IMPL      CMD        JUDGE       CMD          CMD        CMD(持续)
```
- **LOCATE**:k=2 个不同模型的 locator 并行,各给"落点文件/函数/一条验证命令";一致则直通,不一致交 DECIDE(arbiter)。落点必须带 `path:line`。
- **IMPL**:在 `git archive HEAD` 出的私有副本里改,只能写 domain;完成时必须自己跑 gate 一次并附输出(runner 不信它,VERIFY 重跑)。
- **VERIFY**:runner 在**全新副本**上应用 patch,依次跑:编译受影响 `.cx` → gate → `tools/affected.cx --run` → 字节等价回归(若声明)。**失败不进 REVIEW**:把失败输出作为上下文退回 IMPL,最多 2 轮,再败升级给 DECIDE 或标 BLOCKED 报调用者。
- **REVIEW**:reviewer 的输入=patch + VERIFY 输出;判据=是否越出 domain、是否有未验证声明、是否留下同类漏同步(grep 全部读写点)。reviewer 无权"批准",只能给 PASS/FAIL+证据;PASS 才进集成。
- **INTEGRATE**:应用 patch 到 main(`commit-own`,只带 domain)、写账本;提交说明由 runner 生成:`<id>: <goal>`。
- **RATCHET**:提交后立即 `tools/ratchets.cx`;红则自动还原该提交(新提交 revert,不用 reset)并回账本 BLOCKED。
- **DRY-EPOCH**:构造输入(tools/ exec/ cx/ src/ seed/)有新提交 → 触发 `tools/dry-epoch.cx`(私有根跑纪元 7-12 步);红则按"最近改动该文件的提交"回到其工作项,状态置 REOPENED。这一步专抓今天那类"构造链互相抄、纪元日才爆"的腐烂。

## 并发与冲突
- 读并行、写按 domain 串行:runner 持有 `work/locks.tsv`(domain→占用工作项);两个工作项 domain 相交则后者排队。
- 同一时刻 IMPL 并行上限 = 不相交 domain 数,且不超过机器可承受的构造负载(构造链运行期间 IMPL/VERIFY 暂停重 CPU 步骤)。

## 编排者(我)的职责收缩为
写工作项和 gate、回答 `needs`、裁定 BLOCKED;不再逐窗口转述、不再手工提交。人工介入点只有:gate 设计、BLOCKED 裁决、发布第 7 步。

## 防废话/防漂移的机制性约束
- 模型节点只能输出 workflow 的固定 JSON;长度有上限,INVALID 自动重试一次。
- 没有 gate 的工作项不能入库(INTAKE 拒收)。
- 工作项预算用尽即出结论;预算与产出(提交数/gate 通过数)按 角色×模型 统计,低产者降级或淘汰。
- 顶层路线只在 `work/roadmap.tsv`(阶段闸门),与账本对照自动生成"落后项"——今天"提速 0 进展"这类漂移会被一行报出。

## 用今天的事故检验这条管道
| 事故 | 管道里在哪一步被拦住 |
|---|---|
| 补丁 07 误拒全部预处理(只做了 `apply --check`) | VERIFY 的 gate/affected 必须执行;REVIEW 无执行证据即 FAIL |
| pack.cx / rebuildcontext 身份口径漏同步 | DRY-EPOCH 在提交后分钟级报红;REVIEW 要求 grep 全部读写点 |
| `tx_find` 少传参数(arity 缺陷) | VERIFY 编译期 arity 检查落地后拦截;此前由 LOCATE 的"一条验证命令"暴露 |
| `body[65]` 缓冲太小导致静默 rc=2 | VERIFY 的 gate 要求具名输出,静默失败=FAIL |
| 纪元 12 小时才发现 5 处腐烂 | DRY-EPOCH 持续运行 |
| 窗口上下文耗尽、同一问题多轮往返 | 工作项必须小到 IMPL 一次完成;装不下就在 INTAKE 被要求拆 |

## 落地顺序
1. `workflow` 工具 + `locator`/`reviewer`(见 omp-workflow-design)。
2. `tools/pipeline.cx` 骨架:读工作项、跑 CMD 节点、调用 workflow、写账本(先不并发)。
3. 把 B0(P3 explode/implode,见 booter-layering)作为**第一个真实工作项**跑通整条管道,gate=往返逐字节相同。
4. 接入 RATCHET 自动回滚与 DRY-EPOCH 触发;再放开并发。
