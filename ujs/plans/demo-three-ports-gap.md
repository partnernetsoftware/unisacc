# 三款参考 vs 现状差距(2026-11-17)

只对照 `ujs/plans/demo-three-ports.md` 文字合同与仓内页/核代码。未打开 A/B 的 x.com 片、未打开 C 原站截图像素比对。不记观感通过。

## 共同

| 项 | 参考合同 | 现状 | 层 |
|---|---|---|---|
| 世界真相 | `.ujs`/wasm 出表与步进,Host 只光栅与输入 | 页内 `Scene`/`instances`/`tick` 写船位、岛、雾;核只 boot 一次读 list | 宿主 |
| 禁 three | 不准 import three | 三页无 three,用 `renderer-webgl.js` 网格 | 宿主 |
| W1 list 64 | 放宽或事实表 | 计划 §7:已 64->256;`run-host` 28 绿 | 核(已) |
| W2 步进不重跑 let | 浏览器可调步进 | `host1Boot` 一次;`goldI64`/`readI64List`;无每帧 `host_input_read` | 核 |
| W3 提交面 | `host_gpu_submit` 吃核点/色包 | 页用内置 MESH_* 三角,不走 HOST_ABI 提交 | 核+宿主 |
| 验收 | `?auto=1` 只核表长/校验和;人工开船/平移/对片 | 公开页用 `document.title` FLY_OK/WATER_OK/INK_OK/ONESHOT_OK | UXE |

`demo-three-ports.md` §13:现 `water.ujs`/`ink.ujs`/`oneshot.ujs` 与三页只作废料。公开站点曾挂 fly/water-keeps(及短暂 ink/oneshot 入口),与该条冲突。

## A 铅笔水彩可逛 3D

参考文字:石桥、树、屋、天;镜头平移风格不裂;完成=拖 10 秒仍是手绘层不是单色椭圆。步骤:分层折线,Host 描边+半透明填。

| 项 | 参考 | 现状 `demo/ink-walk` + `ink.ujs` | 层 |
|---|---|---|---|
| 形 | 桥/树冠/屋脊/天空四类 | `MESH_OCTA`/`MESH_BOX`/`MESH_PLANE`;HUD "NPR rim" | 宿主 |
| 画法 | 分层折线+水彩边 | WebGL 网格+rim;无描边半透明层 | 宿主 |
| 核表 | 分层折线整数坐标 | 计划 §7:三层折线 `ink.ujs`(38) | 核 |
| 完成判据 | 平移仍手绘 | AUTO `INK_OK` 当 `readI64List` 长>=12 | UXE |

未核 x.com/beyondtheboxcn 片画面。

## B 片同款三维一发

参考文字:与 Anvith 片同一类可转/可看;先逐帧定内容再锁玩法。步骤:1080p 静帧物体清单(未看清片不得开写)。

| 项 | 参考 | 现状 `demo/oneshot` + `oneshot.ujs` | 层 |
|---|---|---|---|
| 片单 | 必须先锁 | 计划 §7:片单仍未锁 | UXE(缺) |
| 物体 | 按片单位姿 | 6 盒 `MESH_BOX`;list 长 18 | 核+宿主 |
| 完成判据 | 拖轨/自动转与片同一类空间 | AUTO `ONESHOT_OK` 当 `box.length===18` | UXE |

未核 x.com/anvithbuilds 片。

## C 灯舟八界

参考: https://what-the-water-keeps.vercel.app/ ;灯舟沿水道 8 界;可舵可随流;灯、水、岸是场景不是色块。完成:WASD 从界 1 到 8,每界岸形和 pal 不同,灯在舟首。

| 项 | 参考文字 | 现状 `demo/water-keeps` + `water.ujs` | 层 |
|---|---|---|---|
| 8 界 | 岸折线、水位、灯色每界不同 | 计划 §7:8 界各 8 顶点+8 色在 `water.ujs`(136);页用一块 `MESH_PLANE` 当水、`MESH_SHIP`/`MESH_OCTA` | 核有表,宿主未画岸折线 |
| 灯舟 | 灯在舟首,场景不是色块 | 页 `lamp` 小 octa;ccapi 记画面很暗、水面起伏肉眼看不清 | 宿主 |
| 舵进核 | W2 | WASD 在页 `tick` 改 `x/z`;核只启动读 pal | 宿主 |
| 完成判据 | 开到第 8 界 | AUTO `WATER_OK` 当 list 长>=8 | UXE |

未打开 C 原站做截图像素对比。观感不记通过。

## 暂停范围

ink-walk、oneshot 在 `pages.tsv` 标 wip,生成器 `--check` 跳过。首页只留 fly、water-keeps。drone/host1-games 不发布。下一步是需求文档与参考口径,不是继续加公开页。
