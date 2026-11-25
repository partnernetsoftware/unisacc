# 起链当天事故预案速查(107;2026-11-25 ccogweco)

只汇总已有页面的失败处置,不新造判据;每条给症状原行、第一动作、谁做、最长耗时、升级条件与出处。
总原则:先留原行,再定因;同一失败连续两次即停手;不放宽 45/120/55 秒与 2 lane。

| 类 | 症状原行 | 第一动作 | 谁做 | 最长耗时 | 升级给 cc 的条件 | 出处 |
|---|---|---|---|---|---|---|
| 链红 | `exit.tsv` 非 0,或 `CHAIN FAIL N <name> rc=R` | 保留链目录与 exit.tsv;读失败相位日志;不重起链 | agy-flow | 5 分钟定因 | 失败相位属产品/身份,或两次同因 | chain-day-roster.md;freeze-107-row.md |
| 链无输出 | 5 分钟无首行 `CHAIN RUN` | 查 owner.tsv、进程组、bg.log | agy-flow | 5 分钟 | 进程不活或三查无果 | chain-day-roster.md |
| 预跑红停 | `PRERUN_STOP` 或 `PRERUN_PRODUCT_RED_STOP` | 留原行;按 S1 分类:已审豁免/已定因夹具/产品红 | agy-flow/cdx | 2 分钟决定 resume 或停 | 产品红,或无登记类 | loss-guards-107.md;prerun-speedup-0.1.x.md S1 |
| 预跑 rc=124 | `PRERUN_WINDOW ... rc=124`(58s 级) | 停该窗;核 2 lane 与 PATH;按原窗并发口径复测 | agy-flow | 单窗 60 秒 + 处置 5 分 | 连续两次 rc124 或同窗复现 | loss-guards-107.md(损耗三) |
| CI 瞬态 404 | `windows-11-arm` 格 fetch 失败、`|| echo warning` 吞掉 | 不重派整轮;用 G4 `--redispatch` 新 run,保留旧 run | cdx | 5 分钟 | 重派后仍同格失败 | car2-16-ci-run-attempt.md;loss-guards-107.md |
| CI 真红 | `CANDIDATE_QUALIFY verdict=FAIL` 或非零 fail | 保留 json/log;定位格与首个诊断;不改判据 | cdx | 10 分钟定因 | 产品侧红或无法复现 | car2-timeline-v2.md;notes-0.0.106-final(192) |
| CI UNKNOWN/无 run | 六格无结论、job 丢失 | 查 run 列表与 attempt;补派一次 | cdx | 5 分钟 | 两次无结论 | car2-16 页 |
| 读回红 | 读回非 `READY`,或 `RECEIPT_DIFFERS` | 不公开;同 argv 再 warm,核 `warm-*.tsv` | agy-flow | 3 分钟 | 再次非 READY | loss-guards-107.md(损耗二);car2-prewarm-list.md |
| Y1/Y2 推送被拒 | `PREPUSH_REFUSE` / push 非 0 | 核 `DEVSVC_OWNER=cc`、freeze 行 owner、PUSH-FREEZE 标记 | cc | 2 分钟 | owner 不符或冻结行异常 | freeze-107-row.md;prepush.cx:1-20 |
| prepush 拒(FREEZE_EXPIRED) | `FREEZE_EXPIRED ... deadline<now` | 过期的冻结行不因过期解冻;续期或显式删行 | cc | 2 分钟 | 需改 deadline | freeze-107-row.md |
| devsvc 死锁/不响应 | `devsvc ping` 不通、lock 不释放 | `devsvc status` 看在跑 job;`wait` 或停该 job;不杀他人锁 | cc | 5 分钟 | 实例无响应需重启 | devsvc 用法;cc-chain-day-script.md |
| 窗口卡菜单 | 交互提示等待输入(未在脚本内) | 不用交互命令;改用 `--plan`/非交互参数 | 发起窗口 | 2 分钟 | 工具无无交互路径 | cc-script-audit.md |
| 上下文将尽 | 会话接近上限 | 先写交接要点(产物 sha、未落库清单、待办),发 cc,再压缩 | 各窗口 | 5 分钟 | 压缩后仍不清 | handoff 样例;lessons-106-night.md(5) |

## 通用升级模板(cc 每轮三句话)

    进度:<链 n/28 或 预跑 done/total 窗>;当前相位 <phase>。
    预计完成:<HH:MM>(基准或推断)。
    风险:<触发的类 + 硬线剩余分钟>;需要 <谁> 在 <分钟> 内做 <动作>。

## 不做什么

- 不重启已判定失败的链(除链目录可续的同候选 resume);不整轮重派 CI;不删原 FAIL 行;不登记预期失败;
  不为过线放宽任何时限;不抢他人锁或杀他人 job。
