# UXE 家族收口口径

已有要求可调用的同名实现、独立金值或 r160 同场景对照，以及能扰动结果的反例。r160 类提供 toJSON 时还须验证写出和读回。复制、释放与通用渲染接线的剩余限制列在右栏。共用实现只在实际经过同一入口和布局时复用证据。计量分母照旧。

| 家族 | 公共合同和证据 | 已知缺口 |
| --- | --- | --- |
| Material | 生成构造进入同一个 Material 布局。公共字段表生成赋值及 JSON 对象字段往返，copy、clone 和 dispose 使用公共入口。两项 r160 门分别覆盖 JSON 和生命周期。 | JSON 根身份、纹理及图像资源、各派生专有字段和参数接到绘制尚欠。 |
| Light | 七类构造及公共记录，按类读写颜色、强度、半影、地面色、尺寸、SH 和适用阴影参数，共用 copy、clone 和释放入口。`light-class-public-linked.cx` 以 C6 对象和编解码基座验七类从实际实例写出字节并重建，覆盖目标隔离、释放和句柄失效。 | Object3D 的变换、子树、身份、用户数据和阴影相机的其余属性尚未往返。灯光对象到 GPU 光源仍须接线，矩形灯镜面 LTC 待补。 |
| Helper | 实例绑定其源对象或几何资源并复制、克隆和释放，适用公共对象字段需能往返。 | 已验六类复制与克隆及 ArrowHelper 释放。Axes、Box、Box3 共用几何的语义由 helper-family-shared-geometry.cx 对照。ArrowHelper 另证根四元数和轴线、锥体的姿态 JSON 往返，根位置、可见性、图层及资源身份待补。Grid、PolarGrid 和 Axes 已验绑定叶节点的根矩阵、可见性、图层和资源引用读回，截断转红。根身份、子树、资源新建与渲染释放仍缺。SkeletonHelper 验收骨链线段、材质与手动更新根矩阵。四类 LightHelper 验收现有根绑定的矩阵、图层和可见性，保留子节点与共享资源引用，逐类可见性篡改转红。已有灯、骨树及资源由调用方持有，带子节点的灯光辅助对象快照仍限平移根。 |
| KeyframeTrack | 六类共用 JSON 读写，独立验收逐字节往返，r160 核对相同类的序列化与重新载入。 | 读取标准平铺 JSON，最多 64 个时间或值、127 字节名称，字符串池由调用方持有。不覆盖旧式 keys、轨道编辑或自定义容器。 |
| Loader | r160 未定义复制、克隆与 JSON 往返方法。验收通过真实异步请求验证配置、对应解码、失败和回收。 | Audio 的 WAV、Material 的基础 JSON 和 Image 的 PNG 可异步加载。JPEG 已有独立解码入口，异步分派、其他音频容器和材质专有资源待补。 |
| Geometry | 可生成的网格复用 BufferGeometry 的复制、克隆、公开属性往返及释放，逐类与 r160 同场景对照。 | 23 个网格生成门已实跑。Plane、Box、Circle、Cylinder 与 Cone 的 C6 门覆盖参数 JSON、网格复制克隆和释放目标。Box 另验六面材质组，Cylinder 与 Cone 对照端盖及开口。其余 18 类待补参数往返和释放。任意 userData 尚未写入，UUID 由调用方供给。Circle 保留半径为零与大于 3 的非整数分段拒绝。 |
| BufferAttribute | 11 类 JSON 写出经 r160 和独立金值对照，11 个篡改转红。 | 读回草稿复用几何属性解析器。字符串解码依赖须先声明 loader_utf8_width 和 uxe_copy_record。依赖补齐后，`cx ujs/tests/bufferattribute-family-struct.cx -- --round 1` 在 6 秒编译界内返回 124，单路复验同样超时。现由 `linked-wasm-flow.cx` 的 `attribute-json` 入口分别准备基座、检查内容哈希并链接热探针。11 种读回对照通过，错配类型逐项转红。完整对象复制、统一上传接线及超出单记录有界名字空间的输入另列缺口。编译器诊断复现为 `./ujs/ujs2wasm.com ujs/practice/uxe/test/native-undefined-local-repro.ujs -o /tmp/ujs-native-repro.wasm`，实测 rc=1，报 `ujs.com: native function requires an explicit local`，其中调用结果已经用 let 绑定。编译器任务是为未解析的 native 调用点报告符号和位置，并保留局部变量拒绝的原合同。 |
| Raycaster | 相机射线和场景网格递归由 raycaster-camera-io.cx 与 raycast-scene-io.cx 验收。场景篡改使子对象命中失败。 | 统一遍历中的混合对象递归分派与通用 intersectObject 和 intersectObjects 入口待补。 |
| Mesh | C6 门 mesh-object-json-struct.cx 核对根矩阵、可见性、两个 UUID 引用、拒绝未知几何及克隆后的资源共享。释放原节点后克隆仍有效。材质引用调换转红。 | 叶节点记录保留已有几何和材质，由调用方登记 UUID。独立打包资源、多材质数组和子树 JSON 待补。r160 Mesh 使用脱离父节点及垃圾回收，UXE 另有 Mesh_dispose 释放节点槽。 |
| BufferGeometry | geometry-json-load-struct.cx 经 C6 读写连续属性、索引、组和球界。修改解析字段能改变独立金值和 r160 对照结果。 | 参数化类专有字段、共享视图身份、任意元数据和变形属性的通用链路待补。 |

