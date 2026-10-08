# 冻结候选构建与校验

`python3 reload_candidate.py build SOURCE_APP COMPILER PRIVATE_ROOT` 要求现有本人0700私有根目录、本人普通编译器及无链接源码树。新目录独占创建，不覆盖已有候选。冻结全部递归 `.c`/`.h`/`.inc`，包含子目录；引用越界、本地include使用其它扩展或非字面include均拒绝。标准尖括号头由被冻结compiler内嵌提供；不支持额外编译输入、第三方头搜索路径或自定义argv。

身份是排序相对路径与各SHA256清单的canonical摘要。compiler单独复制并绑定SHA256。保留旧十五源argv，真实compiler写出候选，并运行TUI完整selftest；另构建agent CLI运行其selftest。每命令14秒看门狗，超时杀所属进程组；两项门禁均须rc0、无FAIL、末行成功标记、stderr为空，输入及产物前后哈希不变，才原子写0600收据并输出machine JSON。门禁采用私有HOME、净role/peer/proxy、localhost拒网endpoint与dummy key，不发真实API或邮件。

`python3 reload_candidate.py verify CANDIDATE_DIR` 重算源码、compiler与两个产物，核对严格收据schema、确切argv/cwd、required_gates、各rc、保存的stdout/stderr及其SHA256。未有收据、门禁失败、未知字段、重复JSON键、未知候选条目、混合或改动输入均不能当作就绪候选。

可信祖先目录、本人私有目录和单写者是前提；SHA256和收据一致性不是不可伪造的认证，也不证明收件人身份或当前进程已接管。仅本机TUI/agent单元门禁，不能代替全部仓库或跨平台发版门禁。本driver不启动owned会话、不做跨exec交接，不改旧窗口；稳定TTY监督者与launcher仍待后续实现。
