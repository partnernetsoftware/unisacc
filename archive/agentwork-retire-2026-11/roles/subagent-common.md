# subagent-common
ROLE: 所有子代理共同遵守的工作规则。编排者的简报只写本任务特有的五段(目标、写域、验收、截止、禁区),本文件写所有任务共有的部分,简报开头写「共同规则见 agentwork/roles/subagent-common.md,先读」即可。
READ: 本文件;AGENTS.md 的协作、提交、测试与性能各节;agentwork/roles/orchestrator.md 的复验与共享检出纪律。
DECIDE: 范围内的实现细节自己定,范围外的列入报告,不自行扩大写域。

## 工作位置与产物
- 直接在 ~/repos/unisacc(main)里工作;不 clone、不 worktree、不建 out/ 或 ~/work(钩子会拒绝);仓内 out/ 必须始终不存在,每个批次跑完核对。
- 临时文件放 $CLAUDE_JOB_DIR/tmp 或 sy_tmpdir();需要私有构造(重建产品)时在 /tmp/unisacc-session-<任务名>/ 自建会话根,保持活跃(tmp-sweep 清理 120 分钟未改动的条目)。
- 只用 Edit/Write 或 .cx 工具改仓内文件;禁止 sed/awk/printf/heredoc 改仓内文件;分析脚本用 .cx。

## 提交
- 只提交自己的路径:`sh ./unisacc.com tools/commit-own.cx -- -m 说明 --whole 路径...`;共享表(tests/batches.tsv 等)用 --keyed,一次提交只用一个 --keyed(多个并用时第一个会被丢)。
- 预检一分钟以上:放后台跑并写输出文件;HEAD 被别人推进导致 cannot lock ref 时重试一次,同一失败连续两次即停手上报。
- 提交前 git diff --cached --name-only 确认没带他人路径(共享检出里常有别人暂存或未提交的改动);不 stash/reset/clean/checkout 他人路径。
- 新批次登记:写 tests/manifests/<id>.tsv 清单,跑 tools/tables-gen.cx -- --write 与 tools/batch-packages-gen.cx -- --write,清单、三张生成表、检查文件同提交;预算秒数填实测最大值的 2 倍(不是用例数),检查输出的标记行用「标记名 TAB 其余」。
- 改了包源后 layer-api 要 bump:先 git add 包源路径再 `layer-api --bump 包名`,把 build/layer-api/<包>.tsv 与 build/tlib-packages.tsv 的行一并提交;漏 bump 会让 main 变红。

## 代码规则
- 新 .cx 不手写 #include,用 #pragma unisa use;不直接调 libc(printf/malloc/strcmp 用 sy_*/mm_*/tx_*;tools 里的输出可用 printf);上层不写平台 #if。
- precommit 的 R4 lint 禁止对输入内容做 strstr/tx_contains 子串锚点且不可 allow:用 memcmp 逐位置循环(参照 tools/check-s9-nightly.cx 顶部 has())。
- cx/sys、cx/core 受 L0-CORE-FREEZE-049 冻结:子代理只 git add 不提交,由编排者(冻结 owner)以 UNISACC_INTEGRATOR=cc 提交。
- 事实表只经「事实表 -> 生成器(--check/--write)-> 表」修改;kernel/ 是生成物,禁止手改。

## 验证
- 验证对象必须是入库路径,不是自己 /tmp 里的副本(模块按入口文件所在目录解析:仓内跑探针时临时放 tools/ 下,跑完删除)。
- 批次判定只认 `BATCH_RUN status=PASS`;DEFERRED(start-deadline、killed-at-deadline)既不是 PASS 也不是 FAIL,重跑到明确结果。
- 机器是 16 核,load 4 只是约 25% 利用率;验证判定不得依赖机器安静;性能数字取 5 次最小值与中位并记 uptime,不因负载丢弃数据。
- 度量先交叉验证再当依据;推算必须标明依据,不得当实测;估算耗时必须给出每个数字的来源行。
- 每条命令 60 秒上限,更长的放后台并设总 deadline,不无界轮询;同一失败连续两次即停手上报,先读发出该退出码的源码行再判断。

## 进度与汇报
- 3 分钟内交第一个数据点,之后每个数据即写进 $CLAUDE_JOB_DIR/tmp/<任务名>-progress.md(Write 整份覆盖);12 分钟编排者看进度,15 分钟必须交部分结果;到点没做完,要么提交已自洽的部分,要么还原自己的改动并把补丁存到 $CLAUDE_JOB_DIR/tmp,不留半成品在工作区。
- 报告:先一句结论,再短列表或表格,只写决策、结果、风险,不超过 300 字;中文,无 emoji,箭头用 ->。
- 没有实跑的写「未跑」;UNVERIFIED、DEFERRED 如实列出。

DON'T: 不删断言、不缩覆盖、不放宽时限、不为变绿改判据、不提交他人路径、不泄露凭据;不新增说明文件和规划文档。
OUTPUT: 本任务简报规定的验收输出,原样贴命令输出,不转述。
