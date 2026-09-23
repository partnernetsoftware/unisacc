# UXE · 游戏平台方向

> 短文：平台怎么从 Host + 多款 ship 长出来。  
> **契约真源**仍是 [`HOST_ABI.md`](HOST_ABI.md)；本文不替代 ABI。

## 一句话

**同一套 Host API + 可换 `{game}` 核 + 用户可选自带 LLM API** → 可外网玩的游戏平台。  
先把交付面与玩法密度做实；变现模型后置。

## 三块积木

| 积木 | 现在 | 平台要什么 |
|---|---|---|
| **Host API** | 时间 / 输入 / 帧 / UXEP·GPU / 资源 / 日志 | 继续加厚（H3+）；规划 `host_llm` |
| **引擎** | `gameEngine.wasm`（UJS VM） | 稳定交付名；与游戏核永远分开 |
| **游戏** | Asteroid（Pages）· 大富翁（归档） | 下一款街机实践车；模板可复制 |

外网入口（GitHub Pages · `docs/`）：

- 索引：`/docs/index.html`
- 游戏：`/docs/uxe/{name}/`

本地重建：`cd ujs && npm run ship:engine`。

## 用户自带 LLM（BYO API）

意图：玩家或作者在壳里粘贴 **自己的** API endpoint + key，用来驱动 NPC、关卡生成、裁判、叙事等——**平台不代持密钥、不做中心化计费前提**。

| 层 | 职责 |
|---|---|
| 页面壳 | UI：填 key / 选模型；永不把 key 写入 URL 或 UXEP |
| Host | `host_llm_*`（规划）：发请求、超时、剥 key、只回结果 bytes |
| 游戏核 | 只提交提示结构化请求、消费结构化回复；可降级为「无 LLM 也能玩」 |

详见 Host API §7.1。未实现前游戏保持纯本地可玩。

## 作者路径（目标体验）

```
写 `*.ujs` + 核只调 `host_*`
  → npm run ship:engine
  → docs/uxe/{game}/ 可玩（过可玩性门槛再进索引）
  → （可选）壳打开 LLM
```

今日缺口（按优先级）：

1. **街机无人机战场**（`demo/drone/`，指针已用上）→ 可玩后再 ship / Pages  
2. Host 继续按卡住点加厚（audio / 贴图…）  
3. `host_llm_*`：有玩法需要再落地  

## 明确不做

- 把 Three 搬进 wasm  
- 游戏与引擎合包  
- 开放完整浏览器 JS 当玩法语言  
- 把用户 key 写进仓库、packet、或公共日志  

## 相关文档

| 文档 | 用途 |
|---|---|
| [`HOST_ABI.md`](HOST_ABI.md) | Host API 真源 |
| [`README.md`](README.md) | UXE 目录与 demo/ship |
| [`../../DOCS.md`](../../DOCS.md) | ujs 文档地图 |
| [`../../FUTURE.md`](../../FUTURE.md) | P0 / H 档 / 中期节点 |
