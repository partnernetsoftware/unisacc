# handoff 2026-11-18 停手等换窗口

## 原话档

`ujs/plans/demo-v2/requirements-original.md`(只增不改)。prd 0.3 以它为准。

## 视频与帧(不进 docs/)

| | 本体 | 仓内 |
|---|---|---|
| D2 | 无 mp4。卡片 `/tmp/ref/d2-card.jpg`。可玩站 what-the-water-keeps,headless 出不了完整画面 | `ref/d2/` |
| D3 | `/tmp/ref/d3.mp4` sha256 679a804c067ac109c6e155051be646776fee9912a8655760a85039f04698e2ea | `ref/d3/frames` 01 04 07 |
| D4 | `/tmp/ref/d4.mp4` sha256 5052c61911f2c97ee281dd3bba6a32bcd4b0522599ed3a2fce62904291114371 | `ref/d4/frames` 01 04 07 10 13 |
| JSON | `/tmp/ref/d2.json` `d3.json` `d4.json` | |

无 ffmpeg。本机先前无这些视频缓存。

## 各卡状态

- D1 fly: Forge fighterv2,画面 UNMEASURED(headless 空白)。
- D2 water-keeps: 参考=vercel 可玩站。S0 概念稿/UXE 与 c-vt20000 仍有效。取消待确认。
- D3 ink-walk: 视频帧核过多拱石桥小屋阔叶,NPR。仍缺全时长精确抽帧。
- D4 oneshot: 视频帧核过四足机械城堡丘陵积云白烟,NPR。
- D5 ghibli-world: 终局,依赖 D1-D4,当前不开工。

## UXE 优先级

T1 贴图采样 -> T2 素材加载 -> T3 精灵粒子 -> **T4 NPR(描边、赛璐璐色阶、手绘纹理阴影、天空云; D3/D4 前置,先于 T5)** -> T5 倒影/后处理 -> D5 动态世界。

## 已知坑

- D2 可玩站 headless 不完整。
- 无 ffmpeg,chrome seek 多数非 0 秒失败。
- 第三方帧只作仓内对照,不得进 docs/ 与发布页。
- ujs 工作在 ujs 分支做完再合 main; 不 cherry-pick 代办。

## Q2a 石桥几何

基点 3a9d806，本项提交可用 `git log -1 --format=%h -- tools/check-uxe-d3-geometry.cx` 定位。固定光照与机位的 [1280 x 720 截图](look-dev/d3-q2a.png) 对应三孔桥。运行 `tools/check-uxe-d3-geometry.cx` 的 normal、bare 和 sealed 三种输入；sealed 必须失败，bare 应在前景树冠原像素露出桥墩。取点按解析相机投影，屋顶点取右侧坡面中央，不能取山墙内部。下一步先同步 main 并回归，再做 Q2b 的受光色阶。此帧只验几何，最终风格待固定帧与短横移交所有者签字。

## Q2a2 圆弧拱圈

Q2a 提交 271b47b。圆弧版用三行毫米规格定义三个 16 段半圆和贯通的内壁。[固定机位截图](look-dev/d3-q2a2.png) 与 Q2a 可直接比较。拱顶和两个偏心位置取圆方程上下各 100 毫米的投影点，旧台阶在偏心位置失败，压低拱顶也会失败。Chrome 须等 TSV 完成加载再取读回。Q2a2 提交身份可由 `git log -1 --format=%h -- ujs/plans/demo-v2/look-dev/d3-q2a2.png` 查得。下一项先合 main，再开 Q2b。

## Q2 进度

Q3a `898b11c` 将大树冠改为每团 48 片不规则叶片，三档着色。canopy 覆盖 51/120，暗部 23/51；thin-canopy 覆盖 32/120，暗部 5/32，两项失败。Q3b 把桥高拉到 1.6 倍，采用更低近的机位，添瓦屋面、木阳台、烟囱、崖台岩及远岭。构图规格见 `d3-composition.tsv`，`check-uxe-d3-geometry.cx -- composition` 为 4/0，short-composition 为 4/3。当前并排件在 /tmp/d3-review-comparison.png，左侧参考第 3 秒，右侧为新构图。Q3b 提交可用 `git log -1 --format=%h -- ujs/practice/uxe/test/d3-composition.tsv` 查得。下一项 Q3c 细线。

