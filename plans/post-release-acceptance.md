# 发布后验收

未实跑. 每步一条命令, 预期行从本工作树源码读出. 运行项 UNMEASURED.

## (1) 公开资产是预期集合

```
gh release view <tag> -R <repo> --json isDraft,tagName,targetCommitish,assets --jq '.isDraft, .tagName, .targetCommitish, .assets[].name'
```

字段: tools/cxlib/releaseexec.cx:357. 前三行必须是 `false`, `<tag>`, `<commit>` (同文件 361, draft 实参为 `"false"`). 其后每一行必须含 candidate 未跳过的资产名. 七个角色列: tools/cxlib/releasecandidate.cx:34 `ape` `package` `win_arm64` `record` `build_alias` `zip` `checksums`. 缺一个则 draft-check 拒绝 `draft lacks asset` (releaseexec.cx:561). 公开读回用同一 view, isDraft 必须为 false (同文件 629).

## (2) notes 正文

发布写入用 `gh release edit <tag> --notes-file <notes>` (tools/cxlib/releaseexec.cx:615-617). notes 路径来自配置键 `notes` (同文件 3, 585). 源码没有单独的 notes 读回成功行. 对照命令:

```
gh release view <tag> -R <repo> --json body --jq .body
```

预期是该 notes 文件原文. 这一对比没有工具成功行, 标 UNMEASURED.

## (3) version-seq 行

```
sh ./unisacc.com tools/release-chain.cx -- mark-version-seq --version <V> --repo <仓> --dry
```

用法: tools/release-chain.cx:1054. 已公开时成功行: 同文件 1196 `VERSION_SEQ_MARK\tDRYRUN\tversion=<V>\tchange=ALREADY_PUBLIC\ttable=<path>`. AFTER 行第 4 列 (下标 3) 为 `已公开` (同文件 1119, 1147). 行数不是 1 则拒绝 `VERSION_SEQ_MARK version= hits=` (同文件 1170).

## (4) 根已升级

```
sh ./unisacc.com tools/root-current.cx -- --no-candidate-ahead
```

用法: tools/root-current.cx:3. 成功行: 同文件 717 `ROOT_CURRENT\tPASS\texpect=<V>\texpect_sha=<64>\tactual=<V>\tactual_sha=<64>\tsource=<ver_src>:<sha_src>`. expect 与 actual 的版本和 sha 必须相同. 解冻门要求该行且不含 `WARN=` (tools/release-post.cx:759). 根升级步骤成功行: 同文件 65 配 570 `POST_STEP\t5\troot-upgrade\tOK\troot-upgraded`.

## (5) 解冻完成

```
sh ./unisacc.com tools/release-post.cx -- thaw --version <V> --dry
```

用法: tools/release-post.cx:60. 已解冻且复核过时: 同文件 825 `POST_THAW_GATE\tPASS\t<V>-FREEZE`, 同文件 826 配 65 `POST_STEP\t8\tthaw-freeze\tOK\talready-thawed-verified`. 根不匹配则 804 `POST_THAW_GATE\tFAIL`.

## (6) 锁已放

```
sh ./unisacc.com tools/devsvc.cx -- lock release freeze --owner <持有者>
```

放锁成功行: tools/devsvc-core.cx:649 `OK\treleased\tfreeze`. 已经不在则同文件 638 `ERR\tnot-held\tfreeze`. 事后只读: `lock list`, 成功行 652 `OK\tlocks\tn=<N>`, 且没有 `LOCK\tfreeze` (同文件 654).

## (7) END 广播

```
sh ./unisacc.com tools/freeze-broadcast.cx -- end
```

汇总: tools/freeze-broadcast.cx:289 `FREEZE_BROADCAST\tcmd=end\ttargets=<N>\tdry=0`. 完成: 同文件 339 `FREEZE_BROADCAST\tDONE\tok=<N>\tfail=0`. fail 非 0 则退出 1 (同文件 346).

## (8) 工作树符号链接已恢复

源码没有恢复成功行. 夹具写法: tools/check-devsvc-freeze.cx:196 `ln -s '<共享检出>/unisacc.com' '<工作树>/unisacc.com'`. 命令:

```
ln -sfn <共享检出>/unisacc.com .claude/worktrees/w-<窗口名>/unisacc.com
```

该命令不打印成功行. 只核链接指向共享检出的 unisacc.com. 标 UNMEASURED.
