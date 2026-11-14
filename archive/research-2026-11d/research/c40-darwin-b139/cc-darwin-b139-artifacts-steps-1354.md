# CI artifacts / steps 只读扫（1354）

run `38111789528` @ `82e1a9f0`；采样 `2026-10-11 13:56 SGT`。

## Artifacts
- `GET .../actions/runs/38111789528/artifacts` → **total_count=0**，names=[]。
- `.github/workflows/ci.yml` **无** `actions/upload-artifact`（只读确认）。
- bootstrap scratch 由 EXIT trap 删除；`bs_B` / `bs_U` / tape / core **未上传、未保全**。
- 结论：原件 **UNKNOWN**；纯原样重跑仍可能只留下 `B=139` 而无 core/PC。

## Job steps 时间线（失败步）

| job | 失败 step | started_at (UTC) | completed_at (UTC) | 备注 |
|---|---|---|---|---|
| macos-14 | `Run set -e` (#8) | 04:31:02Z | 05:08:11Z | suites 内 bootstrap 红 |
| macos-15 | `Run set -e` (#8) | 04:31:12Z | 05:13:46Z | 同形 |
| macos-15-intel | `Run set -e` (#8) | 04:30:55Z | 05:48:44Z | 更久；intel build-weights step skipped |

前置 checkout / setup-python / corpus / fixtures / Mach-O host probe：**success**（三格）。

## 日志内 bs_B / 分步 rc / 时间线要点

### 共同（三格）
- 日志只见 **branch 总 rc**：`B=139 U=0`；**无**步①②③④各自 rc；**无** core / DiagnosticReports / 异常 PC / 栈。
- `bs_B` 与 `bs_U` 均已产出（非空路径打印）；故步① empty-tape 路径可排除；步②写盘后崩溃仍 **不能排除**（无分步 rc）。
- `sign_image` 不检查 codesign rc（`tests/bootstrap.sh`）；签名失败未留痕。

### macos-14 / macos-15（arm64）
- acceptance10：Btape/Bimage **rc=0**；**Ctape rc=1**；Uimage rc=0；**compare rc=1**（先于 bootstrap）。
- bootstrap：`finished bootstrap rc=1`；`B=139 U=0`；bs_B/bs_U 均 `osx/arm64` `2328418 B`（text 1839624/1832492；insn 396703/402437；magic `cffaedfe`）。
- wall ~30–31s。

### macos-15-intel（x86_64）
- acceptance10/11：**SKIP** `requires Darwin/arm64 (not a pass)` — **无** Ctape/compare 红账。
- bootstrap：`B=139 U=0`；bs_B `osx/x86_64` 1816546 B（text 1338448，317395 insn）；bs_U 1800034 B（text 1295674，319326 insn）；magic `cffaedfe`。
- wall 52s；`##[error] Process completed with exit code 1`。

## 取证缺口（仍缺 → 不足具名修域）
1. 步② from-tape / 步③ sign / 步④ `bs_B $SELF -c` 各自 rc 与 stderr。
2. 真实 signal、异常 PC、栈、Mach-O 映射。
3. bs_B / bs_U 字节与签前签后 SHA（CI 未上传）。
4. codesign 独立 rc。

**裁定口径**：证据 **不够** 写具名最小修域请示；改走 **诊断副本 + 同 SHA flaky 探针**（本刀代裁授权探针一次）。