Q2e 签字材料为 [固定帧](look-dev/d3-review.png) 和 [横移短片](look-dev/d3-review-traverse.mp4)。固定光照，以同一机位起步，机眼和目标沿世界 X 轴同步移动 1 单位，12 帧、12 fps、1280 x 720。并排对照 /tmp/d3-review-comparison.png 左侧为 /tmp/ref/d3.mp4 第 3.00 秒，右侧为原创帧。参考图仅放在临时对照件。最终风格待所有者签字。

|补景提交|检查入口与同版本证据|
|---|---|
|Q2e1 `2c67b5e`| `check-uxe-d3-geometry.cx -- sky` 4/0，渐变红值 71 至 93，云上、下为 235 和 203。cloudless 4/4。两个生成文件逐字节检查，篡改天空一字节退出 1。|
|Q2e2 `33104a0`| `check-uxe-d3-geometry.cx -- crowns` 120/0，模板每树九团，树冠样本 48 中 20 个暗部。sparse 120/1。|
|Q2e3 `238a231`| `check-uxe-d3-geometry.cx -- ground` 144/0，取到草簇 28、桥台岩 32 个样本。flat-ground 144/2。|

原几何校验 16/0，色阶 6/0，描边 1 和 3 像素正例零错误，生成物篡改扫查零失败，三个补景提交各自的十项 UXE 回归均为 10/0。Q2e 签字页和短片都用原创材质；小屋造型仍简化，云为有明暗的天空贴图。签字时重点看横移中的透天拱洞、拱腹砌缝、树冠暗块黏附以及草与桥台的前后层次。

Q2d 的原创砌缝与水彩纹由 `tools/gen-d3-masonry.cx` 生成，`--check` 比对 16384 字节；磁盘篡改已纳入 `tools/check-gen-sweep.cx`。用 `d3-bridge.html?style=masonry&shift=0` 看石面，`shift=1` 在同样光照下横移一单位。拱腹沿弧长与深度取样，静止树冠在世界坐标中取水彩纹。下一项 Q2e，Q2d 提交可用 `git log -1 --format=%h -- tools/gen-d3-masonry.cx` 定位。

Q2c 在 WebGL 场景设 `outlineWidth` 启用 1 至 8 像素的内描边。额外绘制不透明物体的可见法线，按法线跳变和背景覆盖画线。立方体中线的 1、3 像素验证入口为 `tools/check-uxe-outline.cx`，`--mutate` 关闭第二轮描边，应返回 1。下一项 Q2d 石砌纹。

Q2b 为 UXE WebGL 材质添加 `toonLevels`，范围为 2 至 16 的整数。立方体使用面法线，D3 内拱使用各圆弧段的几何法线。检查 `tools/check-uxe-toon.cx` 在 SwiftShader 下独立算 2 档与 4 档 RGB。加 `--mutate` 将 4 档改为 3 档，原期望保持不变。下一项 Q2c，Q2b 提交可用 `git log -1 --format=%h -- tools/check-uxe-toon.cx` 定位。

接手分支为 `ujs`，工作目录为 `~/repos/unisacc/.claude/worktrees/ujs`。Q2a `271b47b5e7` 建立真三维三孔桥与低机位。Q2a2 `0c8fddc3c5` 改为 16 段圆弧和有厚度的内壁。督导已分别看图复核；所有者最终风格签字仍在 Q2e。本次同步的 main 输入为 `8cf969aeb3`，冲突表按合并后的批次重新生成，工具编译清单为 149 个分片。接手时先核对 `git status --short --branch` 和 `git log -1 --format='%h %p'`。

下一步从 Q2b 开始，每项单独提交，先证失败再做正例，提交说明和信封附同输入的正常、篡改与回归输出。

