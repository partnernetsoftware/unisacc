# prerun-start P1 启动与续跑包装收口记录 (0.1.2 车前)

## 背景与 P0/P1 拆分裁定

按 2026-11-25 P0 预备窗口裁定:
(1) P0 落地 `tools/prerun-start.cx` 的 D1 冻结入场门单件 (`admission`、`verify-admission`、`cold-identity`)，直接产出并校验 `warmup-cold-v2` 单批冷态收据供 T11 甲件 (`prerun-drive.cx plan/prepare`) 对接。
(2) `start` 与 `resume` 子命令转入 P1 (0.1.2 车前)，0.1.1 起跑使用已落 `main` 的 P0-(2) (`93a4a562e2`, `tools/prerun-inputs.cx` 参数对齐) 加起跑单兜底。完整 P1 草案补丁已归档于外部镜像 `prerun-start-p1-full.patch`。

## P1 关键设计要点 (已验证成果保留)

(1) 稳定 inode 内核 `flock` 跨子写手继承 (不设 `O_CLOEXEC`):
- `ps_lock_acquire` 打开 `DIR/run.lock` 时使用 `open(lock_path, O_RDWR | O_CREAT | O_NOFOLLOW, 0600)` (不加 `O_CLOEXEC`)，调用 `sy_flock(fd, 6)` (`LOCK_EX | LOCK_NB`)。
- `flock` 在内核中绑定于打开文件描述 (open file description)。由于不设 `O_CLOEXEC`，由 `np_run` (`ext/sysx/processcore.cx:77-108`) `fork`/`execvp` 启动的子写手进程 (`prerun-inputs`、`prerun-drive plan`、`prerun-drive run`) 自动继承该文件描述符引用。
- 若外层 `prerun-start` 包装进程被 `SIGKILL` (`kill -9`) 杀死而子写手仍在运行，内核保持 `flock` 不释放，新的 `prerun-start` 进程会被内核拒绝 (`already-running`)，从而杜绝残余子写手与新进程并写；待最后一个子写手进程退出后，内核自动关闭最后一份描述符引用并即刻释放 `flock`，无需任何用户态 `claim` 或墓碑回收协议。
- 持有者 `pid` 与 `started_ms` 仅写入 `run.lock` 供审计与 `--nowait` 拒绝时的 `holder=pid=...` 提示，不作用户态死活判据。

(2) 跨工作树/换树续跑迁移「先写后换」(`ps_write_fsync_rename`):
- `resume` 默认校验 `start-receipt.tsv` 冻结身份 (`repo`、`rev_sha`、`tree_sha`、`ape_sha256`、`windows_sha256`、`ids_sha256`)，不一致即拒绝。
- 显式传入 `--allow-migrate REASON` 时采用「先写后换」:
  - 先将当前活动 `start-receipt.tsv` 经临时文件 + `fsync` + `rename` 复制保留为 `start-receipt.tsv.orig.<seq>` (首次迁移同时保留最早 `start-receipt.tsv.orig`，后续迁移绝不覆盖最早 `.orig`，且不提前移走活动 `start-receipt.tsv`)；
  - 再将新绑定回执写入 `start-receipt.tsv.tmp.<pid>` 并 `fsync`，最后用 `sy_rename` 原子覆盖 `start-receipt.tsv` 并追加 `migration-ledger.tsv`。

(3) P1 必测夹具清单:
- 杀外层 wrapper (`kill -9`) 但子写手仍存活时，第二个 `prerun-start --nowait` 必须被拒 (`rc=2, reason=already-running`) 且无并写；
- 子写手进程组全部退出后，下一个 `prerun-start --nowait` 立即获锁成功 (`rc=0`)；
- 迁移替换前被杀续跑仍可用旧回执，替换后被杀续跑直接用新回执且无需二次授权，连续两次迁移保留 `.orig`、`.orig.1`、`.orig.2`。