## GPU 模板操作主线复验

从 main `4e6603a` 的 Git 归档新建副本，用 ujs 工作树的 `ujs/ujs.com`，SHA-256 为 `1b9f49c07c103a6854d3748dd6d3e0f196987cacdefe407ff590859128f20f64`。单路执行 `cx -I ujs/tests ujs/tests/gpu-stencil-ops-pixel.cx --`，退出码为 0。

```text
GPU_STENCIL_OPS ops=8 variants=3 stencil_values=24 failed=0
```

同一归档副本追加三次单路复验，每次间隔约 2 秒，退出码和原始输出如下。

```text
第 1 次 rc=0  GPU_STENCIL_OPS ops=8 variants=3 stencil_values=24 failed=0
第 2 次 rc=0  GPU_STENCIL_OPS ops=8 variants=3 stencil_values=24 failed=0
第 3 次 rc=0  GPU_STENCIL_OPS ops=8 variants=3 stencil_values=24 failed=0
```

状态为已关闭。先前浏览器读回失败的成因未查明，可能为偶发。

## A 栏 35 项分组

以 b07f6b9 的燃尽表为基线。每组记录晋级首先要补的合同，表中其余方法缺口继续由燃尽表逐项追踪。

| 组 | 类与缺口 |
| --- | --- |
| 纹理 JSON 与生命周期 | Texture 缺完整公共字段与图像引用往返。FramebufferTexture 缺捕获源引用与参数往返。DepthTexture 缺附件描述往返与独立复制。CompressedTexture、CompressedArrayTexture、CompressedCubeTexture 缺 mip 描述往返和复制，分别覆盖二维、数组层、六面。 |
| 对象 JSON 与生命周期 | Object3D、Scene 缺递归对象及场景资源往返。LOD 缺阈值、迟滞和子对象引用。InstancedMesh 缺矩阵、颜色与实例数，BatchedMesh 缺实例和几何批次状态。CubeCamera 缺子相机、六面目标引用与复制合同。 |
| 曲线 JSON 与生命周期 | ArcCurve、EllipseCurve 缺参数往返和克隆。CatmullRomCurve3、SplineCurve 缺控制点复制和往返。CurvePath 缺子曲线递归往返及克隆。 |
| 缓冲与动画 JSON | InterleavedBufferAttribute 缺共享缓冲身份和克隆往返。InstancedBufferAttribute 缺除数及数组往返。KeyframeTrack 缺基类轨道往返和克隆。 |
| 音频对象与宿主接线 | AudioListener 缺过滤链、对象复制和往返。Audio 缺播放节点、暂停与对象往返。PositionalAudio 缺锥形衰减、宿主接线和对象往返。 |
| GPU 目标与渲染接线 | WebGLRenderTarget、WebGL3DRenderTarget、WebGLArrayRenderTarget、WebGLMultipleRenderTargets、WebGLCubeRenderTarget 缺具名构造及尺寸、复制、克隆与资源释放的类级验收。WebGLRenderer 缺完整场景入口及上下文恢复。PMREMGenerator 缺场景和等距柱状入口与完整预滤链。 |
| 加载器接线 | CubeTextureLoader 缺通用配置和六面图像解码接线。ObjectLoader 缺资源表、材质数组和骨架绑定。 |
| 着色器与格式映射 | ShaderChunk 缺内建片段库和 GPU 接线。ShaderLib 已有五个程序的 r160 uniform 键与换键反例。standard 与 physical 共用本仓基础 GGX 程序，physical 的额外着色语义和 r160 阶段源码等价待验。WebGLUtils 已有非压缩格式门，压缩扩展待设备证据。 |

