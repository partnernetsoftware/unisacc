# ci.yml 最小诊断补丁页（1413；仅诊断，不放宽验收）

## 目标
失败 macos job 可下载 artifact，内含 bootstrap 关键文件（依赖 suites 步导出 `UNISACC_DIAG_OUT` + bootstrap EXIT 前 copy）。

## 改动点（窄）

### 1) `jobs.macos` suites 步（`run: |` / `set -e` 那段）
在脚本开头加：
```bash
export UNISACC_DIAG_OUT="${RUNNER_TEMP:-/tmp}/unisacc-diag"
mkdir -p "$UNISACC_DIAG_OUT"
```
不改变 `bad`/exit 逻辑。

### 2) `tests/bootstrap.sh`（env 门控，默认无行为）
在 `T=$(scratch)` 之后 source 或内联：若 `[ -n "${UNISACC_DIAG_OUT:-}" ]`，定义 `diag_copy`；在 `wait` 后与任何 `exit` 前调用 `diag_copy "$T"`（含成功路径可选，失败路径强制）。  
参考仓外：`bootstrap_diag_copy.sh`。  
**NEG**：未设 `UNISACC_DIAG_OUT` 时与现网字节级行为一致（无新文件、无新出口）。

### 3) suites 步后追加两 step
见 `ci-macos-diag.fragment.yml`：`preserve`（always）+ `upload-artifact`（failure）。

## 明确不做
- 不 `continue-on-error` 放宽 macos-14/15 验收。
- 不删/改 bootstrap 判定（B=C=U、FAIL 行）。
- 不 bump / Draft / 公开。
- 不依赖 m4pro。

## 落地授权
`auth-same-sha-flaky-diag-default-1413.md` §B；机房主任 1413 代裁可进 PR/主仓。

## NEG（同刀）
1. 本地：`UNISACC_DIAG_OUT=` 跑 `tests/bootstrap.sh`（或 all.sh --suite bootstrap）→ 与门控前同 rc，且无 `$UNISACC_DIAG_OUT` 写入。
2. 本地：`UNISACC_DIAG_OUT=/tmp/diag-neg-$$` 跑一次 → 目录出现 B.log/U.log 或 identity（若失败路径触发）；suite rc 仍由原逻辑决定。
