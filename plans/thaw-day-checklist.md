# 解冻日清单

release-post 之后按此顺序. 本文不实跑. 每步预期行都从源码读出.
跑工具前: `mkdir -p .claude/worktrees/w-<窗口名>/.work/tmp && export TMPDIR=$PWD/.claude/worktrees/w-<窗口名>/.work/tmp`

## (1) 放 freeze 锁

```
sh ./unisacc.com tools/devsvc.cx -- lock release freeze --owner cc
```

用法: tools/devsvc-core.cx:8. 成功行: tools/devsvc-core.cx:649 `OK\treleased\tfreeze`. 锁不在: 同文件 638 `ERR\tnot-held\tfreeze`. 属主不对: 同文件 639 `ERR\tnot-owner\tfreeze\towner=<持有者>`.

## (2) 冻结广播 end

```
sh ./unisacc.com tools/freeze-broadcast.cx -- end
```

用法: tools/freeze-broadcast.cx:3. 正文含 tools/freeze-broadcast.cx:188 `冻结广播 END`. 汇总: 同文件 289 `FREEZE_BROADCAST\tcmd=end\ttargets=<N>\tdry=0`, 同文件 339 `FREEZE_BROADCAST\tDONE\tok=<N>\tfail=0`. fail 非 0 则看 320 的 `FREEZE_BROADCAST_FAIL`.

## (3) 按窗口队列表 cherry-pick

`.claude/worktrees/w-ccogweco/.work` 里检索 window-commits 为 0 行,队列表尚未落地. 落地后按表中顺序,一行一个提交号:

```
git cherry-pick <表中提交号>
```

仓内工具没有 cherry-pick 成功行. 不要在表落地前填提交号,不要改顺序.

## (4) derive --check

```
sh ./unisacc.com tools/derive.cx -- --check
```

末行格式: tools/derive.cx:29 与 441 `DERIVE_SUMMARY\tok=<N>\tdrift=0\trefused=0\tskipped=0\telapsed_ms=<T>`. 退出码: tools/derive.cx:30,全清为 0. refused 或 TIMEOUT 为 2,不得当绿.

## (5) ratchets

```
sh ./unisacc.com tools/ratchets.cx
```

每项: tools/ratchets.cx:582 `RATCHET\t<name>\tok\t...`. 末行: 同文件 560 `RATCHETS\tok=<N>\tfail=0\telapsed_ms=<T>`. 有跳过时改用 559,带 `skipped=`. 退出码: 同文件 562,fail 非 0 则 1.

## (6) 恢复工作树符号链接

没有单独的恢复工具. 夹具写法: tools/check-devsvc-freeze.cx:196 `ln -s '<共享检出>/unisacc.com' '<工作树>/unisacc.com'`. 解冻后对每个仍在用的 `w-<窗口名>` 树:

```
ln -sfn "$PWD/unisacc.com" .claude/worktrees/w-<窗口名>/unisacc.com
```

该行不打印成功行. 只核链接存在且指向共享检出的 unisacc.com. 不把整仓解到 /tmp.

## (7) 分支与工作树清理

release-post 只打印,不执行. tools/release-post.cx:575 `POST_WORKTREE_CMD\t# see ... ; print-only, never execute`. 同文件 580 打印 `git worktree remove .claude/worktrees/v<本版>`. 同文件 581 配 tools/release-post.cx:65 `POST_STEP\t6\t<名>\tOK\tprinted-cleanup-cmds`.
只删已公开版本工作树. 不删 `w-<窗口名>` 树,不删未公开分支.
