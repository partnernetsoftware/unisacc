# 0.0.91 计划草案:身份与构建边界(L.1 + Z5 + B3,产品车道)

说明:仓内 plans/v0.0.91.md 已有 29 行占位(2026-11-21 版本序调整提交 7a75c62553),本草案是开版评审版,待编排者拍板后合并替换。注意 version-seq 已把 0.0.91 定为"身份与构建边界",本草案不推翻该裁定。

planned_units=5
a_class=身份与构建边界
a_files_cap=20
freeze_at=2026-11-30T12:00(沿用占位)

## 开版评审(待编排者拍板)

依据:build/version-seq.tsv 第 26 行;build/owner-reqs.tsv R60/R70/R72-R81;prd_mempalace.md 0.0.90 发布交接节;plans/v0.0.90.md 已知缺口 (1)-(5);csr/tsv-lint-F、zero-assert、mut-audit、flow-next/r71-design.md。

## 一、范围候选与归类

### A 类候选(进候选键,争名额)
- (A1) 身份与构建边界:L.1 source-packages-v3 + Z5 删 cx/text 四链接/迁 ext + B3(名单待核)。占位估 17-19/20 文件。类别 = 身份与构建。
- (A2) tsv-lint F 类补静态绑定(R75):exec/parse2 下 modelsignature、modelcandidates、unarycontrol、callcontrol 4 表补 NAME=@str:NAME(libraryimports 48/48 为 runtime fresh: 不改),约 4-6 个 exec/ 表加派生表。csr 实测按条 11277 -> 10704 -> 10629(上限 11050,余量 421)。类别 = 事实表整理。
- (A3) U4 收尾:volatile 内联 asm 拒绝补 file:line:col,需 exec/lex/attr-byte.tsv 1 文件(第 1346 行 HALT REJECT),使 diag-contract c10 之外的 volatile 位置缺口闭合。类别 = 事实表整理。
- (A4) R70 热启动去 rk_replan 兜底(cx/runtime/rcident.cx 253-260,431-457,4-6 文件):省约 10-17ms。类别 = 运行时缓存。守门人有条件批准(owner-reqs 记 9 条件:搜索根与 cxroot 入键、跟随 stat 含 ino+ctime、每 16 次抽样 replan 且 CI 全量对拍、8 变异先红后绿、不可检出时钟回拨与 TOCTOU 等),可信度约 70%,收益未实测。version-seq 现无其槽,owner-reqs 记 0.0.93。

### 非 A 类并行车道(main 上,不占候选键)
- R76 ext/net HTTP 服务端(B 类,ccogw 在 moltbaby 仓迁移与验收,四门:六目标实编、layer-api 同步、affected 选批、上一公开 APE 兼容实编)。
- R78 变异门全量 212 项分片重放(0.0.90 起链窗口外,工具已落库)。
- R69.6 零断言守卫:先离线影子阶段(zero-assert/plan.md,首次启用拒绝数应为 0,先实测分母),C 类,单独守门人评审。
- R79 释放后读(UAF)同函数粗检、R81 永不失败阈值断言盘点:均先出报告,不进棘轮。
- R71 余项 M3/M4/M6(M8 已落,设计见 flow-next/r71-design.md)。
- R74/R77 后续小修(两项已 DONE;R74 后续见第五节)。
- dup-census 白名单/cxlib 帮手抽取(muthelp、cscan)、台账待定项扫描。
- R72 已 DONE(0.0.90),R60 一期(A 类 8-10 个)不进本版。

## 二、建议 0.0.91 只收哪一类

