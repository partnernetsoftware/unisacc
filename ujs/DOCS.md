# UJS 文档地图

入口按读者分流。**文档必须与代码同步**：改 ABI、目录或门禁时，同会话改相关 README。

**命令真源**：[`package.json`](package.json) scripts · [`scripts/uxe-gate.sh`](scripts/uxe-gate.sh) · [`scripts/ship-pages.sh`](scripts/ship-pages.sh)。

---

## 三层一句话

| 层 | 是什么 | 目录 |
|---|---|---|
| **产品 API（core）** | `bootRuntime` / `wasm_run` / `compile` | [`core/`](core/) |
| **UXE 嵌入** | Host ABI + `{app}` 核 + GPU | [`uxe/`](uxe/) |
| **构造侧** | Python gold → 权重 / `web-build` | [`construct/`](construct/) |

---

## 产品 / 开箱

| 读什么 | 何时 |
|---|---|
| [`README.md`](README.md) | 安装、`wasm_run`、分发 |
| [`core/README.md`](core/README.md) | core 文件表 |
| [`prd.md`](prd.md) | 产品活规格 v1.1 |
| [`TOOLS.md`](TOOLS.md) | CLI、发版、门禁 |
| [`package.json`](package.json) | 版本与 npm scripts |

---

## UXE

| 读什么 | 何时 |
|---|---|
| [`uxe/README.md`](uxe/README.md) | demo vs ship |
| [`uxe/HOST_ABI.md`](uxe/HOST_ABI.md) | Host API 真源 |
| [`uxe/PLATFORM.md`](uxe/PLATFORM.md) | 平台叙事 |
| [`../docs/uxe/live/README.md`](../docs/uxe/live/README.md) | WebRTC 实验经验（非主线） |
| [`FUTURE.md`](FUTURE.md) | 下一刀 |

**交付面（外网）**

```
/docs/index.html
/docs/uxe/engine.js
/docs/uxe/{asteroid,drone,live}/
```

本地（`npm run demo` 服务于 `ujs/`）：

```
/web/          playground
/uxe/demo/     源码测
/uxe/ship/     发布对照
/core/         产品 wasm / ESM
```

```bash
npm run ship:pages
npm run test:uxe:all
```

---

## 构造侧

| 读什么 | 何时 |
|---|---|
| [`construct/README.md`](construct/README.md) | gold / `web-build` → `core/` |
| [`native/README.md`](native/README.md) | C VM / game wasm |

---

## 同步规则

1. 产品规格认 `prd.md`。  
2. 改 `host_*` / UXEP·UXIN → `uxe/HOST_ABI.md` + 门禁。  
3. 改 core 布局 / exports → `core/README.md` + `package.json` + `tests/ujs.sh`。  
4. 过期实践车 → `uxe/archive/`。
