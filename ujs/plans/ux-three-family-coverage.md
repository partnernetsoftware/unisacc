# UXE 家族收口口径

已有要求可调用的同名实现、独立金值或 r160 同场景对照，以及能扰动结果的反例。r160 类提供 toJSON 时还须验证写出和读回。复制、释放与通用渲染接线的剩余限制列在右栏。共用实现只在实际经过同一入口和布局时复用证据。计量分母照旧。

| 家族 | 公共合同和证据 | 已知缺口 |
| --- | --- | --- |
| Material | 生成构造进入同一个 Material 布局。公共字段表生成赋值及 JSON 对象字段往返，copy、clone 和 dispose 使用公共入口。两项 r160 门分别覆盖 JSON 和生命周期。 | JSON 根身份、纹理及图像资源、各派生专有字段和参数接到绘制尚欠。 |
| Light | 七类构造及公共记录，按类读写颜色、强度、半影、地面色、尺寸、SH 和适用阴影参数，共用 copy、clone 和释放入口。`light-class-public-linked.cx` 以 C6 对象和编解码基座验七类从实际实例写出字节并重建，覆盖目标隔离、释放和句柄失效。 | Object3D 的变换、子树、身份、用户数据和阴影相机的其余属性尚未往返。灯光对象到 GPU 光源仍须接线，矩形灯镜面 LTC 待补。 |
| Helper | 实例绑定其源对象或几何资源并复制、克隆和释放，适用公共对象字段需能往返。 | 已验六类复制与克隆及 ArrowHelper 释放。Axes、Box、Box3 共用几何的语义由 helper-family-shared-geometry.cx 对照。ArrowHelper 另证根四元数和轴线、锥体的姿态 JSON 往返，根位置、可见性、图层及资源身份待补。Grid、PolarGrid 和 Axes 已验绑定叶节点的根矩阵、可见性、图层和资源引用读回，截断转红。根身份、子树、资源新建与渲染释放仍缺。SkeletonHelper 验收骨链线段、材质与手动更新根矩阵。四类 LightHelper 验收现有根绑定的矩阵、图层和可见性，保留子节点与共享资源引用，逐类可见性篡改转红。已有灯、骨树及资源由调用方持有，带子节点的灯光辅助对象快照仍限平移根。 |
| Loader | r160 未定义复制、克隆与 JSON 往返方法。验收通过真实异步请求验证配置、对应解码、失败和回收。 | Audio 的 WAV、Material 的基础 JSON 和 Image 的 PNG 可异步加载。JPEG 已有独立解码入口，异步分派、其他音频容器和材质专有资源待补。 |
| Geometry | 可生成的网格复用 BufferGeometry 的复制、克隆、公开属性往返及释放，逐类与 r160 同场景对照。 | 23 个网格生成门已实跑。Plane、Box、Circle、Cylinder 与 Cone 的 C6 门覆盖参数 JSON、网格复制克隆和释放目标。Box 另验六面材质组，Cylinder 与 Cone 对照端盖及开口。其余 18 类待补参数往返和释放。任意 userData 尚未写入，UUID 由调用方供给。Circle 保留半径为零与大于 3 的非整数分段拒绝。 |
| BufferAttribute | 11 类 JSON 写出经 r160 和独立金值对照，11 个篡改转红。 | 读回草稿复用几何属性解析器。字符串解码依赖须先声明 loader_utf8_width 和 uxe_copy_record。依赖补齐后，`cx ujs/tests/bufferattribute-family-struct.cx -- --round 1` 在 6 秒编译界内返回 124，单路复验同样超时。现由 `linked-wasm-flow.cx` 的 `attribute-json` 入口分别准备基座、检查内容哈希并链接热探针。11 种读回对照通过，错配类型逐项转红。完整对象复制、统一上传接线及超出单记录有界名字空间的输入另列缺口。编译器诊断复现为 `./ujs/ujs2wasm.com ujs/practice/uxe/test/native-undefined-local-repro.ujs -o /tmp/ujs-native-repro.wasm`，实测 rc=1，报 `ujs.com: native function requires an explicit local`，其中调用结果已经用 let 绑定。编译器任务是为未解析的 native 调用点报告符号和位置，并保留局部变量拒绝的原合同。 |
| Raycaster | 相机射线和场景网格递归由 raycaster-camera-io.cx 与 raycast-scene-io.cx 验收。场景篡改使子对象命中失败。 | 统一遍历中的混合对象递归分派与通用 intersectObject 和 intersectObjects 入口待补。 |
| BufferGeometry | geometry-json-load-struct.cx 经 C6 读写连续属性、索引、组和球界。修改解析字段能改变独立金值和 r160 对照结果。 | 参数化类专有字段、共享视图身份、任意元数据和变形属性的通用链路待补。 |

## 压缩纹理软件范围与设备缺口

BC1 到 BC5 及 ETC1 已有独立软件像素验收。ASTC 直接软件入口支持 14 种二维足迹的 LDR 常色块和合法有限范围，sRGB 保留编码字节。HDR、非常色块的分区、权重及端点模式仍缺。EAC 的有符号与无符号 R11、RG11 可通过 CompressedTexture_decodeEAC 解码，输出保留 11 位精度的交错 16 位小端样本。ETC2 可通过 CompressedTexture_decodeETC2 解码 RGB、全 Alpha 和单比特透明，包含 T、H 与平面模式。sRGB 保留编码字节。ETC2 与 ASTC 常色块已接通二维、数组及立方体的共用软件 mip 分派。EAC 精确 11 位输出使用独立入口，ETC2、EAC 与 ASTC 的压缩原样 GPU 上传待验。BC4 与 BC5 的有符号输出为二补码 SNORM。

真实设备对压缩源的原样上传、格式扩展、mip 完整性、数组分层、立方体六面采样及释放仍待取证。r160 对照在这些软件门中只用于枚举与纹理描述，像素期望由独立金值给出。硬件格式的 A/B 计量保持原口径。

## 几何与材质尚需的类专有验证

(1) EdgesGeometry 与 WireframeGeometry 的嵌套 PlaneGeometry 源已验，其他源类型和通用资源图仍待接。PolyhedronGeometry 的数组、半径和细分已验，专有复制与释放仍待补。

(2) ShapeGeometry 已验单 Shape 的直线外环和洞。ExtrudeGeometry 已验单轮廓、无倒角的轴向挤出，孔洞、倒角和路径挤出待补。TubeGeometry 已验 LineCurve3 的开闭管道，其他曲线及其标架待补。

(3) MeshPhysicalMaterial 的无贴图字段已验，贴图接线、GPU 透射、独立法线和间接光仍待补。ShaderMaterial 与 RawShaderMaterial 已验源码、GLSL 版本、标量和三维向量 uniform，其他 uniform、extensions 及运行时对象接线待补。
