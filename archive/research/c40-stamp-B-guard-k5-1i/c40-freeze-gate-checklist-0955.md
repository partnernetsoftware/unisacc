# 0.0.40 冻结门闩清单（cc，0955；research-only；未执行任何冻结动作）

**本页只列门闩与执行时的复核步骤。** 不 bump、不 Draft、不 freeze 实跑、不改 `src/version.h` / prd / gatedeps、不改验收。授权人确认前，三项仍为「未开」。

## 0. 快照（事后采样，写本页时）

- HEAD = origin/main = `fc13d9e71a3389dd31446a7667b74669a8cd4561`（含 #106 paper-a 笔记 FF；不挡 0.0.40）。
- `git status --porcelain` 为空；`git ls-remote origin refs/heads/main` 与 HEAD 一致。
- `src/version.h` 仍为 `0.0.39`。
- Latest v0.0.39；无 Draft v0.0.40。

这些是**写页时的快照**，冻结执行时必须重新采，不能沿用本页。

## 1. 四前置（出处：`c40-freeze-first-observe.md` 的「实跑前置」，与 `cdx2-postclose-0942.md` 对照）

| # | 前置 | 当前判定 | 门闩 |
|---|---|---|---|
| 1 | 0.0.40 开发/预验完成，授权人批准冻结（含未完门闩具名处置） | **仍开** | 授权人具名批准；run/.cx intake 等未完切口须具名「不依赖 / 顺延 / 阻断」。首红命令层闭合不等于开发全面完成。 |
| 2 | 单独版本提交 `src/version.h` → `0.0.40`（freezecheck 起点） | **仍开** | 仅此一个版本提交；其后产品闭包 `exec/ unisa/ src/ kernel/ include/ weights/` 的改动须以 `fix:` 开头或在 `release/freeze-waivers.tsv` 有 waiver（`tests/freezecheck.py` 判定）。 |
| 3 | 净树；HEAD / origin/main / ls-remote 三者一致 | **快照已满足**，执行时重核 | 见 §2 步骤 1–4。 |
| 4 | 冻结提交后刷新 gatedeps（refresh ≠ review） | **仍开** | 版本提交之后运行 `tests/refresh_gatedeps.py`；只刷新，不等于复核；stamp-A/B 既有认键不替代冻后身份核验。 |

**结论**：四项中第 3 项是快照满足、执行时重核；第 1、2、4 项仍开。Draft 未放行。

## 2. 执行窗内的三者一致重核（只在授权冻结之后做）

1. `git fetch origin main`；`git status --porcelain` 必须为空（不得带脏树进入冻结）。
2. `git rev-parse HEAD` 与 `git rev-parse origin/main` 相等。
3. `git ls-remote origin refs/heads/main` 的第一列与 HEAD 相等（rc 0）。
4. 版本提交只改 `src/version.h`（`git show --stat` 仅此一文件）；`grep UNISACC_VERSION src/version.h` 为 `0.0.40`。
5. 版本提交后：`python3 tests/freezecheck.py` 无违例（或只剩已登记 waiver）。
6. 运行 `tests/refresh_gatedeps.py`，`git diff -- tests/gatedeps.json` 只允许 refresh 产生的改动；随后再次做 1–3。
7. 任一步失败：停，记录 rc 与现场，不做「顺手修」，不改验收。

每一步的输出都要存仓外证据目录，并记 sha256；事后采样须在文档里标明。

## 3. 本页不授权的内容

- 不授 bump、不授 Draft、不授 freeze 实跑、不授首观察（只到第一个 make 的 rc 97 截停也不是 PASS）。
- 不授改错声明 sha 负例的阻断化（原口径：可顺延）。
- 不授 run/.cx 缺陷进入本冻结（另窗具名裁）。
- 不宣称 PGID/TERM/UA 本体/门绿已闭合。

## 4. 谁裁

- 前置 1 与冻结批准：授权人（政委 / 机房主任）具名。
- 前置 2 的 bump 窗口与前置 4 的 refresh：机房主任另裁执行窗口。
