# 代裁 ≈11:59 SGT — 首观察范围闭合 + UNKNOWN 具名接受 + research 入仓（机房主任）

## 现场
- tip **bbd84a22**（本巡 FF paper-a #115 → 合入 1153 ACK `bbd84a22`；上轮 tip `10c2e9b7`）
- Latest **v0.0.39**；无 Draft v0.0.40（历史 Draft v0.0.31 不计）
- 首观察 1147：preflight→首 make，rc97 垫片截停（预期）
- 补证 1152：cdx PASS 范围；UNKNOWN 具名；未见重跑/Draft/queue 越权
- 现场当时私有 Darwin 宿主 offline（历史旁注，**非阻塞**）；Linux B.2 / 真 make 仍不授

## 代裁（宿主/卫生/顺延）
1. **首观察范围闭合**：认 1147+1152 为「只到首 make」观察完成；**非**产品 PASS / 非全量 queue。
2. **UNKNOWN 具名接受**（不升格绿、不倒填）：t_end；1147 ps 原表；identity-after ls-remote；失效键/seed/exittable；真 make；若干声明式 rc0 原证。
3. **授 research-only 入仓**：把 1147/1152 回执+cdx/cdx2 短记+本裁定迁入 `research/c40-first-observe/`（可附状态目录文件哈希清单）；**禁**改 version/gatedeps/.cx/prd/验收措辞。
4. **不授**：重跑首观察、续 Linux B.2、sudo 装 make、Draft、公开发布、seal、bump、全量 queue。
5. Draft/发布验收改钉 **GitHub CI / macos runner**（机房主任 2026-10-11）；不以私有 Darwin 宿主上线为硬依赖。本机 Linux B.2 仍不授续跑。

> 历史旁注：1159 当时曾写「Draft 挂私有 Darwin 另刀」；该口径作废，现非依赖。

## 持刀
- cc：入仓 research + 短回执
- cdx：只读核入仓范围/哈希/越权
- cdx2：盯 tip/漏证/禁借入仓续 B.2
