# prd_release.md —— 发布与 CI 手册（自 0.0.48 发布经验归纳）

本文件是 [prd.md](prd.md) 的子文档：只写**怎么发布、CI 怎么跑、踩过什么坑**。当前版本状态见 prd.md §9.1；单次任务进度写 `agentwork/ledger.tsv`。

## 1. 发布链（顺序即依赖）

| 步 | 动作 | 工具/命令 | 通过判据 |
|---|---|---|---|
| 1 | 冻结源 SID | `seed/ident ROOT`；`freeze.tsv` + `tools/precommit.cx` 冻结闸 | main 源 SID == 候选 SID |
| 2 | 候选构造 | CTX→131 作业→42 网全键验证→kernel→P4 包→六个 whole-o 切片 | 全部 rc=0；全键观察数一致 |
| 3 | APE 打包与烟测 | `release-ape`；`install-ape ROOT APE SHA [--write]` | `--version` 正确；退出码 7 烟测 |
| 4 | 提交档 s9 | `tools/all-waves.cx`（外层 `tools/bound 55`） | 提交档全绿；daily/release 未跑须如实披露 |
| 5 | 候选表 | `release/candidate.tsv`（21 列，18–21 派生）；`tools/release-layout.cx --candidate TABLE SOURCE NEW_OUT`，再 `--check OUT` | check 通过 |
| 6 | 私有草稿 release | `gh release create --draft`，上传全部资产 | 资产齐全（见 §2） |
| 7 | 同候选六目标原生 CI | `gh workflow run candidate-native.yml -f ape_sha256=… -f commit=<40位> -f release_tag=…` | QUALIFIED N/0 |
| 8 | 公开 | `gh release edit TAG --draft=false --prerelease=false --latest` | 读回 `SHA256SUMS` 与 `--version` |

## 2. 草稿 release 必备资产

- APE `unisacc.com`、`SHA256SUMS`、记录/构造 zip；
- **`slice-win-arm64`**（win/arm64 切片作为独立资产）；
- **`pkg.bin` 必须是带 16 字节尾的 `compiler.p4.pkg`**（`UNIPKG1\n` + LE64 尾），不能是裸 `p3-package.body`。

## 3. 0.0.48 的 CI 三轮教训

| 轮 | 现象 | 根因 | 处置 |
|---|---|---|---|
| 1 | 失败 | 草稿 release 缺 `slice-win-arm64` 与 `pkg.bin` | 补资产 |
| 2 | `bad or missing package footer` | `pkg.bin` 用了无尾裸正文 | 换 `compiler.p4.pkg` |
| 3 | QUALIFIED 42/0 | Mac Intel `stream-pipe` 第 2 轮超时为偶发，第 3 轮通过 | 运行 42545543777 |

规则：触发 CI 前先对照 §2 清单机查资产；偶发超时只在重跑通过时记偶发，不改时限。

## 4. 版本串与签名

- 版本串在产品输入内（`exec/facts/limits.tsv` 共 8 处、`src/version.h`、`src/drive.cx`），换版本即换源 SID，需重建整链（0.0.49 P0-2 要解耦）。
- 版本补丁共 9 文件，含 `.github/workflows/candidate-native.yml` 的转义正则 `^unisacc 0\.0\.NN\r?\n$`；grep 版本要包含转义形态与 `check-productpkg`。
- Mac 切片是 ad-hoc 签名：版本串变化必牵动覆盖该页的 32 字节页哈希槽。判据：差异集 == 版本字节 ∪ 该槽，且槽 == SHA256(该页)。Linux/Win 仅版本字节。

## 5. 时限与耗时（0.0.48 实测）

| 项 | 实测 | 上限 |
|---|---|---|
| Linux whole-o | 36–41 s | 60 s |
| Win whole-o | 70–83 s | 120 s（`/usr/bin/timeout --signal=KILL 120`；`tools/bound` 只接受 1..60） |
| Mac A+B | 97–100 s（A 51–57 s） | 贴 60 s 线 |
| 候选构造总计 | 约 2 小时 | 0.0.49 目标压缩 |

## 6. 集成操作纪律

- 共享工作区只有集成者写 main；其他窗口出补丁，集成者在私有克隆 `cherry-pick/rebase` 后推送。
- 同步本地用 `git merge --ff-only origin/main`，**直接检查返回码**，不接 `| tail`（0.0.48 曾因此漏报合并失败）。
- s9 会话期间，被跟踪工作树文件（含 `agentwork/ledger.tsv`）变化会使整波作废：用静默窗口（0.0.49 P0-4 修闸）。
- 公开发布后写 `agentwork/ledger.tsv`，并读回 `SHA256SUMS` 与 `--version`。

## 7. 往期发布

| 版本 | 状态 | 入口 |
|---|---|---|
| 0.0.43 | 历史 | [plans/release-0.0.43.md](plans/release-0.0.43.md) |
| 0.0.45 | 公开预发布 | [plans/release-notes-0.0.45.md](plans/release-notes-0.0.45.md) |
| 0.0.47 | 已发布 | [plans/release-notes-0.0.47.md](plans/release-notes-0.0.47.md) |
| 0.0.48 | 正式版，CI 42/0（运行 42545543777），APE sha256 `559705097427cf8f…` | [plans/release-notes-0.0.48.md](plans/release-notes-0.0.48.md) |
