# S10 旧工作树清理表(只读)

日期: 2026-11-23; 范围: v0.0.88、v0.0.89-old、v0.0.93; ujs 不碰。未删树、未提交本页。

口径说明: CC 写的 `git log 分支 --not --remotes --not main` 里第二个 `--not` 会把 main 重新变成正起点,计数虚高(~26)。下表同时给出字面计数与正确口径 `git log 分支 --not --remotes main`(排除所有 remote-tracking 与 main)。

## 结论表

| 工作树 | HEAD | tip 日期/作者 | status | 未跟踪重要 | 字面 only | 正确 only | tip 在 origin? | tip 在 main? | 脏区性质 | 结论 |
|---|---|---|---|---|---|---|---|---|---|---|
| v0.0.88 | afca1d45cf | 2026-11-21 wanjo | 2 M | 无 | 26 | 0 | 是 origin/v0.0.88 | 否(但 patch-id=main 的 0090ca3614) | prerun-drive/check 未提交草稿; main 已含同功能且更大(1616 vs 2985 行) | 可安全删除工作树 |
| v0.0.89-old | d4f291aafc | 2026-11-21 wanjo | 9 M | 无 | 27 | 1 | 否(无 origin/v0.0.89-old) | 否(同主题已以 472047c16e 入 main,已演进) | cxmod iscxunit + exec/parse2 生成表; iscxunit 已在 main,parse2 为过时局部 regen | 可安全删除工作树; 无需为保全内容先推(可选考古推 tip SHA) |
| v0.0.93 | 326dfc0e33 | 2026-11-22 wanjo | 4 M + 2 ?? | uaf-census 两文件(main 已有更新版) | 26 | 0 | 是 origin/v0.0.93 | 是(祖先) | out-gc 脏区短于 main(177 vs 301); phase-times 工作区=main; uaf 未跟踪为旧稿 | 可安全删除工作树 |

三树作者窗均为 wanjo; 体积约 222M / 205M / 221M。

## 逐项核对

### v0.0.88
- 路径: `.claude/worktrees/v0.0.88`; 分支 `heads/v0.0.88`
- `git -C status --short`: `M tools/check-prerun-drive.cx`, `M tools/prerun-drive.cx`
- tip: `afca1d45cf` SURF macOS timeout 修复; 已在 `origin/v0.0.88`; 与 main `0090ca3614` patch-id 相同
- vs main: 1 commit not in main(即 tip),但等价补丁已在 main
- 脏区: warmup 取最轻批 + `pd_prebuild_layout` 等; **不是无意义产物**,是未提交功能草稿,但已被 main 后续演进覆盖 -> 删树丢弃无独特损失
- 此前「有未提交改动」属实; 现核为过时草稿

### v0.0.89-old
- 路径: `.claude/worktrees/v0.0.89-old`; 分支 `v0.0.89-old`(无上游)
- status: 9 M — `cx/compiler/cxmod.cx`, `cx/cx0/plan.cx`, `src/cxmod.c`, `tools/check-cxmod-tri.cx`, 及 5 个 `exec/parse2/*.tsv`
- tip: `d4f291aafc` check-cx-suffix-sites + tests/cxpath; **未推该分支名**; main 有同主题 `472047c16e`(检查与 expect 已演进,非同一 patch-id)
- 正确 only-local=1(即 tip); 内容保全不依赖再推该 tip
- 脏区: `cxm_iscxunit` 主体已在 main; parse2 增行相对 tip 的局部生成,main 上 `builtin.all` 等已更完整 -> 遗留 WIP/生成物,非待抢救独特提交
- 此前「有未提交改动」属实; 现核为遗留 WIP+生成表

### v0.0.93
- 路径: `.claude/worktrees/v0.0.93`; 分支 `heads/v0.0.93`
- status: M `out-gc`/`check-out-gc`/`phase-times-calc`/`check-phase-times-calc`; ?? `uaf-census.cx`/`check-uaf-census.cx`
- tip: `326dfc0e33` 已在 main 与 origin/main/origin/v0.0.93; only-local(正确)=0
- 未跟踪: uaf 两文件 main 已有(更新于 e30496b653); WT 版更旧
- 脏区相对 tip 的改动: phase-times 工作区已与 main 相同; out-gc 工作区短于/旧于 main -> 可丢

## 删除建议(等下令)
1. 三棵均可 `git worktree remove` 工作树目录; 远端保留 `origin/v0.0.88` 与 `origin/v0.0.93` 即可考古。
2. `v0.0.89-old` 无远端; 若要留 tip SHA 指纹可先 `git push origin d4f291aafc:refs/heads/v0.0.89-old`,非内容保全必需。
3. 删前不必提交/stash 脏区(已判定为过时草稿或生成物)。
4. ujs 不在本表,不碰。

未下令,不执行删除。

## 备份(2026-11-23, 持久目录, 不入仓)

目录: `~/.local/share/unisacc/archive-worktrees/2026-11-23/`
回滚办法: 同目录 `README.md`(从 tar 铺脏文件; 从 bundle 恢复 v0.0.89-old tip)。

| 资产 | bytes | sha256 |
|---|---|---|
| v0.0.88-dirty.tar.gz | 25465 | 743449c4db9061c1b216514c60031c6112c157b514a0edcc96928de558b22d19 |
| v0.0.89-old-dirty.tar.gz | 114382 | 12bc1b863575c787f8476d223d7ae9257accfcc35405564457f8b65e63b7bd43 |
| v0.0.93-dirty.tar.gz | 26062 | a1abb55e4f63056a2c50c6a7c72c8eb35087018ed4b74a62471a4aa258b2a510 |
| v0.0.89-old-tip.bundle | 4384 | f73233c7d67b69b8efc17848f96440bd8fdf7206e45921319a0ad6cd928cf386 |
| v0.0.89-old-tip-not-origin-main.bundle | 4384 | f73233c7d67b69b8efc17848f96440bd8fdf7206e45921319a0ad6cd928cf386 |

注: 两份 bundle 内容相同(tip 相对 origin/main 仅 1 提交,前提 parent=`c4db3d07f98f57d438c979e16679980aad17f622`)。tip=`d4f291aafc0b201a2da2a25e3774060961c5e53f` 已记入 `v0.0.89-old.manifest.txt`。

## S10 删除命令(只在下令后执行;执行前再核 tar sha)

```
ARCH=$HOME/.local/share/unisacc/archive-worktrees/2026-11-23
# 执行前核验(须与上表一致)
sha256sum $ARCH/v0.0.88-dirty.tar.gz $ARCH/v0.0.89-old-dirty.tar.gz $ARCH/v0.0.93-dirty.tar.gz $ARCH/v0.0.89-old-tip.bundle
# 三条(ujs 不碰)
git worktree remove --force .claude/worktrees/v0.0.88
git worktree remove --force .claude/worktrees/v0.0.89-old
git worktree remove --force .claude/worktrees/v0.0.93
git worktree list
```

未下令,不执行删除。备份已完成。
