# 进程生命周期合同(proc-lifecycle)

所有者:ccogweco(进程生命周期唯一所有者,cc 2026-11-22 派工)。写域:`cx/sys/proc.cx` 的
`sy_proc_*` 段、`ext/sysx/reaper.cx`、`tests/manifests/proc-lifecycle.tsv`、
`tools/check-proc-lifecycle.cx`、本文件。cdx 与其他窗口只改消费者,经 devsvc 写锁按文件拿窗口。

此前 Z3(capture)、Z5(REAPED 状态位)、Z6(reaper 回收路)各补一块,互相打架,根因是
"组长何时可以回收"没有唯一答案。本文件给唯一答案。

## 一、九条合同

1. **pgid 归句柄独有**:带组 spawn 的 pgid 在 `sy_child_t` 里(`h`),对外不另存、不缓存副本。
2. **锚定规则**:组清理完成前不回收组长。僵尸组长钉住 pid 与 pgid,这是"清理先于回收"的
   物理保证。提前探测用**不消费**的 `waitid(P_PID, pid, &si, WEXITED|WNOHANG|WNOWAIT)`,
   放 `_os` 静态帮手;不读 `/proc`。Linux 先实跑验证(live 时 si_pid=0,退出后 si_pid=pid,
   两次探测都不消费,之后 waitpid 仍取到 code=7);osx 标 UNMEASURED 并写明风险
   (osx 的 waitid 存在但 WNOWAIT 语义需核;不能确认时探测返回 -1,调用方不再等截止)。
   **可观测性(已闭合)**:锚定的存在理由是"组长提前回收后组号被复用,再对 -pgid 发信号
   会误伤"。夹具在 `unshare -U -r -p -f`(user+pid,无需 root,与 cdx 的
   check-reaper --reaped-root-pidns 同法)下让哨兵复用被回收组长的同号并自建同号组,
   五条断言全 PASS(`sentinel-reuses-leader-pid`、`reaped-handle-returns-esrch`、
   `reaped-handle-kill-esrch`、`sentinel-not-signalled`、`positive-control-observes-kill`)。
   阳性对照证明该夹具能观察"哨兵被信号"。
   **变异已闭合**:关键在观察方式——信号的投递与"可被 waitpid 看见"是异步的,单次
   `waitpid(WNOHANG)` 立刻读会把已投递的信号漏看(2026-11-23 首次三版不红的根因)。
   改为在调用后轮询 `waitpid(WNOHANG|WUNTRACED)` 最多 300 ms(10 ms 一步),负断言与
   阳性对照共用同一 `observed_signalled`。此后:"REAPED 分支仍对组发信号"的变异版
   `anchor.sentinel-not-signalled` 转红(正常版 5/0,变异版该条 FAIL)。
   `--anchor-only` 参数只跑该组,namespace 内 5/0、55 秒内完成。
3. **REAPED 精确含义**:禁止一切以该 pid 或以该 pgid 为目标的信号与等待;
   `sy_proc_try_wait`/`sy_proc_wait` 返回已存状态;`sy_proc_kill` 返回 `-ESRCH`。
   前提是规则 2 保证清理先于回收。**不得**为了清后代而绕开句柄直发 `kill(-pgid)`。
4. **唯一清理原语** `int sy_proc_end(sy_child_t *c, int grace_ms, sy_exit_t *st)`:
   TERM 组 -> 有界宽限 -> KILL 组 -> 有界回收组长 -> 置 REAPED。全程有截止,
   **不依赖管道是否关闭**。`sy_proc_capture`、`sy_proc_wait` 的到期分支、`px_run`、
   `fl_kill_wait` 都调它,不各写一套。
