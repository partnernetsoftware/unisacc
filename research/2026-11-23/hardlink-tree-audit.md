# 全仓硬链接树审计与 `/tmp` 只读监视 (2026-11-23)

## 1. 背景与 `system-cleanup.sh` 白名单核对

2026-11-23 05:00:07 共享检出 `cx/` 下 276 个 `.cx` 文件权限位批量从 `100644` 变为 `100755`, 根因已确认为 `/home/devali/repos/moltbaby/systemd/scripts/system-cleanup.sh` 第 50 行对 `/tmp` 下超时 (>120min 未访问/未修改) 泄漏目录执行 `chmod -R u+rwx "$entry"` 时, 经硬链接树改写了共享检出文件的 inode 权限位。

核对 `/home/devali/repos/moltbaby/systemd/scripts/system-cleanup.sh` (L32-L56) 的 `/tmp` 顶层白名单:
- 受保护模式仅包含: `.*`、`tmux-*`、`rmux-*`、`ssh-*`、`systemd-*`、`*.sock`、`*.lock`、`omp-workflow-sup-*`、`unisacc-e3cache-*`、`unisacc-bound-*`、`unisacc-cxbin-*`、`cand-ident-retire-final.patch`、`cand-ident-retire-gate.txt`。
- **结论: `sessionroot` (`unisacc-session-*`、`unisacc-affected-*`, 定义于 `tools/cxlib/sessionroot.cx`) 不在 `system-cleanup.sh` 白名单内, 不受保护。**
- 因此, 凡在 `/tmp` (含 `sessionroot`) 下创建指向仓内文件的硬链接树, 只要异常退出泄漏并超过 120 分钟, 就会触发 `chmod -R u+rwx` 污染仓内 inode 权限位。改路径到 `sessionroot` 无法消除风险, 根治办法是**彻底禁止在 `/tmp` 下对仓内文件建硬链接树 (一律改为拷贝或 `git archive`)**, 同时系统侧 `system-cleanup.sh` 的 `chmod -R u+rwx` 应改为仅针对目录 (`find "$entry" -type d -exec chmod u+rwx {} +`)。

## 2. 全仓硬链接调用点审计表

覆盖全仓 `git grep` 命中的全部 `sy_link` / `sy_linkat` / `link(` / `cp -al` / `ln` (共 17 个工具与模块, 包含指定复核的 12 个工具):

