# 10 分钟定时器提示词(unisacc 主线推进)

用途:会话内每 10 分钟触发一次的推进提示词。定时器只在会话内存活,7 天自动过期,重建时用下面的原文。
来源:开头取自旧定时器 3fd4a0fc 的原文;2026-11-13 起附加巡检要点、foresight(前瞻)与前提审计(premise audit)。粒度刻意保持粗,避免细则导致漂移。
命名:前瞻叫 foresight(参谋 foresight advisor);质疑前提的角色叫 premise audit(前提审计)。
设计要点:foresight 找杠杆,premise audit 质疑前提;两者职责不同,不合并。

## 提示词原文

```
推进unisacc.com 新版本的设计与开发到发布公开(公开后要对缓存目录临时目录大扫除才继续下一个版本);
主线:核心库包(cx/cx0/ext)由内往外封装、cx.stream 与 tape 库(tlib)重构;每轮先报告本轮落了什么到 main,没有就定因换动作;
持续项(项目所有者 2026-11-13 强调,每份 plan 都要写):unisacc.com 自身运行速度(冷启动、冷编译典型工具、生成代码三档)要持续提升,每轮至少有一个速度项在 dag 里推进或有新实测;速度数字只用 tools/run-startup-bench.cx、tools/coldcompile.cx、tools/check-bench-gencode.cx、tools/speed-probe.cx 实测给出,不报估计;发布前对比 build/speed-baseline.tsv,变慢必须有账本理由;产品代码优先于 tools/tests/build/agentwork 基础设施(近 7 天 91% 提交只碰基础设施);
按任务图并发推进:每轮看 agentwork/dag.tsv(有 tools/dag.cx 时跑 dag plan),写域互不相交的就绪节点全部同时派给自己的 subagent(难题与规划 fable,一般执行 sonnet);只在 ~/repos/unisacc 工作,检查一过立即提交推送,不建外部目录、不往 out/ 放交付物,不再开新规划文档;

巡检要点:subagent 超过 12 分钟催收尾、超过 15 分钟停掉接手;盯发布尾链与整链有失败立即取证。机器忙是现象不是前提:负载高时先查是谁占用,验证的判定不得依赖「机器安静」,依赖了就是缺陷要记下来。

前瞻(foresight):轮次末若没有运行中的 fable foresight 参谋,就开一个(只读,12 分钟):找出让研发迭代持续变快的杠杆,先用数据说明时间花在哪、同类缺陷重复几次,再设计新杠杆并挑 1 个写成五段简报;下一轮立即派出。

前提审计(premise audit,与前瞻交替或同轮各开一个 fable,只读,12 分钟):专门质疑「正在做的事所依赖的前提」,不是找 bug。凡是反复出现的缺陷、需要特殊条件才能成功的流程(例如要安静机器、要人工轮询、要降载令、要重起整链)、每次都靠补丁处理的同类问题,都先问「这个前提本身对不对、能不能在设计上让它不再需要」,给出证据(file:line、账本行)、根治设计和第一步简报。

回复末尾写一行「foresight:运行中/已交付 N 项已派;前提审计:运行中/已交付 N 项已派」。计划之后选置信度最高的动作直接执行,用真实输出验收。
```

## 重建命令

CronCreate,cron 为 `*/10 * * * *`,recurring 为 true,prompt 取上面代码块全文。
