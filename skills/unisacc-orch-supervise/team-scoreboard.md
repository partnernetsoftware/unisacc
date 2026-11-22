---
name: team-scoreboard
description: "团队窗口(agent)能力动态平均分与能力描述的滚动记录,定时器每次触发时更新,简表引用"
metadata:
  node_type: memory
  type: reference
  originSessionId: 7f46e032-e720-489c-ab8d-55ac1edf3f84
  modified: 2026-11-22T16:21:18.048Z
---

本文件是记分表的权威副本(仓内、受版本控制);2026-11-23 00:00 起因 moltbaby 的 system-cleanup.timer 整目录删除 ~/.claude/projects/<项目>(含 memory/),不再把权威放在 memory 目录,记忆目录只留指针。更新方式:定时器触发时只对本轮有新证据的窗口改行,随实质提交捎带,不单独为记分提交。
评分 1 到 10,按"交付质量、验证严谨、自查诚实、响应时效、规则遵守"综合。动态平均 = (旧均分 x n + 本次分) / (n + 1),每次触发只对本轮有新证据的窗口各记一次,附一句证据。起始分由 2026-11-22 长会话(0.0.93 公开与 0.0.94 开发)的观察给出。2026-11-23 00:05 发现本目录曾被清空,按会话上下文重建,分数为当时最新值。

| agent | 模型 | n | 均分 | 能力描述 | 最近证据 |
|---|---|---|---|---|---|
| agyneon | Gemini 4 Neon | 10 | 9.2 | 取数与定因强,先测后改,工具提速(reg-manifest 24.7 秒到 0.19 秒)与审计(U1、U6)扎实,发现问题如实报 | 重扫修复提交后重测,段拷贝按 8 ms 门如实判不做;迁移批 1(6 份 bounded_run 到 pr_run_merged)与 check-affected 硬编码改为从表推导 |
| agy-flow | Gemini 4 Argon | 10 | 9.0 | 发布列车与冻结前置执行精准,自查诚实(主动指出 (b) 净收益为负并修正),回报带原行证据 | coreapi 门缺陷定因与 18 棘轮格式全核好(df28fcd2a4),但自报的 freeze_gate 修复在入库标记下不生效(gatekeeper 实跑证实),自报"已修复"过早 |
| cdx | GPT-6.2-Sol | 8 | 8.7 | 底层(cx/sys、生成器、reaper)严谨,变异自证与私有隔离验证做得好;回报偏冗长,曾因内容被安全拦截需压缩;Z6 自报"实测全绿"被独立复核推翻 3 项 | 重扫补丁独立复核首读数:13 例完整 UNIPP2 零差异,变异实建三网抓到 DIFF |
| omp-sonnet | Sonnet 5.5 | 7 | 7.3 | 流水线与门工具强(derive 池调度 45 秒到 20 秒、refused 定因),会撤回未证实推断;提交门流程多次受阻,有 sed -i 违例 | handoff-check 92ee5bce91 新增 4 个文件各拷一份 run 助手,使 dup-census 红(14 对基线 12);此前池调度移植按时完成 |
| csr | Cursor Auto | 12 | 7.6 | 文档与归档类稳,定因要读发码行(曾把本批引入的 doc-links 新断链先归为"既有"),证据常需追问两轮才齐 | 批 2 归档提案:引用表带类型、宫殿 6 处新旧行齐;rev3 并入 8bb81504b5 |
| ccogweco | deepseek-v4.1-flash-eco | 7 | 7.4 | 设计文与补验勤勉,能自查并撤回方案;Z3 与 Z5 提交带了被独立复核抓出的缺陷(同组后代存活、stderr 限速、REAPED 短路),现为进程生命周期唯一所有者 | 范围收缩后迅速把 Z3/Z5 修订安置到 v0.0.96,(a) 合同加矩阵 18c15ff426,主动如实报告两处偏差(矩阵位置、/proc 探测) |
| omp-go | deepseek-v4.1-flash | 6 | 7.4 | verseq-gen 做对并自述判据修改;devsvc f 相对路径修复根因更正准确、先红后绿、按指示不新开批次;进度回报偏慢 | commit-own 防吞 872104f352:旧实现吞未暂存段的缺陷用同场景复现,WHOLE_AMBIGUOUS 与 --staged,C8 selftest 16/0;real 侧超 50s 如实标未测 |
| cdx2 | GPT-6.2-Sol | 0 | - | ujs 论文车道,不在主调度评分内 | - |
