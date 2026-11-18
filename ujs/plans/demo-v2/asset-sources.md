# UXE 素材路线实验

截至 2026-11-19，以下许可均来自作者发布页或作者在素材平台的条目。选型时继续保留单个素材的出处、版本及许可，便于复查具体文件。

| 场景与对象 | 来源与许可 | 观感判断 |
| --- | --- | --- |
| D3 谷底石块 | Quaternius 的 Rocks，CC0，Poly Pizza 提供 GLB。[单件发布页](https://poly.pizza/m/OQvi8PIZ40) | 小网格适合先试谷底路边的中远景石块。冷灰硬面需要接近桥体的暖灰、分缝和旧化才适合近景。 |
| D3 叶丛与石块 | Quaternius Ultimate Stylized Nature Pack，CC0，作者明确可商用，提供 glTF、贴图及法线贴图。[作者发布页](https://quaternius.com/packs/ultimatestylizednature.html) | 先挑一株叶丛看透空、明暗块和远景轮廓，再决定是否替换手绘叶团。法线贴图与透明裁切需要单独验收。 |
| D4 屋顶、窗与檐 | Quaternius Medieval Village MegaKit，CC0，作者明确可商用，提供 glTF。免费层和素材可获得性随发布层级而异。[作者发布页](https://quaternius.com/packs/medievalvillagemegakit.html) | 网格对齐的屋面、窗框和外挂部件适合增加城镇层次。优先取零件，保留现有城镇的高矮关系和机械轮廓。 |
| D4 厂房轮廓 | Kenney City Kit (Industrial)，Creative Commons CC0。[作者发布页](https://kenney.nl/assets/city-kit-industrial) | 厂房轮廓可作远层对照。原套件的现代构件与近景风格差距较大。只核对了发布页，未下载验证文件格式。 |

## 从 glTF 或 GLB 到 UXE

| 能力 | 已接通的单件实验 | 通用载入仍需要 |
| --- | --- | --- |
| 网格 | `.cx` 校验 GLB 2.0 容器和 `bufferView`、`accessor` 范围，读一组浮点位置与法线、16 位索引，按三角形展开给 UXE 的 `extraMeshes`。应用模型子节点的平移、四元数和均匀缩放。 | 多节点层级与多 primitive、非均匀缩放后的法线变换、规范化整数与稀疏 accessor、外部缓冲和 URI、压缩扩展、按索引绘制或分批上传。 |
| 材质 | 读取 `baseColorFactor` 的 RGB，交给 UXE 的固定光照和 toon。试验页自行设定粗糙度和金属度。 | 将 glTF 的金属度、粗糙度、双面与透明模式映射到 UXE，并对色彩空间、PBR 与风格化表现作对照验收。 |
| 贴图 | 石块原件没有纹理，网格使用纯色。 | 载入 PNG/JPEG、内嵌图片与外链、UV、采样器、色彩空间和纹理变换。D3 叶片还需要 alpha mask、双面法线和远处采样稳定性。 |
| 骨骼 | 使用静态节点姿态。 | 载入 `skin`、逆绑定矩阵、`JOINTS_0`、`WEIGHTS_0`、动画通道与插值。需要可验证的关节调色板和 CPU 或 GPU 蒙皮。D3 静态石块和 D4 建筑可先于骨骼验证。 |

相关结构见 [Khronos glTF 2.0 规范](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html)。

## 首帧证据

原件为 [Quaternius Rocks GLB](https://static.poly.pizza/c6f65def-08c5-4c81-b17f-f37b171ad911.glb)，5940 字节，SHA-256 `f2085fb5edfe6ec33b7fe9dd8969fdc016900c18649b94c9ba703d8413cdcd97`。`.cx` 读出 156 个源顶点、252 个索引、84 个三角形。100 字节的截断文件返回 1，无转换产物。

单次本机实测，原件读取 35 微秒，解析、展开与 JSON 写出合计 5337 微秒。Chrome SwiftShader 对相同字节的转换产物实读，页面读取并整理 JSON 5.0 毫秒，WebGL 建立与网格上传 117.7 毫秒，首绘加同步像素读回 342.7 毫秒。中心像素为 `(33,41,53,255)`。这些数据包含该次软件渲染初始化与同步等待，尚无真机或连续帧样本。

首帧为 `/tmp/uxe-rock-first.png`。原件、转换结果与截图只留在 `/tmp`。仓内保留 [解析实验](../../practice/uxe/asset-study/rock-glb.cx)和[首帧页面](../../practice/uxe/asset-study/rock-preview.html)。这次证据只覆盖上述单个静态资产。

```sh
curl -fLS https://static.poly.pizza/c6f65def-08c5-4c81-b17f-f37b171ad911.glb -o /tmp/cdx2-uxe-rocks.glb
sh ./unisacc.com ujs/practice/uxe/asset-study/rock-glb.cx -- /tmp/cdx2-uxe-rocks.glb /tmp/cdx2-uxe-rocks.json
google-chrome --headless=new --use-angle=swiftshader --enable-unsafe-swiftshader \
  --allow-file-access-from-files --virtual-time-budget=5000 --window-size=960,627 \
  --screenshot=/tmp/uxe-rock-preview.png "file://$PWD/ujs/practice/uxe/asset-study/rock-preview.html"
```
