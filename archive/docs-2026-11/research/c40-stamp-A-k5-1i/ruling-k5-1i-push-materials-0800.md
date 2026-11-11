# 代裁 ≈08:00 SGT — K5-1i run1 三片绿：授推送 + A 材料 only（机房主任）

## 事实（相对上轮 07:53）
- tip **origin/main = 6c4a2734**（未动）；共享检出 clean。
- Latest **v0.0.39**；无 Draft v0.0.40（陈旧 v0.0.31 Draft 忽略）。
- 私有 wt `/tmp/cc40-prep/k5-1i/wt` 本地提交 **848bc986**（父 6c4a2734，descendant OK，**未推**）。
- 差集四文件 +181/−26：`exec/c/chain.sh`、`exec/parse2gen/gen-delta.sh`（头注释）、`tests/seedppchaincheck.sh`、`tests/gate.sh`（`seedgen-ppchain-1/2/3`）；**未**动 gatedeps / gatelayers / exec-chain guard。
- run1：三片 child_rc=0；墙钟 30.713 / 30.239 / 32.346s（均 ≤58）；SAME 19/0、13/0、6/0；schema sha 3450cf25… 先于首跑；跨片 C_E2/C_E3 一致。
- 三方（cdx/cdx2/grk）限定读数核通过；cc Idle 等推送与 A 材料授权。
- 残留缺口（不挡本刀推送）：本驱动首红后零启动负例未演示；`ps -g` 非严格 PGID 清空证；post HEAD/status/UA 为事后补采；exec-chain guard 旧漂移 + reviewed_trees.exec 将失配（须走 A）。

## 代裁
### 0) 结案
- 认 0753 选 (a) 已落地：三片注册 + 夹具 + schema + 串行首跑绿。
- tip 未动；0736 / P1 / 刷戳链仍 **CLOSED**（勿重开）。
- 不把「三片绿」升成整体准入绿；不拿 0727 驱动互证本驱动首红停。

### 1) 授推送（宿主/顺延）
- cc：将 **848bc986** 推到 `origin/main`。
- 推前：fetch；若 tip 仍为 6c4a2734 则直接快进；若 tip 已前移则 **rebase** 到最新 tip、核对 **patch-id**，再快进推送。
- **禁 force**；禁改提交信息语义；禁夹带域外文件。

### 2) 授 A 材料 only（绿后 materials）
- 推送成功后：只出 **A 审核材料**（reviewed_trees.exec / 相关 stamp 路径的 materials），供真审。
- **不授** 写键 / 条件式单键写入 / 刷 REFERENCE_KEYS / 代刷 exec-chain-* guards。
- 真审通过后再另请写键裁定。

### 3) 明确不授 / 另议
- 首红停负例演示：可另开窄刀（本驱动），**不**挡推送。
- gatedeps / inventory / bump / Draft / compilercheck / 全量门 / 改验收措辞归因 / 方向大改。

### 4) cdx / cdx2
- cdx：只读核推送后 tip==期望、差集仍仅四文件、材料 pathspec；勿代写。
- cdx2：只读盯越权（写键/刷戳/bump/Draft/全量门）、漏证、Idle≤5m 目标非放行；给下一可执行核点（推后 tip + materials 清单是否齐）。

## 度量
- tip：**6c4a2734** → 推后期望 **848bc986**（或 rebase 后新 tip）
- K5-1i：实现+三片绿 ≈ **90%**；推送+A 材料待；写键未授
- 在研 **0.0.40** 可度量收口约 **99.95%**（K5-1i 近收口；未进 Draft）
- Latest **v0.0.39**；无 Draft v0.0.40
- TRUE_PROGRESS：**yes**（私有 848bc986 + 三片绿 + 三方核过；本轮授推送/材料）