| 工具/模块 | 调用位置 | 硬链接什么 | 清理路径 | 是否在 `/tmp` 直下 | 泄漏风险 | 建议 |
| --- | --- | --- | --- | --- | --- | --- |
| `tools/precommit.cx` | L1734 `sy_link`; L2618/2631 `precommit-tree.XXXXXX` | `cov_link_dir` 硬链接 `tests/` 下普通文件到 `precommit-cov-<pid>-<tag>/tests/`; 隔离树在 `${TMPDIR:-/tmp}/precommit-tree.XXXXXX` 建工作树与软链 | `cov_clean` / 退出钩子, 异常/SIGKILL/超时易泄漏 | 是 (`precommit-cov-*` / `precommit-tree.*` 均无白名单) | **极高 (已发生真实事故, 234 个泄漏目录)** | `agy-flow` 整改中 (本次不动) |
| `tools/check-old-product-material.cx` | L34 `fsx_scratch("check-opm")`; L93 `cp -al` | **把共享检出整棵 `cx`、`cxroot.tsv`、`ext`、`include`、`tests`、`exec`、`build`、`src`、`apps`、`seed`、`weights`、`iterate`、`ujs`、`tools` 全部 `cp -al` 硬链接进 `layout-a`/`layout-b`** | `layout-a` L112 手动 `rm -rf`; `layout-b` 仅靠 `fsx_scratch` 的 `sy_atexit(fsx_sc_atexit)` 删除; 遇 `SIGKILL`/超时/崩溃即泄漏 | 是 (`sy_tmpdir()/check-opm*`, 无白名单) | **极高 (直接硬链接 276 个 `cx/` 文件与全仓源码, 与本次事故同源)** | **立即改拷贝: 将 L93 `cp -al` 改为 `cp -a` 或 `git archive HEAD \| tar -x`, 禁止硬链接仓内树** |
| `tools/check-cx-sys.cx` | L339-341 `sy_linkat`; L480-482 `sy_link` | 仅硬链接私有临时目录 `root` 下自建测试文件 (`a->b`, `w3/c->w3/hard`), 不碰仓内文件 | 退出时删临时目录 | 是 | 无仓内污染风险 | 保持 |
| `tools/check-cx-sys-spawn.cx` | L97 `ln /tmp/cx-sys-dev-a /tmp/cx-sys-dev-b` | 仅硬链接自建空文件, 不碰仓内文件 | L102 `sy_unlink` 清理 | 是 (固定路径) | 无仓内污染风险 | 保持 (可选低优先改 `sy_mkdtemp`) |
| `tools/check-dvqualify.cx` | L347 `ln ev alias` | 仅硬链接私有 `tmp` 下自建 evidence 文件 (`ev -> ev-hard`), 不碰仓内文件 | 夹具结束清 `tmp` | 是 | 无仓内污染风险 | 保持 |
| `tools/check-driver-output.cx` | L38/71 `hardlink(path, other)` | 仅硬链接 `/tmp/cxoXXXXXX` 下自建 `inode -> alias`, 不碰仓内文件 | 退出清 `dir` | 是 | 无仓内污染风险 | 保持 |
| `tools/check-push-replay.cx` | L73/194/404/456 | **无硬链接**, 均为 `sy_symlink`/`ln -s` 指向 `unisacc.com` | 夹具清理 | 是 | 无硬链接污染风险 | 保持 |
| `tools/check-relscan.cx` | L71 | **无硬链接**, 仅 `ln -s lib/base.cx alias.cx` | 夹具清理 | 是 | 无硬链接污染风险 | 保持 |
| `tools/check-s9-shards.cx` | L236/297 | **无硬链接**, 仅 `ln -sfn` / `symlink` | 夹具清理 | 是 | 无硬链接污染风险 | 保持 |
| `tools/merge-version.cx` | L370-372 | **无硬链接**, 仅 `ln -sf` 软链 `unisacc.com` | `--work` 目录 | 取决于 `--work` | 无硬链接污染风险 | 保持 |
| `tools/check-devsvc.cx` | L108/615 | **无硬链接**, 仅 `ln -s`/`ln -sf` 软链 `unisacc.com` | 夹具清理 | 是 | 无硬链接污染风险 | 保持 |
| `tools/check-flowadmit.cx` | 无 | **无硬链接** (仅变量名 `ln`) | - | - | 无 | 保持 |
| `tools/std-surface-v2.cx` | L109/115 | **无硬链接调用** (仅 API 名单字符串 `"sy_linkat"`) | - | - | 无 | 保持 |
| `tools/prerun-drive.cx` | L2108 `sy_link(paths[i], dest[i])` | 同归档目录 `.prerun-archive-XXXXXX` 暂存新文件 -> 原子发布到 `dest[i]` | L2113 立即 `sy_remove_all(staging)`, `nlink` 归 1 | 否 (目标归档目录内) | 无 | 保持 |
| `tools/session-mirror.cx` | L81 `sy_link(from, to)` | 硬链接 `$HOME/.claude/projects/-home-devali-repos-unisacc` 会话日志到 `$HOME/.local/state/unisacc/claude-project-mirror/gen-N/` | 超过 3 天自动 prune `MIRROR/gen-*` | 否 (`~/.local/state/`) | 无仓内污染风险 (不碰仓内文件, 不在 `/tmp`) | 保持 |
| `tools/sim.cx` | L120 `sy_linkat` | 同目录 `no-replace` 原子发布 `write-events` 新文件 | L121 立即 `unlink(tmp)`, `nlink` 归 1 | 否 | 无 | 保持 |
| `cx/cx0/plan.cx` & `cx/runtime/runexec.cx` | L66/73 `cx0_link`; L425 `sy_linkat` | 新建接口/捕获文件的 `no-replace` 原子发布 | 紧跟 `unlink(tmp)` / `sy_unlink(from)`, `nlink` 归 1 | 否 | 无 | 保持 |

### 关键结论
除了 `agy-flow` 正在整改的 `tools/precommit.cx` 之外, 全仓**唯一**另一个在 `/tmp` 直下对仓内目录树建硬链接的危险点是 **`tools/check-old-product-material.cx:93`** (`cp -al` 硬链接整个 `cx/ src/ tools/ tests/ ...` 到 `/tmp/check-opmXXXXXX/layout-{a,b}`), 应优先改为拷贝 (`cp -a` 或 `git archive HEAD | tar -x`)。