## 网格与纹理对象升级核对

这批类的复制、克隆或资源分派已有独立验收及反例。r160 的网格类继承 Object3D.toJSON，纹理类继承 Texture.toJSON，Skeleton 自有 toJSON。表中记录各类 JSON 往返的证据或缺口。

| 类 | 已通过的验收 | 升级所缺的 JSON 往返 |
| --- | --- | --- |
| SkinnedMesh | skinnedmesh-clone-struct.cx 验共享骨架、绑定模式与独立绑定矩阵。skinnedmesh-bind-json-struct.cx 对照 r160 的骨架 UUID、绑定模式、矩阵与重算的逆矩阵，篡改模式转红 | 绑定字段已往返。完整网格对象、骨架资源表和通用渲染接线仍待补 |
| InstancedMesh | instancedmesh-clone-struct.cx 验活跃数量、矩阵和颜色副本 | 实例数量、矩阵与可选实例颜色 |
| BatchedMesh | batchedmesh-copy-struct.cx 独立验预留表与副本隔离 | 批次几何及实例状态。r160 直接克隆触发 image.slice TypeError，当前复制门使用独立金值 |
| LOD | lod-lifecycle-struct.cx 对照层表、迟滞和子树克隆 | 自动更新标志、层级阈值及绑定子对象 |
| Skeleton | skeleton-clone-struct.cx 对照骨表、逆矩阵与清零调色板。skeleton-json-struct.cx 对照 r160 完整 JSON 往返，矩阵篡改转红 | 已验收骨架 UUID、骨 UUID 列表及逆绑定矩阵。缺失骨具名为拒绝状态，调用方需提供骨绑定 |
| DataTexture | data-texture-object-struct.cx 对照 RGBA8 采样及源共享。data-texture-image-json.cx 读回 Source 和十项公共参数，像素及环绕篡改转红 | 已验收 RGBA8 图像和十项参数的往返。完整 Texture 元数据与其他数据格式待补 |
| CubeTexture | cube-texture-object-struct.cx 对照面分派及容量拒绝。共用 C6 纹理门验 Source URL 六面顺序，交换两面转红 | 支持由调用方提供已解码面句柄。图像解码与 GPU 跨面过滤另走独立入口 |
| CanvasTexture | canvas-texture-object-struct.cx 对照真实画布、刷新和克隆。共用 C6 门验 Source UUID、快照 URI 字节与刷新版本，省略刷新转红 | 画布编码和 URI 图像解码由调用方提供 |

## 压缩纹理软件范围与设备缺口

BC1 到 BC5 及 ETC1 已有独立软件像素验收。ASTC 直接软件入口支持 14 种二维足迹的 LDR 常色块和合法有限范围，sRGB 保留编码字节。HDR、非常色块的分区、权重及端点模式仍缺。EAC 的有符号与无符号 R11、RG11 可通过 CompressedTexture_decodeEAC 解码，输出保留 11 位精度的交错 16 位小端样本。ETC2 可通过 CompressedTexture_decodeETC2 解码 RGB、全 Alpha 和单比特透明，包含 T、H 与平面模式。sRGB 保留编码字节。ETC2 与 ASTC 常色块已接通二维、数组及立方体的共用软件 mip 分派。EAC 精确 11 位输出使用独立入口，ETC2、EAC 与 ASTC 的压缩原样 GPU 上传待验。BC4 与 BC5 的有符号输出为二补码 SNORM。

真实设备对压缩源的原样上传、格式扩展、mip 完整性、数组分层、立方体六面采样及释放仍待取证。r160 对照在这些软件门中只用于枚举与纹理描述，像素期望由独立金值给出。硬件格式的 A/B 计量保持原口径。

## 几何与材质尚需的类专有验证

(1) EdgesGeometry 与 WireframeGeometry 的嵌套 PlaneGeometry 源已验，其他源类型和通用资源图仍待接。PolyhedronGeometry 的数组、半径和细分已验，专有复制与释放仍待补。

(2) ShapeGeometry 已验单 Shape 的直线外环和洞。ExtrudeGeometry 已验单轮廓、无倒角的轴向挤出，孔洞、倒角和路径挤出待补。TubeGeometry 已验 LineCurve3 的开闭管道，其他曲线及其标架待补。

(3) MeshPhysicalMaterial 的无贴图字段已验，贴图接线、GPU 透射、独立法线和间接光仍待补。ShaderMaterial 与 RawShaderMaterial 已验源码、GLSL 版本、标量和三维向量 uniform，其他 uniform、extensions 及运行时对象接线待补。