建议:收 (A1) 身份与构建边界一类,沿用 version-seq 与占位裁定。理由:
- (1) L.1/Z5 有强制顺序(先 L.1 取 SID 快照、后删链接),且 version-seq 0.0.96 依赖 "0.0.91 身份口径",顺延代价高。
- (2) (A2)+(A3) 同属"事实表整理",但改 exec/ 表属 A 类输入、会令全量派生失效,与身份变更同版会污染 Q2 字节对拍的归因,故不与 (A1) 同版。
- (3) (A4) R70 是第三类(运行时缓存),需守门人 9 条件加二审,不得同版。
- 版本序建议:0.0.91 = A1;0.0.92 按 version-seq 为执行器内核产品化(A 类 14-16/16 零余量,不能再塞);(A2)+(A3) 作为"事实表整理"独立小版,建议插在 0.0.93(与 R70 同属 version-seq 现标 0.0.93/0.0.94 速度类的前面或另立),R70 排 0.0.93 之后速度类版,与 R60 一期同序。须编排者定。
- 备选(若编排者想先清已知红):0.0.91 改收 (A2)+(A3),身份与构建后移一号。代价是 0.0.96 依赖链顺延,不推荐。
- 风险提示:tsv-lint 红自 0.0.83 起无门执行,0.0.90 已带红发布并披露;留到 0.0.93 才修意味着连续 3 版带红,需在本版先把"补绑定后数 <= 上限"纳入 chain-ready 或棘轮的只降不升项(R75 要求),该并门动作在 tools/ 属 C 类,可本版并行做,不占 A 类名额。

## 三、A 类候选清单估计

| 候选 | 文件估计 | 依赖 | 先红测试 | 守门人 |
|---|---|---|---|---|
| A1 L.1+Z5+B3 | 17-19,cap 20(底稿 stack3/patches/combined-v4.patch;B3 名单待按 core-complete/verdicts.tsv 重列) | L.1 先于 Z5;cxroot.tsv 在 v3 下;text.num 54 处消费者先单例实编 | I1/I2/I3 守门检查待补(未见已有先红证据,需核) | 需要:身份类、关键路径 |
| A2 tsv-lint 补绑定 | 4 表 + 派生,约 6-9 exec/ 文件 | modelsignature 为主(削 573);其余 3 表再削 75 | 有:原度量 unresolved>11050 即红,csr 已有 red/green 日志 | 建议复核,手法为只补语义核实为静态的绑定 |
| A3 volatile 行列号 | 1(exec/lex/attr-byte.tsv) | 依赖 0.0.90 U4 的 diag 位置机制 | 需新增(现仅有 HALT REJECT 无行列) | 一般 |
| A4 R70 | 4-6(cx/runtime) | 9 条件 + 二审;8 变异;CI 全量对拍 | 8 变异先红后绿,尚未实施 | 必须,二审 |

## 四、并行车道窗口分工建议

- agy-flow(可用,可派 sonnet 子窗口):R71 M3/M4/M6、R74 后续、chain-ready 并入 tsv-lint 度量、A2 补丁落库(现有 csr 补丁需在私有树按 U4 独立验证复核)。
- ccogw(在 moltbaby 仓):R76 验收、迁移,不占 unisacc 候选。
- cdx 与 csr:不可达,待恢复。其产出(tsv-lint-F 补丁、r72/r69 补丁、zero-assert 方案)均已在 jobs 目录,待恢复后由其交付或由 agy-flow 接手复核。
- agyneon:无响应,发布列车、文档台账、dup-census 暂挂起。
- cc:主调度与 A1 守门人评审协调;1:cc 额度不可达时按长期授权推进。

## 五、起链前置清单

- (1) R80:版本槽 11-version-slot 必须与分支一致(bg-candidate start_preflight 已强制,已 DONE)。
- (2) chain-ready 20/20(不接受冻结时再切)。
- (3) R74 预跑 PD_MAX_WINS 扩容与无样本批保守耗时:owner-reqs 标 DONE,但"prepare 默认值修复"是否已落未从素材核实,需查 tools/prerun-drive.cx 与 check-prerun-drive 再定。
- (4) R77 daily 15 片编译冒烟批已 DONE;(5) 产生后备份前置门(backup-evidence)已落。
- (6) A1 的 Q2 字节对拍、SID 快照、ext-dock-exceptions.tsv 到期项更新。
- (7) 起链后列车工具冻结;严格预跑的无生产者 UNKNOWN 批(cx-booter-ape、tools-default-regression)若仍存在需随版披露。

## 六、缺失素材(如实)
- r70-review/ 目录不存在,9 条件只能转引 owner-reqs R70 摘要,条件全文缺。
- mut-audit 的全量 212 重放待办仅见 owner-reqs,无独立待办清单;R79/R81 无设计稿。
- A1 先红证据、B3 精确名单、R74 prepare 默认值落地状态未核实。
- A2 补丁基线为 base_WT=eb8473d9,需在当前 main 重放验证。
