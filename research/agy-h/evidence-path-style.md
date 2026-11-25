# R37/0.0.106 发布前核对: 已发布证据路径写法与 ape-receipt ~/.cache 判据

日期: 2026-11-25
执行窗口: agy-h (只读核对)
核查范围: `release/evidence/0.0.105/`、`release/evidence/0.0.101/`、`release/notes-0.0.105.md`、`release/notes-0.0.101.md`、`release/evidence/0.0.73/chain-ape-receipt.txt`、`tools/no-home-path.cx`、`tools/scrub-home-paths.cx`、`tools/release-flow.cx:192-214`、`tools/release-exec.cx:1172-1184`

## 1. 结论

(1) 已发布证据可接受的路径写法事实: 允许使用 `~/` (`~/.cache/...`、`~/.unisacc-release-evidence-backups/...`、`~/repos/...`、`~/work/...`)、`$HOME/` 或仓库/链内相对路径; 唯一被提交门与发布门禁入的是未清洗的绝对家目录路径 (如带实际系统用户名的绝对家目录前缀)。仓内现有已发布证据与工具链中没有任何 `external/` 改写先例。
(2) 对 `agy-flow` `ape-receipt` 草稿 (11 行 `~/.cache/...`) 的裁定: 不必改成 `external/` 或强改链内相对路径, 保留 `~/.cache/...` 简写完全可接受且与 `0.0.101` / `0.0.105` 已发布证据及门禁工具完全一致; 只需确认不含未脱敏的真实用户名绝对路径即可直接随证据提交。

## 2. `0.0.105` 与 `0.0.101` 实测路径命中原行

### 2.1 `INDEX.tsv`、`notes`、`rollback.txt`、`strict.tsv`、`expected-fail.tsv`

- `release/evidence/0.0.105/` 与 `release/evidence/0.0.101/` 下的 `INDEX.tsv`、`rollback.txt`、`expected-fail.tsv`、`strict.tsv`、`release/notes-0.0.105.md`、`release/notes-0.0.101.md` 中, 对家目录绝对前缀、`~/`、`.cache`、`external/` 均命中 `0` 行 (均用包内文件名或版号相对路径)。
- 全仓已发布证据目录 (`0.0.98`、`0.0.100`、`0.0.101`、`0.0.105`) 及 `tools/` 中, 对 `external/` 改写命中 `0` 行。

### 2.2 `0.0.105` 与 `0.0.101` 回执/日志中 `~/` 与 `~/.cache` 实录原行

(a) `pack-run.out` (`0.0.105` 共 30 行命中, `0.0.101` 共 28 行命中, 均直接保留 `~/.cache/...` 与 `~/.unisacc-release-evidence-backups/...` 与 `~/repos/...`):

```text
release/evidence/0.0.105/pack-run.out:1:RELEASE_RUN	COMMIT_BACKFILL	before=0bca20928bb6b39e7248660136dbc9ee026c5a51	after=414a024ea5e52462b22e7ed9d11f6c44b1d6c2e8	~/.cache/unisacc/release-v0105-state/pack/candidate-in.tsv
release/evidence/0.0.105/pack-run.out:4:RELEASE_RUN	CACHE_MANAGER	x64	9dea69bf8a79f8587b45c7b6f9b8d9dc72097a804662750f231887898e4ed4e4	srcdir	~/.unisacc-release-evidence-backups/0.0.105/src-dir/cache-manager-x64
release/evidence/0.0.105/pack-run.out:7:RELEASE_RUN	FIXEDPOINT	VERIFIED	~/.cache/unisacc/chains/v0.0.105/0bca20928bb6b39e7248660136dbc9ee026c5a51
release/evidence/0.0.105/pack-run.out:24:unisacc-next.com	~/.cache/unisacc/release-v0105-state/pack/ape/unisacc-next.com	sha=934c7c0bc09a9a4d2e107b8d6bdc855553cf05f64dbb7d6c1e5fefdb42117dd1	bytes=8842611
release/evidence/0.0.105/pack-run.out:26:RELEASE_RUN	CM_EVIDENCE	~/repos/unisacc/.claude/worktrees/v0.0.105/release/evidence/0.0.105/cache-manager.tsv
release/evidence/0.0.101/pack-run.out:7:RELEASE_RUN	FIXEDPOINT	VERIFIED	~/.cache/unisacc/chains/bg-b1/99aa998d8764e46b7b9ce140d435db003dbb131b
release/evidence/0.0.101/pack-run.out:24:unisacc-next.com	~/.cache/unisacc/release-v0101-state/pack/ape/unisacc-next.com	sha=bb715a8bd3ed1237bb69d42d0cd1c2594c0a27c8e223b518ae39efe6b79eac82	bytes=8827155
release/evidence/0.0.101/pack-run.out:26:RELEASE_RUN	CM_EVIDENCE	~/repos/unisacc/release/evidence/0.0.101/cache-manager.tsv
```

