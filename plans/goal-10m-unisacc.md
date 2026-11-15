# 定时器提示词(unisacc 主线推进)

用途:会话内每 5 分钟触发一次的推进提示词。定时器只在会话内存活,7 天自动过期,重建时用下面的原文。文件名沿用旧称。
2026-11-14 重写:去掉已退役的 agentwork/dag.tsv 与 real4 专名;协同只用 prd_mempalace.md;新增"前瞻不闲置"硬规则(在途少于 3 个必须补派,fable 前瞻规划有触发条件)。

## 提示词原文

```
推进unisacc.com 新版本的设计与开发到发布公开(公开后要对缓存目录临时目录大扫除才继续下一个版本);今天要疯狂推进 unisacc.com 的进化(项目所有者 2026-11-14),少记账多推主线;
协同只用 prd_mempalace.md(记忆宫殿是唯一协同入口,agentwork/ 退役不再追加;当前版本、阻塞、下一动作原位更新,不写流水账,只记别人必须遵守的决定、不可逆动作、阻塞发布的 UNVERIFIED);
主线:核心库包(cx/cx0/ext)由内往外封装、cx.stream 与 tape 库(tlib)重构;每轮先报告本轮落了什么到 main,没有就定因换动作;
前瞻不闲置(新增硬规则):每轮开头先数在途 subagent 与未派发的可做任务;若在途少于 3 个,必须当轮补派到 3 个以上,不得空等整链、CI 或下一次触发。补派优先级:(a)fable 前瞻规划:距上次规划超过 60 分钟、或下一动作清单少于 5 项、或刚有整链/发布/CI 结果时,派 fable 预判后续 2 到 3 个版本的瓶颈与风险,产出写进 prd_mempalace.md 下一动作原位更新,任务拆到单项 15 分钟内且写域互不相交;(b)fable 设计演进:cx 模块与内存安全(export、通配、cx.stream、tlib)、nn 以网换逻辑的下一步、造工具辅助进化;(c)sonnet 执行:规划中已拆好的可开工项、并行化与加速、扫除、文档合并。规划产出的任务当轮即派,不等下一轮;
持续项:unisacc.com 自身运行速度(冷启动、冷编译典型工具、生成代码三档)要持续提升,每轮至少有一个速度项推进或有新实测;速度数字只用 tools/run-startup-bench.cx、tools/coldcompile.cx、tools/check-bench-gencode.cx、tools/speed-probe.cx 实测给出,不报估计;发布前对比 build/speed-baseline.tsv,变慢必须说明理由;产品代码优先于基础设施;
并发规则:写域互不相交的任务同时派 subagent(难题与规划 fable,一般执行 sonnet);只在 ~/repos/unisacc 工作,检查一过立即提交推送(先确认 git branch --show-current 为 main,再 git push origin HEAD:main),不建外部目录、不往仓库 out/ 放任何东西,不开新规划文档;派工简报固定五段(目标、写域、验收命令含预期值、截止、禁区),禁用 sed -i/python/node/perl;
巡检:subagent 超过 12 分钟催收尾、超过 15 分钟停掉接手;盯在途整链(日志在 /tmp/unisacc-session-build-lr0060/log/,构造根 src 工作树必须与被构造 REV 同提交),失败立即取证;机器忙是现象不是前提;整链/CI 等待期间调用 wait-housekeeping 技能做维护动作。
中级扫除(无新事实时,每轮不超过 10 个文件):归档过期记录型文档(git grep 引用计数,无活引用才 git mv 到 archive/ 并追加 archive/ARCHIVE.tsv,再跑 tools/check-archive-refs.cx)。
反交差:(1)每轮至少独立复验一条他人报告的 PASS;(2)只报增量,没有新事实就一句话并换动作;(3)每轮抽查自己上一轮一个判断去证伪;(4)在途已满(3 个以上)不凑任务,不满必补;(5)未验证清单保持在 prd_mempalace.md 可见,公开前清零或如实写进发布说明。选置信度最高的动作直接执行,用真实输出验收。
输出格式:结论一句话,其余短列表或表格,段落不超过 400 字符,只写决定、结果、风险。
```

## 重建命令

CronCreate,cron 为 `*/5 * * * *`(项目所有者 2026-11-14 由 7 分钟改为 5 分钟),recurring 为 true,prompt 取上面代码块全文。当前在跑的定时器 id 为 932cbd2a。
