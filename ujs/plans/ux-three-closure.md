# UXE 与 three.js r160 清单收口

本报告沿用 three-inventory.tsv 的分母。全量复核依据清单证据和已知缺口记录；A、B 栏只扣除当前明确核出的重大缺口，因此全量比例列为上限。

| 栏 | 已有 | 部分 | 缺 | 不适用 | 可验收分母 | 达成率 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| A 核心能力 | 212 | 0 | 2 | 1 | 214 | 99.07% |
| B 常量与枚举 | 200 | 0 | 0 | 1 | 200 | 100.00% |
| C 扩展能力 | 24 | 2 | 4 | 0 | 30 | 80.00% |
| C 栏其中保留重大缺口 5 项 | 19 | | | | 30 | 全量口径 19/30（63.3%） |
| A 栏其中至少保留重大缺口 9 项 | 至多 203 | | | | 214 | 全量口径至多 203/214（94.9%） |
| B 栏其中至少保留重大缺口 25 项 | 至多 175 | | | | 200 | 全量口径至多 175/200（87.5%） |

清单口径以同名实现、独立金值或 r160 对照及篡改反例为准，全量口径只计无已知重大缺口的条目。

(1) C 栏扣除 DRACOLoader、KTX2Loader、GLTFExporter、OutlinePass 和 SSAOPass，具体缺口见下表。

(2) A 栏扣除三类 CompressedTexture（设备直传及 ASTC 非常色块）、WebGLRenderer（上下文恢复）、MeshPhysicalMaterial（GPU 透射等）、RectAreaLight（镜面 LTC）、PMREMGenerator（完整预滤链）、Raycaster（混合对象递归分派）和 ShaderLib（额外物理着色语义）。

(3) B 栏扣除 BC7 与 BC6H 三项、PVRTC1 四项、ASTC 十四项、ETC 三项及 DisplayP3ColorSpace 一项，涉及未覆盖的解码模式、设备上传或浏览器画布读回。

(4) DXT1 RGB、DXT1 RGBA 与 DXT5 经 WebGL2 压缩上传、采样及像素读回，对照 r160 枚举和独立金值。

(5) ETC1、ETC2 RGB、ETC2 EAC RGBA 用软件像素门验收。PVRTC1 四项对照 r160 描述符以及 Khronos 的有效尺寸、块几何与字节预算。

(6) BC7 模式 6 的 RGBA8 解码通过 43 项，选择器和 P 位变异各使一项失败。BC6H 有符号与无符号模式 11 的 RGBA16F 解码通过 64 项，索引和符号格式变异各使一项失败。共用软件 mip 与快照门共 27 项通过。

## 已知缺口与未证实项

| 项目 | 已证实 | 待证实 |
| --- | --- | --- |
| BC7 | 模式 6，含插值、裁边与拒绝不写入 | 模式 0 到 5、模式 7 与设备直传 |
| BC6H | 两种符号格式的模式 11，HDR 位模式保留 | 其他模式与设备直传 |
| PVRTC1 | 四种枚举、位深、通道、块足迹与字节预算 | 软件解码、真实扩展上的压缩上传与像素读回 |
| ETC 与 ASTC | ETC 软件解码，ASTC 14 种 LDR 常色足迹 | 压缩原样上传，ASTC 非常色块与 HDR |
| SkinnedMesh | 索引位置蒙皮接场景绘制。UXE 一张材质纹理，r160 另用四张骨骼调色板纹理。位姿和计数反例各自转红 | 带法线的蒙皮及骨架资源表 |
| VideoTexture | r160 清单条目保留为缺 | 真实视频元素解码帧、刷新与渲染读回 |
| createCanvasElement | r160 清单条目保留为缺 | 真实浏览器创建画布、命名空间、上下文和绘制读回 |
| Display P3 | 色域变换 | 真实浏览器 Display P3 画布读回 |
| SMAAPass | 边缘检测、面积表采样与错索引反例。搜索纹理 2178 个红通道字节与 r160 GPU 读回一致 | 同图的完整权重目标和最终混合未验，保留部分 |
| GLTFLoader | 同一 GLB 的场景节点、属性、稀疏 UV、采样器及内嵌 PNG。PBR、蒙皮与动画另有组件门 | 完整场景的材质、蒙皮与动画装配及失败回收尚未统一验收，保留部分 |
| KTX2Loader | 原始 RGBA8 双级 mip，线性及 sRGB，交换级次反例 | Basis 转码、其他 Vulkan 格式、设备格式选择与压缩上传 |
| DRACOLoader | 顺序原始 FLOAT32 POSITION 与直存索引，错索引反例 | 压缩索引、量化属性、EdgeBreaker 与异步资源释放 |
| GLTFExporter | 单个索引网格 GLB、平移与 PBR 颜色，r160 回读与索引反例 | 多节点、纹理和动画导出 |
| OutlinePass | 深度掩模、遮挡边缘、双轴模糊、叠加与最终加色 | 自动选取对象、内部目标调度和缩放 |
| SSAOPass | 正交与限定视锥的透视场景，遮挡和无遮挡读回，翻转深度比较反例 | 通用法线、任意透视、模糊及场景合成 |
| HTMLMesh、CSS2DRenderer、CSS3DRenderer、XRButton | 4 项保留为缺 | 真实 DOM 布局与合成、HTML 纹理交互及 XR 会话读回 |

家族内部粒度和产品接线的其他限制见 [家族覆盖记录](ux-three-family-coverage.md)。当前 D1 到 D4 的迁移状态见 [工作队列](ux-three-queue.md)。
