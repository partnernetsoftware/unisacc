# v0.1.4 交接完整性核对与规格对应索引

## 1. 起点材料齐备核对

| 材料 | 预期/所在路径 | 状态 | 备注 |
| --- | --- | --- | --- |
| `c3-v014-handoff.md` | 仓内/研究目录 | 缺失 | 主仓与 `v0.1.3` 工作树均未检索到该文件 |
| `c3-s3-v014-spec.tsv` (v2, 58 格) | `.claude/worktrees/v0.1.3/research/cdx/c3-s3-v014-spec.tsv` | 齐备 | 已复制到 `/tmp/c3-s3-v014/research/cdx/c3-s3-v014-spec.tsv` |
| `c3-s3-v014-check-design.md` | `.claude/worktrees/v0.1.3/research/cdx/c3-s3-v014-check-design.md` | 齐备 | 已复制到 `/tmp/c3-s3-v014/research/cdx/c3-s3-v014-check-design.md` |
| `c3-s3` 检查骨架 | `/tmp/c3-s3-v014/tools/check-proc-s3.cx` | 齐备 | 表驱动 58 行=58 断言，未提交仓内 |
| `c3-s3` 骨架补丁 | `research/agyneon/c3-s3-v014-skeleton.patch` | 齐备 | `git apply --stat` 通过（1 file, 245 insertions） |
| `rebase-plan-only-design.md` | `research/agyneon/rebase-plan-only-design.md` | 缺失 | 当前主仓 `research/agyneon/` 为新建目录，此前草稿未落盘于此 |
| `rebase-plan-v014-spec.tsv` (33 格) | `.claude/worktrees/v0.1.3/research/cdx/rebase-plan-v014-spec.tsv` | 齐备 | P01 至 P33 |

## 2. C3 S3 私有仓与重放命令

- 私有仓路径：`/tmp/c3-s3-v014`（基线 `df07b6801` + `git init`）
- 重放命令：
  `cd /tmp/c3-s3-v014 && sh ./unisacc.com tools/check-proc-s3.cx --`
- 最新实跑原行（基线 `df07b6801`）：
  - `PROC_S3_ROW id=38 event=sy_proc_wait rc=0 slot=FREE handle=REAPED release=1 ms=352 status=PASS reason=OK`
  - `PROC_S3_ROW id=39 event=sy_proc_run rc=0 slot=FREE handle=NONE release=1 ms=353 status=PASS reason=OK`
  - `PROC_S3_ROW id=40 event=px_run_timeout rc=0 slot=FREE handle=NONE release=1 ms=33 status=PASS reason=OK`
  - `PROC_S3_ROW id=41 event=px_run_timeout rc=0 slot=FREE handle=NONE release=1 ms=57 status=PASS reason=OK`
  - `PROC_S3_ROW id=43 event=rp_kill_tree_keep_root rc=-38 slot=LIVE handle=LIVE release=0 ms=0 status=FAIL reason=MISSING_API=rp_kill_tree_keep_root`
  - `PROC_S3_ROW id=45 event=rp_kill_tree_keep_root rc=-38 slot=LIVE handle=LIVE release=0 ms=0 status=FAIL reason=MISSING_API=rp_kill_tree_keep_root`
  - `PROC_S3_ROW id=46 event=rp_kill_tree_keep_root rc=-38 slot=LIVE handle=LIVE release=0 ms=0 status=FAIL reason=MISSING_API=rp_kill_tree_keep_root`
  - `PROC_S3_ROW id=53 event=rp_kill_tree_keep_root rc=-38 slot=LIVE handle=LIVE release=0 ms=0 status=FAIL reason=MISSING_API=rp_kill_tree_keep_root`
  - `CHECK_PROC_S3 rows=58 asserted=58 failed=54 unmeasured=0`（`exit=1`）

## 3. reaper API 缺口清单（对应 `df07b6801` 实现）

1. `ext/sysx/reaper.cx:84-114` (`kt_stat`)：当前将 `sy_open_read` / `sy_read` / 解析失败全部折叠返回 `-1`，未区分 `-ENOENT`、`-EACCES`、`-EIO` 与格式错。
2. `ext/sysx/reaper.cx:162-171` (`rp_kill_tree`)：当前 `if (kt_stat(root, ...) != 0) return 0;` 直接把首探测失败当已退出返回 `0`；且尚未新增并导出 `rp_kill_tree_keep_root`（确认身份前错误原样上浮、已钉住后才认 `ENOENT`/换代/`Z`/`X`）。

## 4. `c3-s3-v014-spec.tsv` (58 格) 与骨架对应表

| 规格 ID 范围 | 类别 | 骨架实现 (`tools/check-proc-s3.cx`) | 当前状态 |
| --- | --- | --- | --- |
| 01-37 | C0/C1/C2/C5/R3 原表 37 格 | 具名分派占位 | `FAIL reason=UNIMPLEMENTED` |
| 38-39 | B1 (`sy_proc_wait`/`sy_proc_run` `deadline_ms=0`) | `run_b1_wait` / `run_b1_run` | `PASS` (`ms=352/353`) |
| 40-41 | B2 (`px_run_timeout` 非树/树) | `run_b2_px_timeout` | `PASS` (`ms=33/57`) |
| 42 | C3 (`px_run_end` `poisoned_partial_exit`) | 具名分派占位 | `FAIL reason=UNIMPLEMENTED` |
| 43, 45, 46, 53 | R1/R2 (`rp_kill_tree_keep_root` 初探错误与 `ENOENT`) | 具名红占位 | `FAIL reason=MISSING_API=rp_kill_tree_keep_root` |
| 44, 47, 51, 52, 54-58 | R1/R2/C4 (`keep_root` 后续探测与清点) | 具名分派占位 | `FAIL reason=UNIMPLEMENTED` |
| 48-50 | R3/C0 (`EINTR` 截止与 `null_out` 缓存) | 具名分派占位 | `FAIL reason=UNIMPLEMENTED` |

## 5. `rebase-plan-v014-spec.tsv` (P01-P33) 对应表

| 规格 ID 范围 | 场景主题 | 对应设计维度 |
| --- | --- | --- |
| P01-P02 | 只读入口与禁用执行入口 | `--plan` 只读保护，无执行路径 |
| P03-P09 | 过期证据 5 场景、枚举 EIO、符号链接 | `out` 目录/文件类型/`lstat`/strict 校验 |
| P10-P14 | 新旧 REV 规范化、冻结根、strict source、祖先门 | 40 位小写 SHA、独立冻结根、merge-base |
| P15-P19 | 真实 SID 与 `tools/bound.c`、`release-ape.cx`、`core-closure.tsv`、helper 失败 | `UNISACC_IDENT_HELPER` argv 直调 |
| P20-P24 | 间接依赖闭包、路径精确相交、严格汇总 `FAIL=0`、非 PASS、闭包失败 | `affected.cx` + `batch-product-inputs.tsv` |
| P25-P28 | >512 路径、Git 输出截断、done 容量、保留池 10% 抽样 | `r.truncated` 拒绝与稳定排序抽样 |
| P29-P30 | 真实 W0118 (`C7->C10`) 回放与同 SID 文档对照 | 真实快照只读回放 |
| P31-P33 | 计划输出一致性、登记/棘轮、检查器独立验收 | `REBASE_PLAN_SUMMARY` / `REBASE_PLAN_WIN` |
