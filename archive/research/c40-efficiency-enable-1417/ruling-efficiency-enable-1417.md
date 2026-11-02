# 代裁落地：效率 L3/L2 启用 + L1→tip（机房主任 1417）

墙钟：2026-10-11 14:19 SGT。授源：政委「要实际改进、0.0.40 等一天」；cdx2 已落 `efficiency-macro-1411.md` + L1/L2/L3 模板。对照 1413 代裁与宏观本周清单。

## 裁定（立刻）

### 1) 启用 L3（签字为现行默认）
- 模板：`/tmp/unisacc-cdx2/templates/auth-pack-first-red-same-sha.md`
- 内容：文件域 + 预算 + 首红停；同 SHA 只读/失败重跑探针范围内免再问（越界仍回裁）。
- 回执：`enable-receipt-L3-auth-pack-1417.md`
- 签字：机房主任代裁 / WJ C · 2026-10-11 14:19 SGT

### 2) 启用 L2（ruling 并批为默认）
- 模板：`/tmp/unisacc-cdx2/templates/ruling-batch-same-claim.md`
- 回执：`enable-receipt-L2-ruling-batch-1417.md`

### 3) L1→仓内 tip（cc 主刀；cdx 真审 PASS）
- 窄改：`.github/workflows/ci.yml` + `tests/bootstrap.sh`
  - EXIT 前 env 门控 copy；macos suites 常设 `UNISACC_DIAG_OUT`
  - **失败 job** `upload-artifact`（retention 5d）
- cdx 审：`cdx-ci-diag-L1-review-1417.md` → **PASS 可进 tip**
- 禁：放宽验收、bump、Draft、公开、seal、二次 rerun
- 说明：attempt2 钉旧 SHA，**不**回溯带证；若仍红，**下一跑**（新 tip dispatch/schedule）可带诊断包

### 4) paste 三窗 + 盯 attempt2
- `paste-cc-1417.txt` / `paste-cdx-1417.txt` / `paste-cdx2-1417.txt`
- attempt2：run `38111789528` attempt=2；三 mac 采样时仍 step8 in_progress；**不二次 rerun**

## 硬禁（不破）
禁 bump / Draft / seal / Latest / 公开 / 放宽验收 / m4pro / 以 U 代 B / 一次绿当根因闭合 / 二次 rerun。

## 身份（采样）
- 仓：`~/repos/unisacc-cc`
- CI 钉源码 SHA（attempt2）：`82e1a9f06e9e39f805e25f0ad285b7306acb9937`
- tip（已进 origin/main）：`1cb3fad0bba62a9e9343e4782626978e5a52aabb`（短 `1cb3fad0`）

## 已落路径
- 本页；L2/L3 模板状态更新；两份启用回执；cdx 真审；paste×3
- 仓内：ci.yml + bootstrap.sh 窄改（进 tip）
