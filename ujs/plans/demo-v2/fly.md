# demo-v2 fly

项目所有者 2026-11-18 补录原始需求。此前「无外部参考、另立峡谷穿环」作废。

## 参考

- 试玩: `https://remix.ui8.ai/fly` (308 到 `https://forge.ui8.ai/share/frag_1eb8944c-57b1-42ca-a909-5c2d7e517c24`)。标题 Forge / fighter, 版本 fighterv2, 作者 Dash。平台 `https://ui8.ai/forge`, 可 Remix。
- 取证: ccapi 2026-11-18 headless chrome 只读。画布空白, 画面观感 UNMEASURED, 需所有者录屏或关键帧。参数面板可读, 即功能清单。

## 体验一句话

战斗机飞行: Sprite stack 像素堆叠机身, 追尾相机看岛与云, 可空战与坠毁弹射; 精确切片范围待所有者确认。

## 参数面板(功能清单, 原样)

Helpers: Tutorial and Intel buttons (top right) 开; Touch buttons (auto/on/off)。
Sound: Sound (starts with the first click or key) 开; Volume 0.6; Radio chatter during a rescue 开。
Flight: Crashes (dive into the ground to eject) 开; Speed 92; Thruster boost (x speed while held) 1.8; Turn rate 1.25; Bank angle 52; Cruising altitude 110; Ceiling (highest it climbs) 1500; Ground clearance (lowest it skims) 7; Wingmen 0; Steer with pointer 开; Flight-stick up/down (down climbs, up dives 类) 开; Rolls on autopilot 开。
Combat: Weapons (right-click cannon, M missile) 开; Cannon rounds before a reload 300; Missiles before a reload 6; Reload time 5 秒; Enemy jets (keep one ahead to lock on) 3; Enemies attack (guns and missiles) 开; Dodge slow motion 1.3 秒。
Sprite stack: Jet size 0.6; Slice spacing 1; Pixel size 3(0=auto); Stack-up intro 关。
Camera: View (top / chase / cockpit, 空格切换) 默认 chase; View change 1.4 秒; Overhead: turn with the jet 0.6; Chase: distance 2.6 机长; Chase: height 12 度; Chase: jet width on screen 0.19; Ejection: close-up on the pilot 1。
World: Time of day (day / sunset / night) 默认 day; Islands 0.33; Clouds 0.45; World seed 7; Jet shadow 开。
Look: Glow (fire, sun, engines, glints) 0.8。

## 从面板能确定的事实

- 战斗机飞行, 带救援(电台对话)、空战(机炮、导弹、敌机锁定、闪避慢动作)、坠毁弹射(飞行员特写)。
- 渲染: Sprite stack(切片堆叠像素风 3D 飞机, Pixel size 3); 世界岛与云, 昼/黄昏/夜, 机影与发光。
- 三种视角: 顶视、追尾(默认)、座舱。键鼠/指针与触屏, 教程与情报按钮, 音效与音量。
- 参数可调并可 Remix。

覆盖至少应含: 飞行手感、追尾相机、岛云世界、敌机与机炮/导弹、坠毁弹射、HUD 与教程按钮。精确范围由所有者确认。

## 场景元素(待所有者切第一片)

| 元素 | 必须/可选 | 可检 | 注 |
|---|---|---|---|
| Sprite stack 机身 | 待确认 | 切片飞机可读 | Pixel size 3 |
| 追尾相机 | 待确认 | 默认 chase | 距 2.6 机长、高 12 度 |
| 岛与云 | 待确认 | 非清屏 | Islands 0.33 Clouds 0.45 seed 7 |
| 机炮/导弹/敌机 | 待确认 | 锁定与开火 | 战斗是否进第一切片 UNCONFIRMED |
| 坠毁弹射 | 待确认 | 飞行员特写 | |
| HUD/教程/情报 | 待确认 | 按钮可读 | |
| persist 飞行状态 | 必须 | G4 | 跨帧不重置 |

## 相机与操作

默认 chase; 空格切顶视/追尾/座舱; 指针转向; 触屏按钮。

## 未知(需所有者补)

- 画布实际画面、救援流程、教程内容、音效。请录屏或关键帧。
- 范围: 是否全部战斗/救援, 还是先做飞行 + 岛云 + 追尾相机第一切片。

## 验收

画面观感 UNMEASURED, 禁止 FLY_OK 金值冒充观感。S0 静帧待参考帧到位后再签。

## 不做什么

不恢复峡谷穿环目标; 不打补丁旧 fly 页; 不新增线上入口直到 S4; 不把 headless 空白画布当画面证据。
