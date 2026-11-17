# 三款 demo 从零实现(替代 three.js)

先前把整数金、24 槽列表、8 个三角形岸当成移植,那不是本计划。本计划从零做三款可玩 demo,世界与步进尽量进 `.ujs`/wasm,Host 只光栅与输入。不引进 three、不搬原作 GLB。

参考:

| 编号 | URL | 完成时玩家应看到 |
|---|---|---|
| A | https://x.com/beyondtheboxcn/status/2122472282824065492 | 铅笔+水彩的可逛 3D 场景:石桥、树、屋、天,镜头平移时风格不裂 |
| B | https://x.com/anvithbuilds/status/2122408638950371579 | 与片中同一类可转/可看的三维一发 demo(先逐帧定内容再锁玩法) |
| C | https://what-the-water-keeps.vercel.app/ | 灯舟沿水道过 8 个观感不同的界;可舵可随流;灯、水、岸是场景不是色块 |

现状作废为起点:现有 `water.ujs`/`ink.ujs`/`oneshot.ujs` 与三页只作废料,实现时覆盖,不在其上打补丁充数。

## 0. 共同架构(替代 three 的合同)

```
.ujs 世界表 + 步进  --ujs.com-->  .wasm
                                    |
页 Host: 读 UXIN/键盘,调步进或读表,光栅(2D 或裸 WebGL)
不准: import three, 不准页内再写一份物理当真相
```

核一次能吐的结构必须够画一帧或一段航程。Host 不得发明岸线/树位。

## 1. 实现前必须补的 wasm/ujs(一类一条,按阻塞排)

移植过程已撞实、不做就无法「完全」:

| 序 | 缺口 | 不做的后果 | 建议一类 |
|---|---|---|---|
| W1 | 字面量 list 最长 64 | 8 界只能各 3 点,场景装不进 | 放宽 list 上限或 `list(n)` 填表走事实表,不抬棘轮 |
| W2 | 一次 `main_export`、浏览器局部每调清零 | 舵无法进核 | 浏览器可调的步进(UXE `sim.wasm`/`host_input_read`,或 host1 内存槽不重跑 let) |
| W3 | 无网格/提交面 | 只能 `fillRect` | `host_gpu_submit` 吃核打的点/色包(HOST_ABI 已写未兑现) |
| W4 | 无嵌套 list/长 dict key | 8 界不能一表多岸 | 与 W1 同刀或下一刀 |
| W5 | CLI 无 `--frames` | 同进程多帧只能 `frame-run.cx` | 等 `ujs_main` 迁 L0 后加,不夹进本计划 |

本计划演示可先 W1+W2 够 C 可玩;A 的水彩边要 W3 才像片。W3 单列一类,不与 W1 混。

## 2. 三款从零步骤

### A 水彩可逛世界

(1) 从参考片截关键帧,锁:桥、树冠、屋脊、天空四类形,不做角色。(2) `.ujs` 出分层折线(远天、中树、近桥),坐标整数。(3) Host 按层描边+半透明填,镜头 WASD/拖。(4) 完成:拖镜头 10 秒仍是手绘层,不是单色椭圆。

### B three 一发

(1) 把 1080p 片拆成静帧,写一页「片中物体清单」(未看清片不得开写)。(2) `.ujs` 出物体位姿表。(3) 裸 WebGL 画(scissor/三角,无 three)。(4) 完成:拖轨/自动转与片同一类空间关系。

### C 灯舟八界

(1) 原作 8 界只借观感名与顺序,不抄资源。(2) W1 之后 `.ujs` 每界:岸折线、水位、灯色。(3) 步进:航向、流速、界切换;有 W2 则核吃舵,否则 Host 只把舵写成 UXIN 字节交给核(不得另算岸)。(4) 完成:WASD 能从界 1 撑到界 8,每界岸形和 pal 不同,灯在舟首。

## 3. 分期

P0(可玩):C 八界开船,地图全在 `.ujs`(先做 W1)。A/B 出片单与第一层折线。

P1(像):A 分层水彩;B 按片单上物体;C 水波与灯(仍 Host 光栅)。

P2(核):W2 舵进 wasm;W3 提交面包点。未完成不称替代 three。

## 4. 验收

- 三页 `?auto=1` 只核 wasm 表长度与校验和,不核截图。
- 人工:C 开到第 8 界;A 平移;B 对照片单。
- `run-host` 28 行不动;不升 `ujs.com` 0.0.7;不引进 three。

## 5. 禁区

three.module、原作 GLB/音频、抬 cxraw/cxtrunc、改 host1-pass 28 行、把扫描器改 ask 网来画。

## 6. 下一动作

W2:`host1-runtime.step` 已有;四 demo wasm 仍 `globals:[]`,须改 `.ujs` 为注入式才算步进。B 片单未锁前不扩 oneshot 网格。

## 7. 进度

Host 已按表画 water128 岸+灯、ink 三层 x,z 折线、oneshot 6xyz;fly 无表不发明岛。四页已接 `host_gpu_submit`(失败回退)。`host1Boot.step` 已入库。water `dist`、ink `cam`、oneshot `ang` 已进核。fly 金 3868 与 globals 未并存。不称替代 three。

