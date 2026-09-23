# UJS / UNISA — 路线与产品化

> 用效果说话；实践车逼 Host。构造权重，不训练填表。  
> 契约：[HOST_ABI.md](web/engine/HOST_ABI.md) · 平台：[PLATFORM.md](web/engine/PLATFORM.md) · 地图：[DOCS.md](DOCS.md)

---

## 已交付（不必再当「下一刀」）

| 层 | 状态 |
|---|---|
| P0–P0.2 | Three 对照 · 裸 WebGL 实验 · GPU 经薄 JS 绑定 — 收口 |
| **UXE 交付面** | Host ABI v0 · UXEP/UXIN · WebGL/WebGPU · `{game}`≠`engine` 双 wasm |
| **Pages** | `docs/`：Asteroid + 无人机；索引可玩 |
| **无人环** | `npm run test:uxe:all`（`uxe-gate.sh` · `_cdp.mjs` · 禁代理） |
| H1–H2 · **H3 指针** | 雾/光/材质/多 mesh · UXIN v2（含触屏相对摇杆） |

**已否决**：Three-in-wasm · 合包 · 完整 ES · 页内训练。

---

## 实践车

| 车 | 状态 | 教训 |
|---|---|---|
| Asteroid | Pages | 连续操作 + 即时反馈 → Host MVP 够用 |
| 无人机 | Pages · 门禁绿 | 指针/双操控/触屏逼 UXIN；WebGPU Y 勿乱翻 |
| 大富翁 | **归档**（[`archive/`](web/engine/archive/)） | 回合制弱反馈不当门面 |

信念：几款街机已证明 **UJS + Host + 可换核** 可扩展。新游戏是压力测试，不是旁支。

---

## 产品化下一刀（按杠杆）

目标形态：别人能按模板交自己的 `{game}`，外网可玩，验收无人。

| 优先 | 做什么 | 为何像产品 |
|---|---|---|
| **1. eng ABI 下沉** | `eng_sim_step` 双 memory 搬砖 → `engine.wasm` 稳定导出 | 页胶水只剩 Host；三款 ship 最大重复块消失 |
| **2. 换核模板** | `ship:pages` 一条链；新建 game = `*.ujs` + `core-*` + bake 钩子 | 作者路径可抄，不靠口头传统 |
| **3. 门禁即契约** | 改 ABI / 输入 / ship → 必绿 `test:uxe:all` | 人只审玩感；**断言在 snapshot，不在截图** |
| **3b. Snapshot 落地** | `host_debug_snapshot` / `__UXE_SNAP__`（见 prd §6） | 代理自测加速；与门禁共用 |
| **4. Host 按卡住点加厚** | audio · 贴图(H4) · … | 不为对标清单而清单；有玩法再开 |
| **5. host_llm（BYO）** | HOST_ABI §7.1；壳持 key | 智能玩法不代持密钥；纯本地仍可玩 |

```
eng 下沉 + 统一 bake
        ↓
同一套 ship → docs/uxe/{game}/
        ↓
模板复制 → 第三方核
        ↓
（可选）host_llm 打开叙事/裁判
```

### 下一款实践车（备选，非阻塞产品化）

| 候选 | 会撞的 Host |
|---|---|
| 光迹对决（Tron） | 尾迹 BOX |
| 点地塔防 | 指针已有，可直接用 |
| 纵深冲刺 | 与飞行偏近 |

**现定主线是夯底层（上表 1–3），不是再开一款门面游戏。** 新游戏只在逼出 Host 缺口时开。

### 已停泊：WebRTC Live 实验

概念通、产品远。Pages 假源/观看 + `uxe-live` Worker 信令已通，**不作 UJS 直播主线**。  
经验与坑写死在 [`docs/uxe/live/README.md`](../docs/uxe/live/README.md)，供以后 **网络游戏 / DataChannel / host_rtc** 复用。

---

## Host 对标档（摘要）

| 档 | 能力 | 状态 |
|---|---|---|
| H1–H2 | 雾/光/材质/多 mesh | ✓ |
| H3 指针 | UXIN v2 · FLAG_TOUCH | ✓ |
| H3 余 | points · 更多 action | 有需要再开 |
| H4 | 贴图 · buffer 驻留 | 下一能力档 |
| 后加 | audio · storage · **net/rtc** · llm | 独立符号，仍经 Host；rtc 经验见 `docs/uxe/live/README.md` |

原则：加字段进 packet，不加逐物体 `host_draw_*`；永不 `import three`。

---

## 中期加深（A / B / C）

| | 内容 |
|---|---|
| A | C 自举缺口 · UJS host fn / 错误行号 · 克制语言面 |
| B | 第三条「有限表」同构短线（非再造编译器叙事） |
| C | 从规格生成 gold（险；挡在发版与交付之后） |

## 先不做（D）

- 通用 npm 生态抢发 · 完整 ECMAScript · 训练/LLM 填决策表

## 发版

- 核：`scripts/release-artifacts.sh` → GitHub Release  
- 游戏 Pages：`npm run ship:pages` → `docs/`  
- 指纹：`web/BUILD.json` · 工具：[TOOLS.md](TOOLS.md)
