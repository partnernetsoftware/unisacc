# UXE 家族收口口径

已有要求可调用的同名实现、独立金值或 r160 同场景对照，以及能扰动结果的反例。r160 类提供 toJSON 时还须验证写出和读回。复制、释放与通用渲染接线的剩余限制列在右栏。共用实现只在实际经过同一入口和布局时复用证据。计量分母照旧。

| 家族 | 公共合同和证据 | 已知缺口 |
| --- | --- | --- |
| Material | 生成构造进入同一个 Material 布局。公共字段表生成赋值及 JSON 对象字段往返，copy、clone 和 dispose 使用公共入口。两项 r160 门分别覆盖 JSON 和生命周期。 | JSON 根身份、纹理及图像资源、各派生专有字段和参数接到绘制尚欠。 |
| Light | 七类构造及公共记录，按类读写颜色、强度、半影、地面色、尺寸、SH 和适用阴影参数，共用 copy、clone 和释放入口。`light-class-public-linked.cx` 以 C6 对象和编解码基座验七类从实际实例写出字节并重建，覆盖目标隔离、释放和句柄失效。 | Object3D 的变换、子树、身份、用户数据和阴影相机的其余属性尚未往返。灯光对象到 GPU 光源仍须接线，矩形灯镜面 LTC 待补。 |
| Helper | 实例绑定其源对象或几何资源并复制、克隆和释放，适用公共对象字段需能往返。 | 已验六类复制与克隆及 ArrowHelper 释放。Axes、Box、Box3 共用几何的语义由 helper-family-shared-geometry.cx 对照。ArrowHelper 另证根四元数和轴线、锥体的姿态 JSON 往返，根位置、可见性、图层及资源身份待补。Grid 和 PolarGrid 的字段往返与渲染资源释放链尚欠，其余七类仍有复制或对象树布局缺口。 |
| Loader | r160 未定义复制、克隆与 JSON 往返方法。验收通过真实异步请求验证配置、对应解码、失败和回收。 | Audio 的 WAV、Material 的基础 JSON 和 Image 的 PNG 可异步加载。JPEG 已有独立解码入口，异步分派、其他音频容器和材质专有资源待补。 |
| Geometry | 可生成的网格复用 BufferGeometry 的复制、克隆、公开属性往返及释放，逐类与 r160 同场景对照。 | 23 个网格生成门已实跑。Plane、Box、Circle、Cylinder 与 Cone 的 C6 门覆盖参数 JSON、网格复制克隆和释放目标。Box 另验六面材质组，Cylinder 与 Cone 对照端盖及开口。其余 18 类待补参数往返和释放。任意 userData 尚未写入，UUID 由调用方供给。Circle 保留半径为零与大于 3 的非整数分段拒绝。 |
| BufferAttribute | 11 类 JSON 写出经 r160 和独立金值对照，11 个篡改转红。 | 读回草稿复用几何属性解析器。字符串解码依赖须先声明 loader_utf8_width 和 uxe_copy_record。依赖补齐后，`cx ujs/tests/bufferattribute-family-struct.cx -- --round 1` 在 6 秒编译界内返回 124，单路复验同样超时。待拆开基座准备与热探针。 |
| Raycaster | 相机射线和场景网格递归由 raycaster-camera-io.cx 与 raycast-scene-io.cx 验收。场景篡改使子对象命中失败。 | 统一遍历中的混合对象递归分派与通用 intersectObject 和 intersectObjects 入口待补。 |
| BufferGeometry | geometry-json-load-struct.cx 经 C6 读写连续属性、索引、组和球界。修改解析字段能改变独立金值和 r160 对照结果。 | 参数化类专有字段、共享视图身份、任意元数据和变形属性的通用链路待补。 |
