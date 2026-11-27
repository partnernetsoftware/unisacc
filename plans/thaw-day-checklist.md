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

## 再冻结 (2026-11-27, 未实跑)

03:10 预跑停在 tools-compile-001 rc=2. tests/tools-compile.cx:315 对未登记分片 `die("REFUSE missing registered shard")`. 需要 202 片, 登记 201, 缺 tools-compile-201. 旧 0.1.5-FREEZE 已解除, 提交 3eae545df. 该提交不是版本号, 不是 C0a, 不是 C0b.

旧预跑目录与旧链证据作废. 第二轮链和全量预跑只用新的 FREEZE_CUT PASS 行里的 base 与 freeze_commit. 不复用 03:10 的 --out 目录.

本清单不跑 register, 不跑 --cut, 不跑链, 不跑预跑. 运行项 UNMEASURED.

### 登记行 (决策已定, 本清单不执行)

```
sh ./unisacc.com tools/register.cx -- --tool --sync
```

用法: tools/register.cx:5. 成功后 tests/batches.tsv 含 tools-compile-201, 使 tests/tools-compile.cx:315 不再 die. 文件头没有单独的 TOOL_SYNC 成功行, 不编造.

### 新 C0a / C0b

```
sh ./unisacc.com tools/freeze-cut.cx -- --check --version 0.1.5
sh ./unisacc.com tools/freeze-cut.cx -- --cut --version 0.1.5
```

--check 成功行: tools/freeze-cut.cx:470 `FREEZE_CUT\tCHECK_PASS\tversion=%s\tslot=%ld\tbase=%s\tfreeze_commit=%s`. --cut 成功行: 同文件 790 `FREEZE_CUT\tPASS\tversion=%s\tslot=%ld\tbase=%s\tfreeze_commit=%s`. 新 C0a 只取 PASS 的 base, 新 C0b 只取 PASS 的 freeze_commit. 后续命令只填这两段, 不填 3eae545df, 不填 03:10 旧 rev. 缺 --version 时同文件 837 拒绝 bad-version.

### 第二轮链与全量预跑

链目录名用新 freeze_commit (C0b). 预跑必须新开 --out, 不指向已作废的 03:10 目录.

```
sh ./unisacc.com tools/prerun-drive.cx -- plan --ids-file <新清单> --out <新预跑目录>
sh ./unisacc.com tools/prerun-drive.cx -- run --root <根> --candidate <候选> --out <新预跑目录>
```

用法: tools/prerun-drive.cx:3 与 5. 本工作树该文件头不打印单独成功汇总行. 旧目录通过行不能当第二轮证据. 运行项 UNMEASURED.

### keepalive 覆盖

本工作树没有 tools/release-keepalive.cx. 下列行读自共享检出, 正式链用那份.

```
sh ./unisacc.com tools/release-keepalive.cx -- 0.1.5 --rev <C0b> --rounds <N>
```

用法: 共享检出 tools/release-keepalive.cx:3. --rev 只收 7 到 40 位十六进制 (同文件 32). 覆盖链目录 chains/bg-b1/<rev> 与 prerun/v0.1.5-* (同文件 8 与 10). 新预跑目录必须落在该允许名单下, 否则标记写不到. 不要把已作废的 03:10 目录当成仍有效证据去续命.

每轮: 同文件 268 `KEEPALIVE_ROUND\tround=<r>/<N>\ttouched=<T>\tskipped=<S>\twall_s=<W>`. 末行: 同文件 285 `RELEASE_KEEPALIVE\tstatus=DONE\tversion=0.1.5\trounds_done=<N>\ttotal_touched=<T>\ttotal_skipped=<S>`. 撞截止则 status=DEADLINE, 退出仍为 0 (同文件 17), 不得当成覆盖完成. 标记文件名 `.keepalive-v0.1.5` (tools/check-release-keepalive.cx:21).
