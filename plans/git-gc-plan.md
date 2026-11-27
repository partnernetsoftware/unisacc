# git gc 停用: 测量与计划

测量日: 2026-11-27. 仓库: `/home/devali/repos/unisacc`. 只读测量, 未跑 `git prune` 或 `git gc`.

自动 gc 停用的原因是 `.git/gc.log` (87 字节):

```
warning: There are too many unreachable loose objects; run 'git prune' to remove them.
```

每次 `git fetch` 都会打印 "The last gc run reported: too many unreachable loose objects; remove .git/gc.log".

## 测量

`git count-objects -vH`:

- count: 9443
- size: 154.92 MiB
- in-pack: 138661
- packs: 14
- size-pack: 105.68 MiB
- prune-packable: 0
- garbage: 0
- size-garbage: 0 bytes

`.git` 共 305M. `.git/objects` 263M, 其中 pack 目录约 106M (与 size-pack 一致). `.git/worktrees` 31M, 15 个窗口. `.git/refs` 20K. `.git/logs` 4.5M.

`git fsck --unreachable --no-reflogs` 退出 0, 11505 行:

- unreachable blob: 5200
- unreachable commit: 633
- unreachable tree: 5672

其中松散对象 9372 个, 另有 2133 个已在 pack 里或没有对应松散文件. 松散不可达对象的文件 mtime 全部小于 30 天:

- 小于 1 天: 530 个, 12279099 字节
- 1 到 7 天: 5250 个, 69642965 字节
- 7 到 30 天: 3592 个, 57233316 字节
- 30 到 90 天: 0
- 大于 90 天: 0

松散不可达提交 192 个, 提交者日期也全部小于 30 天. 633 个不可达提交按提交者日期: 小于 30 天 591 个, 大于等于 30 天 42 个.

## 只经 reflog 或窗口树还能摸到的提交

`git fsck --unreachable --no-reflogs` 的 633 个提交, 对照 `git rev-list --reflog --all` (16537 个) 和 10 个窗口的 `logs/HEAD`:

- 也在 reflog 历史上: 407
- 其中窗口 reflog 原文里出现的 oid: 364, 没有超出上面的 407
- 当前窗口 HEAD: 0 个落在这 633 里
- reflog 与窗口日志都摸不到: 226

这 407 个是 "去掉 reflog 就不可达, 但 reflog 或窗口日志还指着" 的提交. 226 个连 reflog 历史也到不了, 只作为其它不可达对象的祖先或悬空提交存在.

抽查:

- `870868908b` 在不可达提交名单里. 它不是 HEAD 的祖先.
- `4f278f6ea` 不在不可达名单里, 是 HEAD 的祖先.
- `8773b0e06` 对象还在, 不在不可达提交名单里, 仍被某个 ref 指着.

证据引用: 在 `plans/`, `prd_mempalace.md`, `release/`, `skills/`, `build/` 里用 9 字符前缀搜这 633 个提交, 命中 42 个前缀. 所以不能按 "不可达" 直接删. 命中面没有覆盖信封目录和各窗口私有 `.work`, 那部分以 bundle 兜住, 不靠这次搜索宣称已穷尽.

## 计划, 现在不执行

测量结论: `git gc --prune=30.days` 删不掉当前这批松散不可达对象, 因为它们的 mtime 全部小于 30 天. 只跑这条, `.git/gc.log` 的警告还会在. 要消警告, 必须先把小于 30 天的不可达提交打进仓外 bundle, 校验通过之后, 再单独决定是否用更短的 prune 期限. 这一步不在本次授权里.

命令, 按顺序, 本次不跑:

(1) 把小于 30 天的 591 个不可达提交 id 写到仓外, 再打 bundle. 42 个大于等于 30 天的也一并打进同一个 bundle, 数量小, 避免证据落在旧提交上.

```
mkdir -p ~/.cache/unisacc/branch-archive
OUT=~/.cache/unisacc/branch-archive/unreach-lt30-2026-11-27.bundle
git bundle create "$OUT" --stdin < young-and-old-unreach-commits.txt
git bundle verify "$OUT"
```

`git bundle create --stdin` 若拒绝无范围的单个提交, 改用临时 ref, 仍不推送:

```
while read c; do git update-ref "refs/archive/unreach/$c" "$c"; done < young-and-old-unreach-commits.txt
git bundle create "$OUT" $(git for-each-ref --format='%(refname)' refs/archive/unreach)
git bundle verify "$OUT"
git update-ref -d 逐个删除 refs/archive/unreach, 或 git for-each-ref 后删除
```

(2) bundle 校验通过之前, 不删 `.git/gc.log`, 不跑 gc.

(3) 校验通过之后, 按授权跑:

```
git gc --prune=30.days
```

预期: 当前 9372 个小于 30 天的松散不可达对象仍会留下, 警告不会消失. 这是测量结果, 不是失败. 若之后要清警告, 须另一次授权, 且只能在 bundle 已校验之后使用更短的 `--prune`, 不能在本计划里直接 `git prune`.

回滚: bundle 在 `~/.cache/unisacc/branch-archive/`, 不在仓库里. 若 gc 之后发现缺对象, 用 `git bundle unbundle` 或 `git fetch` 该 bundle 把提交取回, 再 `git update-ref` 挂回临时 ref. 本计划不删除该 bundle.

## bundle 回执, 2026-11-27

已执行安全的一半. 未跑 `git gc`, 未跑 `git prune`, 未删 `.git/gc.log`.

路径: `/home/devali/.cache/unisacc/branch-archive/unreach-2026-11-27.bundle`

sha256: `2cb385febedde528e582495c6005c35ff4f4ffff61ebedb62288dfce0e6196b2`

大小: 92969698 字节.

`git bundle verify` 退出 0, 文案 `is okay` 与 `The bundle records a complete history.` 临时 ref `refs/archive/unreach` 已删除, `REFS_LEFT=0`.

内容: 633 个不可达提交, 每个一个 tip. 其中 reflog 还能摸到的 407 个是这 633 的子集, 已包含. `git bundle list-heads` 的 9 字符前缀与这 633 个提交差集为 0.

证据前缀: 在 `plans/`, `prd_mempalace.md`, `release/`, `skills/`, `build/` 里命中的 42 个 9 字符前缀全部在 bundle 的 tip 里. `EVIDENCE_IN_BUNDLE 42`, `EVIDENCE_MISSING 0`.

账本: `plans/omp-grk-ledger.tsv` 行 `GIT-GC-BUNDLE`.

gc 本身由 cc 排到正规链结束之后. 本回执不授权 `git gc --prune=30.days`.
