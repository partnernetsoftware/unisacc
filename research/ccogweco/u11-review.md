# U11 复审结论(只读)

违规内容:tools/check-status.cx 的 git 段断言用自造文件代替真实 git 行为——`logs/refs/stash` 由夹具直接写两行再断言 `stash=2`(7cbc7b58bd 引入),3510d7c60b 又在 worktree 用例里 `touch .git/worktrees/wt/index.lock` 后断言 `present`/`stash=2`,夹具与实现内部路径写法绑定,而不是驱动真实 git stash 或真实锁竞争。
现状仍在:当前 tools/check-status.cx:65-92 与 :113-128 保持这套自造文件写法,后续提交只追加未改写;复审判断它不掩盖产品错误(3510d7c60b 同时把 status.cx/affected.cx 改为 `git rev-parse --git-path` 解析,属于正确修法),故按"只读复审"记录结论关闭该声称,另记一条非阻塞 gap:后续可把 stash/index.lock 断言换成真实 git 操作。
证据:7cbc7b58bd(初版夹具)、3510d7c60b(worktree 断言,git show -- tools/check-status.cx)、tools/check-status.cx:65-92,113-128、build/state-unverified.tsv:U11。
