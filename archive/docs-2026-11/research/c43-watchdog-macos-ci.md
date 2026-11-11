# c43 看门狗 macOS CI 观测（只读，2026-10-15）

**结论先行**：推送 `864b2051`、`40842ef4`（origin 头 `47c12b8e`）后，**没有任何 CI 工作流运行 watchdog 三作业**，因此 macOS runner 上**没有可观测结果**。本笔不记绿，也不记红。

## 1. 为什么没有观测

- 三作业（`watchdog-orphan`、`watchdog-eof`、`watchdog-success`）是 `tests/gate.sh` 的 job 行，由 `tests/watchdogregression.sh` 执行。
- `.github/workflows/ci.yml`：Linux 与 macOS 矩阵运行 `tests/all.sh --suite …`，套件表不含这三项（`tests/all.sh` 全文只有第 114 行一处注释提到 watchdog）。
- `.github/workflows/release-check.yml`：未调用 `tests/gate.sh`。
- `.github/workflows/darwin-gate.yml`：唯一在 macOS 上跑 `tests/gate.sh --com --suite …` 的工作流，但它是手动派发（manual），且只列出固定套件（`exec-bootstrap-osxarm lib-stack-arm forward com-forward` 等），不含 watchdog。
- 因此在 CI 上，watchdog 三作业只存在于本机 `gate.sh` 运行中，不会在推送后自动在 macOS 执行。

## 2. 当前观测（只读记录，不是 watchdog 结果）

- `release-check` 对 `47c12b8e` 运行中（`gh run view 38659374750`）：`ccinterop-x86` 成功，`winsuite-prepare`、`source` 进行中。该运行不含 watchdog 作业。
- 上一次成功的 `release-check`（`887b782f`，`394b57f6`）同样不含 watchdog 作业。

## 3. 需要 cc 定的一项（改 CI 或套件表，本笔不做）

要得到 macOS 上的观测，需要把三作业放到某条 macOS 路径里，二选一：
1. 在 `darwin-gate.yml` 的 `suites` 矩阵里加入 `watchdog-orphan watchdog-eof watchdog-success`（手动派发，arm64 与 x86_64 两个 cell）。
2. 在 `tests/all.sh` 的套件表中加入一个调用 `tests/watchdogregression.sh` 的套件，使 `ci.yml` 的 macOS 矩阵自动运行。

两种都改工作流或套件表，属于 CI 与门禁结构的变更，需要 cc 批准后再做。

## 4. macOS 结论的边界

- 即使 CI 日后观测到绿，**也只记为“CI 观测”**，不记为 `bound.py` 的 macOS 分支已核（`ps -axwwE` 分支仍须在 m4pro 上核对）。
- 若日后红，应采集该 runner 的日志，定位是 `ps -axwwE -o pid=,command=` 的输出格式（环境变量显示方式）还是进程枚举；届时写入本笔的后续节。