## 3. `/tmp` 硬链接树只读监视与并入 `team-pulse --freeze-watch` 建议

已编写纯 `.cx` 只读扫描原型 (遍历 `/tmp` 顶层非隐藏目录, 递归深度 <= 8, 用 `lstat` 统计含 `S_ISREG && st_nlink > 1` 的 `.cx` 文件的顶层目录数 `n` 与最老目录年龄 `oldest_min`, 输出 `FREEZE_WATCH\tHARDLINK_TREES\t<n>\t<oldest_min>`):
- 实测本机当前 `/tmp`: `FREEZE_WATCH	HARDLINK_TREES	0	0`
- 正例自测 (独立临时根下建硬链接 `.cx`): `FREEZE_WATCH	HARDLINK_TREES	1	0` (已验证命中并清理)
- **L0 缺口记录**: `cx/sys/fs.cx` 的 `sy_stat_t` 目前不含 `nlink` 字段, 扫描器临时使用隐式 prelude 的 `lstat` + `sb.st_nlink`; 建议后续给 `sy_stat_t` 增补 `long nlink`。
- **并入 `tools/team-pulse.cx` (`--freeze-watch`)**: 可由 `csr` 直接在 `--freeze-watch` 末尾追加一行 `FREEZE_WATCH\tHARDLINK_TREES\t<n>\t<oldest_min>` (`n > 0` 时一并打印 `HARDLINK_DIR` 详情或升格告警, 且按 `n > 0` 提前拦截整点清理风险)。

### 参考只读实现 (可直接并入 `tools/team-pulse.cx --freeze-watch`)

```c
struct hl_scan { int count; char sample[512]; };
static int hl_ends_cx(const char *name, long n) {
    return n >= 3 && name[n - 3] == '.' && name[n - 2] == 'c' && name[n - 1] == 'x';
}
static void hl_walk(const char *path, int depth, struct hl_scan *ctx) {
    if (depth > 8) return;
    sy_dir_t d;
    if (sy_dir_open(&d, path) != 0) return;
    while (sy_dir_next(&d) == 1) {
        if (tx_eqz(d.name, ".") || tx_eqz(d.name, "..")) continue;
        char child[1024];
        if (cx_fmt(child, sizeof child, "%s/%s", path, d.name)) continue;
        struct stat sb;
        if (lstat(child, &sb) != 0) continue;
        if (S_ISDIR(sb.st_mode)) hl_walk(child, depth + 1, ctx);
        else if (S_ISREG(sb.st_mode)) {
            long nl = (long)tx_len(d.name, 256);
            if (hl_ends_cx(d.name, nl) && (long)sb.st_nlink > 1) {
                if (ctx->count == 0) cx_fmt(ctx->sample, sizeof ctx->sample, "%s", child);
                ctx->count++;
            }
        }
    }
    sy_dir_close(&d);
}
static void freeze_watch_hardlink_trees(const char *tmp_root, long now_s) {
    sy_dir_t d;
    if (sy_dir_open(&d, tmp_root) != 0) { printf("FREEZE_WATCH\tHARDLINK_TREES\t-1\t0\n"); return; }
    int n_dirs = 0; long oldest_min = 0;
    while (sy_dir_next(&d) == 1) {
        if (d.name[0] == '.') continue;
        char top[1024]; struct stat st;
        if (cx_fmt(top, sizeof top, "%s/%s", tmp_root, d.name)) continue;
        if (lstat(top, &st) != 0 || !S_ISDIR(st.st_mode)) continue;
        struct hl_scan ctx; mm_fill(&ctx, sizeof ctx, 0, sizeof ctx);
        hl_walk(top, 0, &ctx);
        if (ctx.count > 0) {
            long age_min = (now_s > (long)st.st_mtime) ? (now_s - (long)st.st_mtime) / 60L : 0L;
            if (n_dirs == 0 || age_min > oldest_min) oldest_min = age_min;
            n_dirs++;
            printf("HARDLINK_DIR\t%s\tcx_nlink_gt1=%d\tage_min=%ld\tsample=%s\n", top, ctx.count, age_min, ctx.sample);
        }
    }
    sy_dir_close(&d);
    printf("FREEZE_WATCH\tHARDLINK_TREES\t%d\t%ld\n", n_dirs, oldest_min);
}
```
