# water-keeps S0 静帧元素(出图前审)

固定相机、固定种子。倒影占位。不写玩法。

| 元素 | UXE 能力 | 素材 |
|---|---|---|
| 金色渐变天空 | 贴图平面 `MESH_PLANE` + `mapId` | `.cx` 生成渐变 PNG/RGBA,`assets.tsv` 行 `sky-gold`(来源=程序生成) |
| 云 | 同贴图精灵 billboard | `.cx` 噪声云,`cloud-soft` |
| 远景桥/岸 | 贴图精灵 | `.cx` 剪影,`far-bridge` `far-shore` |
| 睡莲叶 | 自绘小图平面 | `.cx` 叶形,`lily-pad` |
| 芦苇 | 自绘竖条精灵 | `.cx`,`reed` |
| 发光莲花灯 | 点精灵加法 `alphaMode=1` | `.cx` 径向点或实例色,`lotus-lamp` |
| 浮动光点 | 点精灵 | 无贴图实例,`spark` |
| 标题 WATER KEEPS | `scene.text` 屏幕空间 5x7 | 无贴图 |
| 倒影 | 镜像精灵半透明直通混合 | 复用上列贴图,sped 标占位 T5 替换 |

相机: 眼/目标写死在 look-dev 页,seed=0。截图目标 `water-keeps-s0.png`(本步不出图)。