|项|实现入口|验收口径|
|---|---|---|
|Q2b 赛璐璐色阶|给 UXE 材质增加可选的 N 级漫反射。先在固定方向光下画已知平面法线的立方体或球体。检查渲染器现有法线来源；拱腹法线应来自圆弧几何。|选择落在不同量化区间内部的面或像素，.cx 从法线与光向的点积独立算档位和 RGB。只改着色器的 N，原 oracle 应以非零退出。默认未启用时保住既有材质合同。|
|Q2c 描边与细线|在法线或深度不连续处生成可控宽度的线，分别处理外轮廓、折角及无轮廓的同面区域。|.cx 投影固定立方体的已知棱线，在线内、线外及平面内部验像素，比较两个指定线宽；篡改宽度或禁用边缘应红。查看桥缝与树冠的边缘区别。|
|Q2d 手绘质感|用 .cx 生成原创石砌纹。桥正面按石面坐标铺，内拱按弧长和深度铺。树冠在不同大明暗组内叠水彩噪声。|生成器 --check 逐字节比对磁盘文件，磁盘篡改应红，加入 tools/check-gen-sweep.cx。固定相机再小幅横移，检查砌缝贴住桥面、绕进内壁；只验元素存在不能代替此项。|
|Q2e 签字材料|同一份场景、资产、光照与种子，保留固定机位主帧，再从同一机位作一小段连续横移。与参考视频第 3.00 秒的低机位并排，时间和位移写清楚。|独立检查桥洞透天、拱腹高度、近树和远屋的相对位移；将主帧和短序列一起交督导及所有者，人工看移动时细铅笔线和柔软树冠能否共存。|

### 接手后的检查

(1) 在 worktree 根用 `sh ./unisacc.com tools/devsvc.cx -- x --par 1 --timeout-ms 55000 -- sh ./unisacc.com tools/run-uxe-regress.cx --` 跑十项，末行应为 `UXE_REGRESS n=10 failed=0`。Q2a2 的几何门单独运行，把内层工具换成 `tools/check-uxe-d3-geometry.cx -- normal`。normal、bare 均为 16/0；sealed 为 16/7、low-crown 为 16/4，两个负例退出 1。bare 在同一像素露出被树挡的墩面。

(2) 截图用 headless Chrome、SwiftShader、`--window-size=1280,807`。签字图取 1280 x 720 的 canvas；整窗截图须裁出 canvas。d3-bridge.html 通过模块读取 d3-arch.tsv，本地 file 测试需允许本地资源并等异步加载，空读回须报错。

(3) water-keeps 的 `s0-elements.tsv` 第 2 至 11 列由 `s0-spec.sha256` 锁定。变更规格需独立提交并写依据。D3 圆弧规格见 `ujs/practice/uxe/test/d3-arch.tsv`，.cx 从圆方程和相机算预期，页面只回传读回值。发现差异先查物理来源。

(4) 开发经 devsvc，仓内改动用 Edit/Write 或 .cx；版本控制也经 devsvc。只动本车道；新工具和批次用 register，逐表跑 own-diff，提交共享表用 commit-own --keyed 全字段键。新清单的 coverage 行常需单列 interfaces.* 键。每次提交后回归并 push origin HEAD:ujs。不要写 main 或发布页。

(5) 视频在 `/tmp/ref/d3.mp4`、`/tmp/ref/d4.mp4`，下载地址和 SHA 在 [D3 来源](ref/d3/SOURCE.txt)、[D4 来源](ref/d4/SOURCE.txt)。ffmpeg 曾可从 `~/.hermes/tools/ffmpeg-9.0.1-linux-x64/bin/` 调用。D3 已核全片，0.25 秒采 52 张；D4 采 208 张，详见 [D3 分析](essence/d3-v2.md)、[D4 分析](essence/d4-v2.md)。评论样本为 `/tmp/x-comments/d1.md` 至 `/tmp/x-comments/d4.md`，共 125 条。其要点、作者归属和永久链接已存入[评论区补证](essence/ESSENCE-v2.md#评论区补证)，临时文件清理后仍可复核。
