# WF2 attempt2 回执（cc；机房主任 22:07 授权单窗）——单窗 rc65 观察完成；填尾：未覆盖
## 硬条件实况
- wt /tmp/cc40-prep/wf2/a2/wt @3a2d21d5（pull --ff-only 后的 tip；与 4ec76cb1 相比，gatequeue.py/release.sh/queue.sh/term.sh 无差异），status 0
- queue.sh PID 2800905，22:05:27–22:06:17，外层 monotonic 49.052 s
- 显式 QUEUE_WINDOWS=1；QUEUE_START=fresh；新空 state /tmp/cc40-prep/wf2/a2/state；RELEASE_JOBS 未设（默认 4）
- QUEUE_BACKUP=/tmp/cc40-prep/wf2/backup-a2：mkdir 后确认为空；realpath 后与 ~/.unisacc/queue-backup、attempt1 state、wt、候选目录无交集。fresh 未读；窗后被本轮写入（state/ + build.json），这是预期
- QUEUE_SUITES 共 8 项：tools-1 tools-2 tools-3 k2b attemptchain exittable stagelog gqalive；四个嵌套项已剔除
- 共享默认 backup：~/.unisacc/queue-backup 全部文件 sha256（1130 个）以及 path/size/mtime 元数据（1141 行），窗前窗后 cmp 都相同 → **不变**（shared-backup-{before,after}.{sha,meta}）
- attempt1 基线保留：receipt-attempt1.md sha256 5f0865cf…，attempt1 的 state 未动
## 读数
- window 1 rc=65 22:06:16；8/8 完成，0 失败；final rc=65（观察，不是验收）
- 8 次 START 全是 kind=full（left 48.3…47.5，都 ≥ span-3）；**tail 0 次，DEFER 0，142 0**
- attemptchain --results --strict rc=0：final 8，pending/unknown/gap 0，reconcile 无差异；full/DONE 8 次，72.6 job-s（不是墙钟）
- near-limit：tools-2 full limit 46，实际 45.14 s，余量 0.86 s（风险提示，不放宽）
- limit 与 int(left)-1 在 left 一位小数的误差带内一致（48.3→47，47.6/47.5→46）；不当精确不等式读
- history 只有 unisacc-gate-times-b28d6b89… 变化（hist-before/、hist-after/）；窗内 note 顺序重建：本窗没有 tail，不需要
- 现在没有 queue.sh/gatequeue 进程（owner 声明）
## 结论
入窗判据（plan 第 1–5 条，含 cdx 修正）在本窗只走到 full 准入分支；tail 准入、tail-142→DEFER→fullwindow 恢复链**未覆盖**。不算通过，也不硬造。WF2 门闩仍未通过。

## 更正 / 限定（cdx 22:1x）
- 时序：机房主任那条裁定自带的标签是“22:07”，但本 run 的真实时钟是 22:05:27–22:06:17。会话里的事实顺序是：我先收到那条授权，读完后才开始做准备并起跑。授权消息实际送达的时钟时间我不知道，记为 UNKNOWN；“22:07”只是裁定自带的标签，不是到达时间。我不补写事前授权时刻，也不改 run 的真实时间。标题里的“22:07 授权”应读作“22:07 标签的那条裁定”。
- 共享 backup 的元数据只比了 path、size、mtime，没有比 mode 和链接目标；mode 与链接目标是否守恒算未证。窗前没有采这两项，不补采。
- QUEUE_WINDOWS=1 等环境变量只有回执记载（run.log 里能看到 PID），没有保存完整的 argv 和 env。

## 更正附注（22:3x，追加，不覆写上文）
上文“送达时钟 UNKNOWN”一句，现在有原始来源可以对上：会话记录 /home/box/.claude/projects/-home-box-repos/145e14c2-a04c-4374-b42e-449275976753.jsonl 第 20864 行（uuid a8153a3b-e8ff-423f-8e76-31262a9b154f，整行 sha256 d41e288f…5e6a），ts 2026-10-10T14:05:09.145Z = 22:05:09 SGT，早于本 run 起跑 22:05:27。详见 /tmp/cc40-prep/next/rw/source-entries.txt。这是 owner 单方提供的来源，等评审独核。
