# 代裁：Darwin 日常以 m4pro 本机绿推进 Draft（机房主任 1532）

墙钟：2026-10-11 15:32 CST。授源：政委「好」=同意。

## 裁定
1. **日常 Darwin**：以 **m4pro 本机绿**推进 Draft 材料（B.2→B.3→seal 前置材料）；不以「等 GitHub mac CI 全绿」挡日常 Draft 推进。
2. **GitHub mac CI**：留给**大版本校验**（major / 发布闸旁证）；不作为日常 Darwin 增量推进的阻塞门。
3. **主 CI `38119555397`**：仍**旁观**（@ `ed50f00c`；含 bootstrap `-t HOST_TARGET`；不含后续 research tip）。不把未结/旁观 CI 写成产品 PASS；不因旁观 CI 红/未结回滚本机绿结论。
4. **公开边界**：开 GitHub Draft 前须再向机房主任确认一次公开边界；**Latest 仍须另授**。本刀禁擅自 Latest／公开发布。

## 与 1228 CI 口径的关系
- `research/c40-draft-ci-accept-1228.md`／AGENTS「Draft/发布验收以 GitHub CI 为准」仍保留为**发布／大版本校验**口径。
- 本裁定是**日常 Darwin 推进 flex**：m4pro 本机 bootstrap／门绿可驱动 Draft **材料**推进；正式 `gh release create --draft`／Latest 切换仍须另授，且大版本校验仍看 GitHub mac CI。

## 本刀已落
- m4pro tip `7acb254a`（祖先含 `ed50f00c`／`c2b140bc`）完整 `tests/bootstrap.sh` → **ok B=C=U**（见 `m4pro-bootstrap-flex-1532.md`）。
- paste 三窗：`paste-*-1532.txt`。
- AGENTS／pipeline 短注（本裁定指针）。

## 硬禁（不破）
禁擅自 Latest／公开发布／owner-promotion；禁 bump（本刀未授）；禁把旁观 CI 未结写成产品 FAIL；禁以 U 代 B；禁放宽验收数值。

## 身份
| 项 | 值 |
|---|---|
| tip（本机取证） | `7acb254a217f614b73c811d4b9575160cfa86dd0` |
| 含修 | `ed50f00c`（bootstrap `-t HOST_TARGET`）、`c2b140bc`（acceptance 10-* 同形） |
| 本机终表 | ok B=C=U `257c4ef6944103ef`／421304 lines／rc0（15:33:59–15:34:24 CST） |
| 旁观 CI | `38119555397` @ `ed50f00c` |

## 下一刀（未自动开）
1. 本机绿后推进 B.3／seal／Draft **材料**（对齐 1140 链；净 worktree）。
2. **开 Draft 前**再向机房主任确认公开边界。
3. Latest 另授。
