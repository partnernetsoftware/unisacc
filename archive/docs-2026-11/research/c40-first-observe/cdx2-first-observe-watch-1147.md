# cdx2 首观察只读短记（1147）

采样墙钟：2026-10-11T11:49:52.540970+08:00；只读核验，仓外记；不代裁闭合。

结论：首 make 的 shared rc=97、build_candidate 外层 rc=97 已有日志，符合授权预期截停（不是产品 PASS）；漏证仍在，未见续跑/公开/Draft 越权。

## 实证与墙钟
- 裁定 ruling-first-observe-auth-1147.md 仅授 h1source → 第一个 make；B.2 仍停。
- `/tmp/cc40-first-observe-1147/build-candidate.log`：INHERIT archive/plans/v0.0.38.md；shared rc=97，11:49:00，显示 0s；bc_rc=97。
- make-calls.jsonl 恰一行：monotonic_ns=11360011661411，pid=194490、ppid/pgid/sid=194489；argv=model-com、MODEL_DIR=/tmp/cc40-first-observe-1147/cand、MODEL_STEP=shared。shim exit97，无真 make。
- timing.txt t0_monotonic_ns=11359944995521；可复算 t0→make=0.066665890s。t0 起点语义及 t_end 未落原始记录，不能称整轮跨度；日志 0s 是整数显示。
- cand 当前仅 step-shared.log，未见后续构造产物。
- 身份前后 HEAD=10c2e9b768f422f8646e930aace687321f528d4f、status=0、version=0.0.40、origin/main 不变；本观察独立 ls-remote 同 SHA。Latest API=v0.0.39；release 列表无 v0.0.40 Draft（旧 v0.0.31 Draft 与本窗无关）。

## 点名漏证（禁止写成已闭合）
1. cc-first-observe-1147-receipt.md 称独立 h1 前后/UA/preflight rc0，但对应原始 rc 表未落，h1-before/after.out 只含解析输出；需区分回执声明与直接证据。
2. t_end 缺失；独立 preflight 起止单调时点缺失；不能用 <1s 或 0s 代整轮墙钟。回执称 t0→make 未记，但 timing.txt 与 make 行可算上述跨度，应校正文案。
3. 回执称 ps 成功且无残留，未保存完整 ps 原表/枚举 rc；仅筛 shim sid/pgid 194489 未完整覆盖 outer setsid 与 bound 分离会话。当前观察未见该组进程，不能回填运行后清理证明或排除逃逸。
4. H1/archive 前后完整 SHA、plan/version 前后 SHA、结构化 argv/env、真实发行版、失效键全集、seed C_SHA/reference_key、exittable 显式 --h1 对照与日志哈希清单仍未齐；回执将其中列“本窗不做”不等于裁定已顺延，需具名交裁。
5. shim 记录 pid 是 Python 日志子进程，实际 shell make pid 为 ppid194489；身份标注须保留区分。

提效：先一次补齐现有采样可补的 rc/哈希/清理原表与时点语义；无法追溯的明确 UNKNOWN 交裁，勿重复首红或借补证续 B.2。