(b) `backup-evidence.tsv` (`0.0.105` 与 `0.0.101` 各 7 行) 与 `cache-manager.tsv` (各 2 行):

```text
release/evidence/0.0.105/backup-evidence.tsv:2:chain/heartbeat.tsv	~/.unisacc-release-evidence-backups/0.0.105/chain/heartbeat.tsv	9c3e389bead8ceac33f1e3a7bcac0e5667723109f83cde1cc7671d327ec0614d	17134
release/evidence/0.0.105/backup-evidence.tsv:6:chain/ape-final/unisacc-next.com	~/.unisacc-release-evidence-backups/0.0.105/chain/ape-final/unisacc-next.com	934c7c0bc09a9a4d2e107b8d6bdc855553cf05f64dbb7d6c1e5fefdb42117dd1	8842611
release/evidence/0.0.105/cache-manager.tsv:3:x64	9dea69bf8a79f8587b45c7b6f9b8d9dc72097a804662750f231887898e4ed4e4	srcdir	~/.unisacc-release-evidence-backups/0.0.105/src-dir/cache-manager-x64
release/evidence/0.0.101/cache-manager.tsv:3:x64	d1ab1d507c728787c0af20a6a3e9f06a8425e3e1888447ed896096402851f928	srcdir	~/.unisacc-release-evidence-backups/0.0.101/src-dir/cache-manager-x64
```

(c) `warm-<key>.tsv`、`public-readback.txt`、`prerun-supervise.log`、`ci-<id>.log`:

```text
release/evidence/0.0.105/warm-568b3eb43613c8317a434eaaa951292864717eb099989c0874fce906544071c9.tsv:1:WARM	key=568b3eb43613c8317a434eaaa951292864717eb099989c0874fce906544071c9	root=~/repos/unisacc/.claude/worktrees/v0.0.105	...
release/evidence/0.0.105/public-readback.txt:51:C47_1_READBACK PASS expected_draft=false tag=v0.0.105 version=0.0.105 commit=414a024ea5e52462b22e7ed9d11f6c44b1d6c2e8 output=~/.cache/unisacc/release-v0105-state/exec/readbackte79f2ej/result
release/evidence/0.0.105/prerun-supervise.log:1:PRERUN_FROZEN_SOURCE	rev=0bca20928bb6b39e7248660136dbc9ee026c5a51	root=~/.cache/unisacc/prerun/0.0.105/frozen-root
release/evidence/0.0.101/public-readback.txt:51:C47_1_READBACK PASS expected_draft=false tag=v0.0.101 version=0.0.101 commit=8f0a0af2c4157df7944c044056dc32e187b1a427 output=~/.cache/unisacc/release-v0101-state/exec-out/readback2lphrv88/result
release/evidence/0.0.101/prerun-supervise.log:1:PRERUN_FROZEN_SOURCE	rev=99aa998d8764e46b7b9ce140d435db003dbb131b	root=~/.cache/unisacc/prerun/0.0.101/frozen-root
release/evidence/0.0.105/ci-45347740771.log:199:... Working directory is '~/work/unisacc/unisacc' (共 162 行 ~/work/...)
```

(d) 历史同类 `ape-receipt` 文件对照 (`release/evidence/0.0.73/chain-ape-receipt.txt`):
- 当年 `0.0.73` 整链位于 `/tmp/unisacc-session-v073-chain/...`, 故保留原始 `/tmp/...` 路径而未使用 `external/` 改写; 到 `0.0.106` 整链目录改至 `~/.cache/unisacc/chains/bg-b1/2e8a19a342...`, 原生 `ape-receipt.txt` 带用户绝对家目录前缀 11 行, 经清洗为 `~/.cache/...` 后即等效且合门。

## 3. 工具门与发布流脚本规则对照

- `tools/no-home-path.cx:8-11`: 只拦绝对家目录前缀 (`/home/<NAME>/`、`/Users/<NAME>/`、`C:\Users\<NAME>\`), 明确说明可写 `"~/"`、`$HOME/` 或仓库相对路径。
- `tools/scrub-home-paths.cx:4`: 官方唯一重写规则就是把上述绝对家目录前缀就地改写为 `~/` (Windows 形式改 `~\`)。
- `tools/release-flow.cx:192-214` 与 `tools/release-exec.cx:1172-1184`: 写入 `release/evidence/<V>/` 与日志时自动经过同一脱敏重写 -> `~/`, 不会也不需要改写为 `external/`。
