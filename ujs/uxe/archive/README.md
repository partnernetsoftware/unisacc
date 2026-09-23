# UXE · 归档

不进 Pages 索引、不进 `test:uxe:all`。源码保留供对照与拆件，默认开发路径不要再改这里。

| 项 | 路径 | 为何归档 |
|---|---|---|
| **城市大富翁** | `../archive/monopoly/app-monopoly.js` · `../demo/monopoly/` · `../ship/monopoly/` · `../ship/build-monopoly-pages.mjs` | 回合制 + 弱反馈，可玩性不够当门面 |
| **一次性 debug 探针** | `../demo/_debug_probe.mjs` | 未接 `_cdp.mjs`；正式门禁用 `test:uxe:*` |

可选复活（不进默认门禁）：

```bash
npm run test:uxe:monopoly:rules
npm run test:uxe:monopoly
npm run test:uxe:monopoly:ship
# 本地 Pages 镜像（会写 docs/uxe/monopoly；正式 ship:pages 会删掉它）
node web/uxe/ship/build-monopoly-pages.mjs
```

新实践车请落在 `demo/{name}/` + `core-{name}.js` +（可玩后）`ship:pages` 流水线，并加 CDP 探针。
