# 独占运行与显式状态恢复

`csih.sh agent-owned DIR SESSION HASH` 使用现有绝对路径、本人0700的每会话目录。锁为 `DIR/owner.lock`，日志为 `DIR/journal.jsonl`；旧 `agent`/`run` 保持原行为，不因此受到保护。HASH 是用户显式提供的64位十六进制绑定值，本阶段未验证它对应实际运行源码或候选产物。

空闲时输入 `/export-state HANDOFF` 保存 `DIR/state-v2.json`。控制命令本身不发送、不进入历史，保存失败保留输入。快照包含历史、草稿、队列、目标与自动执行预算；执行中的请求不能导出。

`csih.sh resume-agent DIR STATE SESSION HANDOFF HASH` 先取进程记录锁，核对身份与本DIR固定日志绑定，再同步检查日志长度等于快照offset。恢复cwd、role、peer为明确启动上下文，设置CSIH_CWD/CSIH_ROLE/CSIH_PEER白名单，并通过生产agent_role_configure同步agent角色getter；实测跨TU setenv更新不一致，不能只凭setenv声称所有运行层已更新。TTY raw准备成功为提交点，之后只消费一次状态文件；消费失败明确报告恢复已提交，不能盲目重新应用。激活前失败保留状态文件。

运行容量采用真实agent限制：journal路径小于512字节，cwd小于1024字节且为可进入的绝对目录，peer最多48字节且仅字母数字及冒号/下划线/连字符。更宽的存储schema不意味着运行时可以截断。

锁只约束参与协议的进程；同进程不得重复打开该锁文件。fork子进程不继承记录锁，exec关闭锁fd。此入口不是跨exec热换版或候选身份验证，也未升级任何旧运行窗口。恢复后存在队列时会开始后续模型执行，请显式配置期望endpoint。私有PTY与localhost HTTP stub已实际验证自动pending提示、严格pwd正文、write/peer与本DIRjournal、未发草稿和后续编辑保留、left0自然disarm；证据见/tmp/csih-resume-pending-parent.json。没有调用真实模型/API/邮件，多pending与setter全部拒绝边界仍未覆盖。
