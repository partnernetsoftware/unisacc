# UXE Live — WebRTC 经验札记

> 活文档：概念已通，产品远未到 YouTube。  
> **现状**：Pages 壳 + Cloudflare Worker 信令 + 浏览器 P2P；**不是**可运营直播站。  
> 目的：把踩坑写死，供以后 **网络游戏 / 实时同步** 复用，避免重踩。

配套页面：

| 页 | 角色 |
|---|---|
| [`index.html`](./index.html) | 观众（看） |
| [`host.html`](./host.html) | 假源（画布 → `captureStream` → WebRTC） |
| Worker | `https://uxe-live.kcc668.workers.dev`（仓内源：`ujs/cloudflare/uxe-live/`） |

信令：`wss://uxe-live.kcc668.workers.dev/room/{id}`

---

## 1. 我们实际做成了什么

```
host.html ──WebSocket join(host)──► LiveRoom (Durable Object)
viewer    ──WebSocket join(viewer)─►        │
                │                            │ fan-out SDP / ICE
                └──────── WebRTC P2P ────────┘
                     (媒体不经 Worker)
```

- **Worker 只做信令**（房间、peer 进出、`signal` 转发），**不碰音视频字节**。  
- **媒体面**在浏览器：`RTCPeerConnection` + STUN（`stun.cloudflare.com:3478`）。  
- 假源用 **canvas 像素** 决定「源分辨率」，再用 `RTCRtpSender.setParameters` 要码率 / 禁止 `scaleResolutionDownBy`。

这证明：**无自建机房也能跑通「开播 → 进房 → 出画」**；也证明 **距离「类 YouTube」还差整个栈**。

---

## 2. 和 YouTube 差在哪（别自我安慰）

| YouTube / 商用直播 | 我们现在 |
|---|---|
| 上传 / 转码 / 多码率 ABR | 单路原始 WebRTC，观众侧几乎无阶梯 |
| CDN 全球分发、百万观众 | **网状 P2P**：每多一个观众，主播多一条上行 |
| 录制回放、推荐、账号 | 无 |
| SFU / 转推 HLS/DASH | 未接；对称 NAT 时还缺可靠 **TURN** |
| 运营级卡顿/延迟监控 | 只有页面 log + snapshot 思路未接到媒体 |

**结论（产品）**：UJS / UXE **暂不适合当直播产品主线**。  
**结论（平台）**：WebRTC + Worker 信令是 **实时联网能力的第一块砖**，给以后 **网络游戏、同步台面、遥控** 用。

---

## 3. 已踩坑（务必保留）

### 3.1 「很糊」多半不是 CSS

- 早期假源画布 **960×540**，放大到全宽自然糊。  
- WebRTC 还会 **自适应降码率 / 降采样**；不调 `maxBitrate`、`scaleResolutionDownBy=1` 时，看起来像「分辨率参数没生效」。  
- **显示尺寸 ≠ 推流像素**；host 页已加 360p–1080p / fps / 码率选择（开播前锁定）。

### 3.2 信令 ≠ 媒体；Secret 不能进浏览器

- Cloudflare Realtime **SFU** 若要用，**App Secret 必须停在 Worker**，浏览器只拿短期会话。  
- 当前未接 SFU：纯 P2P，实现简单，**扩展性差**（见上表）。

### 3.3 NAT / 网络

- 仅 STUN 时，部分运营商/公司网 **连不上或单通** → 以后要 **TURN**（可付费，按 GB）。  
- 本机曾出现 **API 可部署、workers.dev 探活超时**（出口网络问题）；以浏览器实机为准。

### 3.4 自动播放与静音

- 观众页 `video` **muted** 才能稳自动播；真要听声需用户手势 unmute。  
- 假源加了近乎无声的音轨，部分移动端管线更稳。

### 3.5 房间与角色

- DO 房间按名字隔离；`host` / `viewer` 只是附件角色。  
- 主播对每个 viewer **单独建 PC、单独 offer**（网状）。人一多主播上行炸 — **预期内，不是 bug**。

### 3.6 Token / 部署纪律

- `~/env.jsonl` 里 **`wkrpns@cloudflare`** 用于部署；旧 `dataxwin@…` token 曾失效。  
- **只部署新 Worker 名 `uxe-live`**，禁止改写账号里其它脚本。

---

## 4. 协议便签（复用时别重发明）

信令 JSON（WebSocket）：

```text
{ type: "join", role: "host"|"viewer", name? }
{ type: "welcome", peerId, peers[] }
{ type: "joined", peerId, role, peers[] }
{ type: "peer_join" | "peer_leave", ... }
{ type: "signal", to?: peerId, from?, payload }
payload.kind = "offer" | "answer" | "ice"
```

媒体：标准 WebRTC；ICE 至少 STUN；生产补 TURN / 或改 SFU。

---

## 5. 以后若做「网络游戏」怎么复用

不要复用「直播产品叙事」，复用 **能力切片**：

| 能力 | 直播里学到的 | 游戏里可能长成 |
|---|---|---|
| 房间 + 信令 DO | `LiveRoom` fan-out | 对局房、大厅、匹配结果下发 |
| DataChannel | （尚未用） | 状态同步、输入帧、快照差分 |
| 音视频轨 | 假源 / 摄像头 | 语音麦、观战流（可选） |
| Host ABI 方向 | 媒体应停在 Host | 将来 `host_rtc_*` / `host_net_*`，核不知 SDP |
| 代理验收 | 断言连房间/peer 数，而非截图 | 与 `__UXE_SNAP__` 同一哲学 |

**优先顺序建议**（未排期，仅防忘）：

1. DataChannel 心跳 + 延迟统计进 snapshot（比再堆分辨率更有用）  
2. TURN 或 Realtime SFU（解决「连不上」比「更清晰」优先）  
3. 再谈多观众、转码、回放  

---

## 6. 明确不做（当前阶段）

- 点播库 / 永久录像 / 推荐  
- 把编码器或 SDP 塞进 `engine.wasm` / UJS 核  
- 用 UXEP 网格「画」出视频帧当直播  
- 未绿门禁前当产品对外宣传「直播平台」

---

## 7. 改文档时

同会话更新本文件。重大结论变化（例如已接 SFU）在本节顶部加一行日期摘要即可。