5. **rp_kill_tree 只管逃逸后代**(setsid、二次 fork 离开组者),三入口共用一份实现:
   - A `rp_kill_tree(sy_child_t *c, ...)`(句柄):回收前调用,自己不回收根;
   - B `rp_kill_tree_pid_after_wait(pid, reaped, root_start, ...)`(调用方已自行判定回收);
   - C `rp_kill_tree_pid(pid, ...)`(裸 pid,头注释标"无身份保护,仅限未被 wait_any 或其它路径
     回收的子进程")。
   **已回收(root_reaped=1)的最终合同**(cc 2026-11-23 采纳 ccogweco 读法,替换先前"reaped=1 一律
   -ENOSYS 零信号"的字面表述):
   - Linux:不对根 pid、不对根的进程组发任何信号(pgid 可能已复用);只扫孤儿,且**每个孤儿
     发信号前必须核对其 starttime 与回收前基准一致**;`root_start` 缺失则不扫并返回 `-EIO`;
     全部清完且无同 starttime 存活者才返回 0,有存活返回正数。
   - 非 Linux:`reaped=1` 时零信号,`-ENOSYS`。
   - **为什么改**:先前字面"一律 -ENOSYS"与 cdx 的 check-reaper 合同冲突——该夹具要求根回收后
     孤儿(持输出管道者)仍被清掉,否则 `np_run` 的 residue 无法证实为 0。所以已回收不等于
     "什么都不做",而是"不碰根与组,只在身份核对下清逃逸后代"。
   - 变异必红(各一个夹具):(a) reaped 时对根或组发信号;(b) 孤儿不核 starttime 就发信号;
     (c) `root_start` 缺失仍返回 0。
6. **subreaper 归进程入口设置**:kill_tree 先自查;未设置时孤儿清扫返回具名拒绝,
   **不得返回 0**。`residue=0` 只在可证实时才允许。
7. **返回三态**:0 = 已证实干净;正数 = 存活数;负数 = 无法证实(-errno 或具名类)。
8. **运行期检查不是编译期所有权**:裸 pid 入口(`sy_signal`、`sy_killpg`、`sy_wait`、
   `sy_kill`)没有保护;非 Linux 的离组后代不受保护;Windows 全部 `-ENOSYS`。
   这一条同时写进 `cx/sys/proc.cx` 与 `ext/sysx/reaper.cx` 的文件头。
9. **本版不做**:pidfd;Windows 实现;发布路径三份循环的迁移(延到 0.0.95 之后)。

## 二、执行顺序(固定)

- (a) 本文件 + `tests/manifests/proc-lifecycle.tsv`(状态 x 操作矩阵)+
  `tools/check-proc-lifecycle.cx` 先落,允许红;变异自证四个:删 REAPED 置位、
  删 KILL 升级、锚定改成提前回收、删 subreaper 自查。
- (b) Z5 修订版:REAPED 恢复"全禁"(信号、等待、组),靠规则 2 的锚定保证清理在前;
  `sy_proc_wait` 超时写 `c->exit.timed_out`。
- (c) Z3 capture 改调 `sy_proc_end`,并修三处 fable 读码推断:
  子进程关 1/2 且忽略 TERM 时到期后无界挂起;非 Linux 无 /proc 时"已退出"探测恒假把正常
  退出报成超时(且 /proc 依赖必须收进 `_os`);`px_run` 在 `rp_kill_tree` 已回收根后读未
  初始化的 `st`(`ext/sysx/run.cx` 约 186-193 行)。每项先红后绿。
- (d) Z6 由 cdx 在 (c) 之后改 reaper 入参(我提供合同,不改他的文件)。

## 三、状态 x 操作矩阵(与 `tests/manifests/proc-lifecycle.tsv` 一致)

状态:LIVE(组长在跑)、ZOMBIE(组长已退出未回收)、REAPED(已回收)。
操作:`kill(TERM)`、`kill(KILL)`、`try_wait`、`wait`、`close`、`end(grace)`、`tree`。

| 状态 | kill(TERM) | kill(KILL) | try_wait | wait | close | end | tree |
|---|---|---|---|---|---|---|---|
| LIVE | 发组与 pid,0 或 -errno | 同左 | 0 运行中 / 1 已回收 | 阻塞至回收 | 只关 fd | 全流程 | 只清逃逸后代 |
| ZOMBIE | 发组,0 或 -ESRCH | 同左 | 1 并置 REAPED | 立即回收 | 只关 fd | 回收并置 REAPED | 只清逃逸后代 |
| REAPED | -ESRCH,零系统调用 | 同左 | 1,返回已存状态,不 waitpid | 0,返回已存状态 | 只关 fd | -ESRCH | 用回收前 starttime |

矩阵里每一格都是 `tools/check-proc-lifecycle.cx` 的一条断言;尚未实现的格在矩阵表里写
矩阵里每一格都是 `tools/check-proc-lifecycle.cx` 的一条断言;尚未实现的格在矩阵表里写
`GAP`,检查里写 `FAIL`(先红),实现后改 `PASS`。

## 四、已知限制

- **pid namespace 内造不出"同 pid 不同 starttime"**:用 `unshare -U -r -p -f` 加
  `ns_last_pid` 让 Y 复用已回收 X 的 pid(实测 X=6 Y=6 成立),但两条的
  `/proc/PID/stat` field 22(starttime)读到**相同值**(26 对 26,间隔加到 1.2 秒仍相同)。
  namespace 内的 starttime 以 namespace 启动点为基准且 pid 复用时不改变,所以"复用后基准
  对不上"的夹具**不能用 namespace 路线造**。变异 (b) 的夹具因此改成白盒:直接对
  `rp_dispose_list` 传入一个真实存活进程与一个故意错误的 starttime 基准,这是产品的真实
  代码路径、对真实进程施加真实错误基准,不是 mock。`tools/check-reaper.cx --identity-mismatch`
  覆盖,删身份检查即红。
- 非 Linux 无 `/proc` 时 rp_kill_tree 返回 `-ENOSYS`(组内已杀、逃逸后代无法证实),该分支
  在 Linux 验收机上不可达,记 UNMEASURED。
- 裸 pid 入口 `sy_signal`/`sy_killpg`/`sy_wait`/`sy_kill` 无身份保护;Windows 全部 `-ENOSYS`。

## 五、整改进度(gatekeeper 第二轮,封顶两轮;写于 2026-11-23)

被退回的三个提交:`a5718fab8f`(D1)、`358c2d239c`(变异 b)、`f7e9069324`(D2)。
前序 (b)(c) 提交保留:`6ab81e577d`、`19514fecdf`、`2ad0f656aa`、`aae7fdc7b6`、`8a98b7dce2`。

夹具:`~/.claude/jobs/7f46e032/tmp/gk96-gkfx.cx`,复制到 `tools/gkfx.cx`(用完不入库),
`sh ./unisacc.com tools/gkfx.cx -- f1|f2|f3|f3b|f4`。修前复现原行:
- f1 `entryC_on_reaped_root_ret=0 orphan_alive=1 FALSE_CLEAN` -> 整改 (a)
- f2 `ret=0 waitpid_after=-1 errno=10 ROOT_REAPED_BY_SWEEP` -> 整改 (b)
- f3 `root_start_passed=0 ret=0 bystander_after_root_killed=1` -> 整改 (c)
- f3b `bystander_after_root_killed=1` -> 整改 (c) 兄弟误杀
- f4 `no_subreaper ret=0 escaped_alive=1 FALSE_CLEAN` -> 整改 (d)

五点整改映射(每条改完贴 gkfx 修后原行):
- (a) `reaper.cx` 212 区域:`kt_stat` 只在 ENOENT 时判干净 0,其余失败返回 `-EIO`;
  `check-reaper.cx` 的 "a reaped root is 0 residue" 断言随之改写。
- (b) `rp_dispose_list` 增 `root` 参数,僵尸根不 `sy_wait_status`(回收归 `sy_proc_end`);
  入口 A 与 C 差别体现在调用是否传 root。
- (c) `rst <= 0` 一律 `-EIO`;孤儿判据加强为"根的后代"(`pp == sy_pid()` 且 `st >= rst`),
  不再杀所有非 keep 子进程。
- (d) subreaper 自查前移到两条路径之前,改为**查询** `sy_child_subreaper_get()`
  (posix.cx 新增,PR_GET_CHILD_SUBREAPER=37);未设时 `-ENOSYS` 而非 0。
- (e) 补三夹具各配变异必红:删 `st < rst`、入口 A 不回收根、subreaper 未设逃逸返回非 0;
  另 D3:batches 登记 `check-reaper-pidns`;processcore/releaseprepare/run.cx 写
  "cleanup unconfirmed";修 s9-shards.cx:714 与三个丢弃返回值的调用点。

三次提交顺序:
1. 整改 (a)-(d) 代码 + 对应 check-reaper 断言改写;
2. 三个新夹具 + 三个变异必红;
3. D3(批次登记 + 四家 cleanup unconfirmed 文案 + s9-shards/丢弃返回值调用点)。

提交 1 记录(2026-11-23,接手者 omp-sonnet):
- 代码:posix.cx 加 sy_child_subreaper_get;reaper.cx 的 rp_sweep 按 (a)-(d) 重写。接手后补三处:
  (1) 删 ccogweco 留下的重复 `rst <= 0` 行;
  (2) SIGKILL 异步落地,处置改为有界轮询(每 2 ms 一步,至多 deadline/2 步,上限 500),旧循环里的延时被删后
      曾使 tree 夹具读到刚发信号尚未退出的后代(res=1);
  (3) 已回收根的孤儿判据由"一律 -EPERM"改为"可证实才杀":与根同进程组、starttime 不早于根、且根 pid 当前
      无活进程(未被复用)者逐个按身份杀;其余"本进程的子进程且不早于根"者(逃逸孤儿或之后派生的兄弟)永不发信号,
      存在即 -EPERM。根仍活着的路径同样在处置后探测此类子进程,有则 -EPERM,不返回 0。
  subreaper 查询对已回收入口也执行(原只在未回收时)。
- gkfx 修后原行:f1 -5;f2 ret=0 根仍可 waitpid;f3 -5;f3b -1 无兄弟被杀;f4 -38(-ENOSYS)escaped_alive=1。
- tree/escape 的 res 实测:修前补轮询时 tree res=1(rep 一个后代)、escape res=1,同一竞态;补轮询后均为 0。
- check-reaper 旧期望到新期望(修前 10 项 FAIL,现 31 项全 PASS,断言总数只增不减):
  A 夹具改成可证实场景,保持被杀且返回 0:
  - reaped-root(非 exact):根先 setpgid 成组长,孤儿留在组内;调用传真实根 pid,哨兵兄弟放进 keep。
    旧:孤儿被杀且 0。新:同。理由:旧夹具的孤儿与哨兵都只靠"本进程子进程"识别,不可证实。
  - escape:中间进程不再立即退出(睡 3 s),守护者是可由父链证实的后代。旧:中间退出后孤儿被杀且 0。新:
    父链完整时被杀且 0。孤儿形态另立 C 夹具。
  - np_run 三项(timeout、real root PID、orphan holding stdout)与 check-netpack 三项:无需改写,代码修好后原样通过。
  B 旧期望是假干净,改负值:
  - "kill_tree: a reaped root is 0 residue" 改为 == -EIO(裸 pid 已回收,无起始时间基准)。反例:gkfx f1
    (逃逸孤儿存活,旧返回 0)。
  - reaped-root(exact,pidns):旧 res==0 且孤儿被杀;新 res<0 且孤儿未被发信号。理由:根 pid 被哨兵复用,
    孤儿不在任何可证实的组内,杀它只能靠猜。
  - tree/escape 的"根与后代已消失"改用 running()(僵尸不算运行):僵尸根的回收归句柄所有者,不归扫描。
  C 新增(4 项):orphan 夹具,父进程先退出的睡眠者:根与孤儿在跑、根被清掉、返回值为负、孤儿未被发信号。
- 已知先存问题:`--pid-reuse` 的 "the reused pid has a different starttime" 在 v0.0.96 的 HEAD(f7e9069324)
  上同样 FAIL,与本次改动无关(未查根因)。
- 其余检查:check-netpack 17/0、check-process-deadline 0 失败、check-l0-killtree 10/0、check-l0-capture 63/0。
- 下一步:提交 2(三个新夹具与变异必红,含孤儿不核 starttime 的白盒夹具),提交 3(D3)。

## 六、裁定:(d) 切出 0.0.96;已回收根只报告,不杀(2026-11-23)

- 依据:三轮整改每轮换设计、每轮出新阻塞。结构原因是靠 pid 加 starttime 事后推断身份:根自行退出并被回收后,
  setsid 逃逸的后代在 /proc 里无从归属,pid 与 pgid 号可被无关进程复用,按号发信号只能猜,猜错即误杀旁人。
- 0.0.96 交付的安全子集:sy_child_subreaper_get(只读查询);rp_dispose_list 不回收根;rst<=0 返回 -EIO;
  入口 C 根已消失返回 -EIO;未回收路径 rp_unproven(同父后启动且不在名单的子进程不发信号,答 -EPERM);
  rp_snapshot 上限溢出返回 -ENOSPC 不截断。
- 已回收路径(入口 A、B、C 一律)只报告负值、绝不发信号:rp_kill_tree_snap(reaped=1) 只列出仍存活的快照成员,
  答 -EPERM;无快照入口 -EIO。消费者(np_run、e2stream、releaseexec、releaseprepare)见负值一律报
  "cleanup unconfirmed",不回退到裸 kill(-pid)(main 上 releaseprepare 曾如此,误杀在消费者侧会原样复活)。
- 断言改写(总数 36 -> 38,只增不减):
  - reaped-root:旧"孤儿被杀且 res==0";新"res<0 且孤儿未被发信号"(另加"从不报干净"一条)。理由:回收后无法证实归属。
  - snapshot-identity 第二轮:旧"真实 starttime 的成员被杀且 0";新"同样不发信号,答负值"。理由:starttime 复核与
    kill(2) 之间仍有 pid 复用窗口,只有 pidfd 能关。
  - np_run "orphan holding stdout":旧"被清理且 residue==0";新"residue<0、孤儿存活、文案含 cleanup unconfirmed"
    (这一条同时是消费者夹具:根已回收、持管道同组孤儿)。
  - reaped-group-reuse:新增答负值断言。
- 快照实现(d0bfc5852f)作为方案底稿保留在分支历史,补丁 ~/.claude/jobs/omp1/tmp/snapshot-design-v0.0.96.patch。

## 七、遗留:已回收根的孤儿清理(不进 0.0.96)

- 真正的方案是回收前取 pidfd 快照,设计稿在分支 pidfd-design 的 plans/pidfd-proc-cleanup.md(不进 0.0.96)。
- 0.0.96 的合同到此为止:已回收根只报告,不杀。
- 复核记录(2026-11-23,只读):已回收路径在库内无信号。消费者仍有"负值后裸 kill"或"回收后裸 kill(-pid)":
  tools/bg-candidate.cx:444-445(w==pid 已回收后 kill(-pid, 0/SIGKILL),与 a1 同形,未修);
  tools/cxlib/e2stream.cx:175、tools/affected.cx:676、682(负值后 kill(-pid)/sy_killpg,根未回收所以组号被僵尸根钉住,
  暂安全,但与"负值只报告"不一致);tools/affected.cx:1602 的 DEFERRED 行对 sv<0 不写 unconfirmed。
  tools/dvreact.cx:266、tools/prerun-drive.cx:1508 忽略返回值。这些在 0.0.96 之外,落地 pidfd 方案时一并处理。
- 缺测:库内无针对 gkfx f1-f4 与 gka c1/e1/s1 的入库夹具(tools/gkfx.cx 未跟踪且不属本分支,gka 在 7f46e032 的私有目录);
  a1 与 snapshot-overflow 已入库(check-reaper 40 项)。
