# UXE · 游戏平台方向

> 短文：平台怎么从 Host + 多款 ship 长出来。  
> **契约真源**仍是 [`HOST_ABI.md`](HOST_ABI.md)；产品化下一刀见 [`FUTURE.md`](../../FUTURE.md)。

## 一句话

**同一套 Host API + 可换 `{game}` 核 + 用户可选自带 LLM API** → 可外网玩的游戏平台。  
小游戏是 Host 的压力测试；**无人门禁**（`npm run test:uxe:all`）代替人肉点鼠标。

## 三块积木

| 积木 | 现在 | 平台要什么 |
|---|---|---|
| **Host API** | 时间 / 输入 / 帧 / UXEP·GPU / 资源 / 日志 | 按卡住点加厚；规划 `host_llm` |
| **引擎** | `engine.wasm`（=`ujs_full`） | eng ABI 下沉；与业务核永远分开 |
| **游戏** | Asteroid · 无人机（Pages）；大富翁见 [`archive/`](archive/) | **换核模板**可复制 |

外网：`/docs/index.html` · `/docs/uxe/{name}/`。

```bash
cd ujs && npm run ship:pages && npm run test:uxe:all
```

## 用户自带 LLM（BYO API）

| 层 | 职责 |
|---|---|
| 页面壳 | 填 key / 选模型；永不写入 URL 或 UXEP |
| Host | `host_llm_*`（规划）：代发、超时、剥 key |
| 游戏核 | 结构化请求/回复；可降级为无 LLM 可玩 |

详见 HOST_ABI §7.1。

## 作者路径（目标）

```
写 *.ujs + 核只调 host_*
  → npm run ship:pages（或单游戏 ship 钩子）
  → docs/uxe/{game}/ + 索引（过可玩性门槛）
  → （可选）壳打开 LLM
```

今日缺口：① eng ABI 下沉 ② 统一 bake/模板 ③ host_llm 有玩法再开。

## 明确不做

- Three 进 wasm · 合包 · 完整浏览器 JS 当玩法语言 · 用户 key 进仓库/packet

## 相关

| 文档 | 用途 |
|---|---|
| [`HOST_ABI.md`](HOST_ABI.md) | Host API 真源 |
| [`README.md`](README.md) | UXE 目录 |
| [`archive/`](archive/) | 下架车 |
| [`../../FUTURE.md`](../../FUTURE.md) | 产品化下一刀 |
