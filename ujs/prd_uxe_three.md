# UXE 对标 three.js 全表与开发计划(目标 99%)

所有者 2026-11-19 指令:启动 D5 之前,UXE 底层功能、设计、能力要达到 three.js 的 99%。本文给出可度量的口径、全量对标树、架构决定、里程碑与逐项验收。D5 在 M12 通过前不开工;D4 迁移在 M7 完成后恢复。

## 1 口径:99% 怎么算

清单来源固定:仓内 `ujs/practice/web/game/three.module.js`(REVISION 160)末尾 `export {...}` 的 416 个名字,由 `.cx` 生成器解析成 TSV(`ujs/practice/uxe/three-inventory.tsv`),生成器带 `--check` 与磁盘篡改自测。不凭记忆列表。

分母分三栏,分别报告,不混算:

| 栏 | 内容 | 计法 |
|---|---|---|
| A 行为项 | 类与函数(Object3D、Matrix4、Mesh、Raycaster 等) | 每个名字一行,下含方法族,门对方法族逐个断言 |
| B 常量项 | 枚举与常量(格式、混合因子、比较函数、色调映射等,约 190 个) | 每个名字必须被 UXE API 接受或具名拒绝,由生成器表驱动并有测试 |
| C 附加件 | examples/jsm 中用得上的常用件(OrbitControls、GLTFLoader、EffectComposer 等,固定清单 30 项) | 单独列,不进 99% 分母,但里程碑里排期 |

修订(cdx2): A、B 合计 416 个名字已从跟踪的 r160 构建产物尾部复核。A 含行为命名空间，B 含枚举命名空间及版本常量。C 固定 30 个下游入口，依据 r160 的公开 Addons barrel。百分比只覆盖 A、B 的具体名字和承诺方法族，C 的排期不能代替其余附加件已覆盖的证据。方法族待展开时，该项不能记已有。各栏实际数量以生成 TSV 的统计为准。

修订(cdx2): S3 首算 A 共 215 项，扣 1，已有 0/214，部分 12。B 共 201 项，扣 1，已有 0/200。合计 0/414，三项比值均为 0.00%。C 另列 30 项。这是冻结基线，旧页面级实验没有给任何一项认定为已有的充分证据。后续若要记已有，须先给生成器增加引用并验证逐项门的规则。未展开的方法族、常量别名与附加件选择的局限在百分比解读中保留。

清单生成入口 `sh ./unisacc.com ujs/tools-ux/gen-three-inventory.cx -- --write`，比对入口将 `--write` 换为 `--check`。篡改入口 `sh ./unisacc.com ujs/tools-ux/check-three-inventory.cx`。生成器只读跟踪的 r160 构建产物、附加件事实表及现状注记，校验上游产物 SHA-256。本轮不需 git 忽略的 `ujs/ujs.com`。后续 Wasm 门通过 `ujs/product/build-ape.cx` 构建编译入口。

证据来源为标签 r160 的 [官方文档索引](https://github.com/mrdoob/three.js/blob/r160/docs/list.json) 与 [公开附加件入口](https://github.com/mrdoob/three.js/blob/r160/examples/jsm/Addons.js)。跟踪的 C 表记录每项原模块路径，参考 Addons.js 的 SHA-256 为 `985e6aad28494bf410a74fc3c5380015357b062b0572b3df8c62b7ae9e88b352`。参考代码只用于本地对照。

"已有"的定义(缺一不可):

(1) 该名字在 UXE 里有同名同语义的 .ujs 实现,经 `ujs.com` 编成 wasm 可被产品 wasm 调用;
(2) 有一个 `.cx` 独立算出的金值门,或与本地 three.js r160 在同一场景下的对照门(见 2.3);
(3) 门含篡改反例,篡改后 rc=1;
(4) 没有门的项一律记"缺",不得因为代码存在就记已有。

修订(cdx2): 第 (4) 条分为两种。存在可定位的 Wasm 原语及页面或探针证据，语义或门仍不完整，可记部分并计零分。没有代码证据或只有 JS 参考，记缺。B 栏的具名拒绝仅证明可诊断，适用项仍须在正常用法下实现其语义才算已有。已冻结的旧页基座版本和当前源须分别取证。

不适用项必须逐条写依据后才能从分母扣除。允许扣除的类别只有:WebGPU 坐标系常量与 WebGL1Renderer 兼容壳(平台绑定)、GPU 压缩纹理格式常量中本机无法验证的部分(只扣"硬件解码"一项,常量本身仍须被接受或具名拒绝)、DOM/WebXR 相关。扣除比例上限 4%,超出须所有者确认。99% 的判据:分母内"已有"项占比大于等于 99%,剩余未达项逐条写原因。

修订(cdx2): 首版仅明确扣除 A 的 `WebGL1Renderer`、B 的 `WebGPUCoordinateSystem`，原因各自写入清单。硬件压缩格式保留在 B，当前无设备证据计缺。分别报告 A、B 及 A+B 三个比值，99% 需 A 与 B 分别达标，防止常量数量遮住行为缺口。WebXR 与 DOM 渲染若是 C 或未选入 C，本来就不参与 A+B 分母。

## 2 对标树

### 2.1 现状图例

已有=满足第 1 节四条;部分=有实现但门不全或语义不全;缺=没有。现状取自 2026-11-19 `uxe.ujs`(684 行)与三个线上页面。当前严格口径下,A 栏已有项接近 0:多数只有页面级读回,没有按 three.js 名字组织的 API 与门。这是计划要解决的第一个问题,不是缺陷粉饰。

### 2.2 树(按里程碑归组,数量来自 r160 导出表的名字统计)

| 里程碑 | 子树 | r160 名字(数量) | 现状 |
|---|---|---|---|
| M1 数学 | Vector2、Vector3、Vector4、Matrix3、Matrix4、Quaternion、Euler、Color、ColorManagement、Box2、Box3、Sphere、Plane、Ray、Triangle、Frustum、Line3、Cylindrical、Spherical、SphericalHarmonics3、MathUtils、Interpolant 系(Linear/Cubic/Discrete/QuaternionLinear) | 约 37 加 5 | 部分:有散落的矩阵与向量运算,无同名 API |
| M2 核心图 | EventDispatcher、Object3D、Group、Scene、Layers、Clock、Cache、Source、Uniform、UniformsGroup、BufferAttribute 全家(Int8 到 Float64、Uint8Clamped、Float16、Instanced、Interleaved、GLBufferAttribute 共 14)、BufferGeometry、InstancedBufferGeometry、DataUtils、ImageUtils | 约 32 | 缺:产品自己摆场景,无父子变换 |
| M3 相机与几何 | Camera、PerspectiveCamera、OrthographicCamera、ArrayCamera、CubeCamera、StereoCamera;Geometry 23 个(Box、Capsule、Circle、Cone、Cylinder、Dodecahedron、Edges、Extrude、Icosahedron、Lathe、Octahedron、Plane、Polyhedron、Ring、Shape、Sphere、Tetrahedron、Torus、TorusKnot、Tube、Wireframe 等);Curve 11、Path、Shape、ShapePath、ShapeUtils | 6 加 23 加 约 15 | 部分:透视投影与相机空间变换已在 `uxe.ujs`;无通用几何生成 |
| M4 GPU 渲染基座 | WebGLRenderer 对应面:渲染循环、渲染列表(不透明/透明排序)、视锥剔除、清屏、视口、裁剪、状态机(混合、深度、模板、剔除面)、RenderTarget 5 种、WebGLUtils、ShaderChunk、ShaderLib、UniformsLib、UniformsUtils、GLSL1/GLSL3 | 约 12 | 缺:现为软件光栅化,无 GPU 命令包 |
| M5 材质与纹理 | Material 18(Basic、Lambert、Phong、Standard、Physical、Toon、Matcap、Normal、Depth、Distance、Shadow、Points、Line*、Sprite、Shader、RawShader);Texture 12(Canvas、Compressed*、Cube、Data、Data3D、DataArray、Depth、Framebuffer、Video 等);颜色空间与色调映射 7;Mapping 13、Filter 10、Wrapping 3 | 约 40 | 部分:toon 三阶、描边、雾、阴影图在软件路径;无 PBR |
| M6 灯光与阴影 | Light、AmbientLight、HemisphereLight、DirectionalLight、PointLight、SpotLight、RectAreaLight、LightProbe;ShadowMap 4 种(Basic、PCF、PCFSoft、VSM);Fog、FogExp2;PMREMGenerator | 约 17 | 部分:方向光、阴影图、深度雾 |
| M7 网格族 | Mesh、InstancedMesh、BatchedMesh、LOD、Sprite、Points、Line、LineSegments、LineLoop、Bone、Skeleton、SkinnedMesh、AttachedBindMode、DetachedBindMode | 约 14 | 缺(粒子印章与精灵 blit 仅是像素级) |
| M8 动画 | KeyframeTrack 7、AnimationClip、AnimationAction、AnimationMixer、AnimationObjectGroup、AnimationUtils、PropertyBinding、PropertyMixer、循环与混合模式常量 | 约 20 | 缺:D3/D4 过程动画在产品里手写 |
| M9 加载器 | LoadingManager、DefaultLoadingManager、Loader、LoaderUtils、FileLoader、ImageLoader、ImageBitmapLoader、TextureLoader、DataTextureLoader、CompressedTextureLoader、CubeTextureLoader、AudioLoader、AnimationLoader、BufferGeometryLoader、MaterialLoader、ObjectLoader;C 栏 GLTFLoader | 13 加 3 | 缺:宿主能取字节,基座不解码 |
| M10 拾取与控制 | Raycaster(含 Mesh、Instanced、Skinned、Points、Line、Sprite、LOD 的 raycast)、Ray 求交;C 栏 OrbitControls、MapControls、PointerLockControls、TransformControls;输入常量 MOUSE、TOUCH | 约 4 加 4 | 缺 |
| M11 后处理、辅助、音频 | Helper 13(Arrow、Axes、Box、Box3、Camera、DirectionalLight、Grid、HemisphereLight、Plane、PointLight、PolarGrid、Skeleton、SpotLight);Audio 6(Audio、AudioAnalyser、AudioContext、AudioListener、AudioLoader、PositionalAudio);C 栏 EffectComposer、RenderPass、UnrealBloomPass、OutlinePass 等 | 约 19 | 部分:D1 有 PCM 事件与合成 |
| M12 软件路径整合 | 保留像素风软件路径(D1 D2 在用),接到同一场景图与材质 API;D1 到 D4 迁到新基座;删除重复实现 | - | 进行中(D1 D2 D3 已是三层) |

B 栏常量按所属里程碑归组(格式 44 个归 M5,混合因子 15 与方程归 M4,深度与模板 27 个归 M4,比较函数 8 个归 M4,色调映射与颜色空间 18 个归 M5,Usage 9 个归 M2,类型 11 个归 M2,其余归所属类)。

修订(cdx2): 上树的约数及 B 栏组数供排期，不作核分。M4 软件路径已具深度测试，历史 UXEP 的 `ujs-gpu-tri` 也有单三角提交。P11 至 P13 的双 Wasm 桥没有 `host_gpu_submit`，因此当前对标 GPU 渲染器记缺。D3 方向光参数驱动桥的 96x96 高度阴影图，并用世界相机、透视 UV 和线性雾，现状证据见 [0.3c](prd.md#03c-uxe-三层合同与迁移-s1)。

### 2.3 验收门类型

| 类型 | 用于 | 做法 |
|---|---|---|
| G-math | 数学类、插值、曲线、几何顶点 | `.cx` 用高精度独立算金值;另在本地 three.js 页面用同输入导出 JSON,对比误差上界(f64 小于 1e-9,f32 小于 1e-5),篡改输入转红 |
| G-struct | 场景图、BufferAttribute、Layers、事件 | 构造固定树,逐节点读回世界矩阵、可见性、排序 |
| G-pixel | 材质、光照、阴影、雾、纹理、后处理 | 与本地 three.js r160 同场景同机位像素对照,GPU 路径平均绝对差(8 位 RGB)不超过 2%,含阴影与 PBR 的场景不超过 4%;软件路径不做对照,走自己的区域门 |
| G-anim | 动画 | 固定时刻采样顶点,对 three.js 同时刻读回,误差 1e-4 以内 |
| G-io | 加载器 | 好文件逐字段读回,坏文件逐种拒绝并释放,不泄漏 |
| G-ray | 拾取 | 固定射线对固定场景,命中个数、距离、面索引、UV 与 three.js 一致 |
| G-ref | 参考页 | `ujs/practice/uxe/ref-three/` 仅本地用,不进 docs |

每个门必须含篡改反例;检查由 `.cx` 驱动,页面只回报读回。

## 3 架构决定

(1) 双路径。像素风软件路径保留(D1 D2 的观感与性能依赖它)。新增 GPU 路径作为对标 three.js 的主路径:基座 wasm 在自己的线性内存里维护场景对象与资源表,渲染时产出命令包,页面桥 `host_gpu_submit` 把包转成 WebGL 调用。着色器源、管线与状态由基座决定,不在页面 JS 里。两路径共用同一场景图、几何、材质描述。

(2) 对象模型。`.ujs` 没有 class 的地方,用"句柄加布局表"表达:每个类一张字段偏移表,继承按布局前缀;三.js 的方法映射为 `Class_method(self, ...)`;属性按 getter/setter 对映射;句柄为 i32,带代数,销毁后访问具名拒绝。命名规则:`Object3D.add` -> `Object3D_add`,`Matrix4.multiply` -> `Matrix4_multiply`。映射表由生成器从清单与方法族表生成,写进 TSV。

(3) 产品调用。产品 wasm 通过 0c 的模块链接导入基座 API,不再有产品自己的光栅化。每页 host-abi 桥白名单行数门保持(现 300 暂定)。

修订(cdx2): 现有 0c 编译器单模块只声明一个带版本原生导出，至多八个声明式导入。草案先以一个带版本的调度入口链接，命名映射是产品侧接口；M0 C6 通过后再拆直接导出。跨实例指针禁止直传，资源块须通过具长度的复制或明定共享内存协议。带代数句柄的底层载体可按 0c 先用 i64 ABI 承载 32 位编码。渲染器读取场景树，几何与材质各自有资源句柄，可共享且独立释放。

| three.js 接口 | UXE 草案 | 对象合同 |
|---|---|---|
| Scene、Object3D、Group | `Scene_new`、`Object3D_add`、`Object3D_remove`、`Object3D_updateMatrixWorld` | 单父节点、父子世界变换、循环拒绝、失效句柄拒绝 |
| Camera、PerspectiveCamera | `Camera_lookAt`、`PerspectiveCamera_updateProjectionMatrix` | 相机是节点，视图矩阵与投影独立 |
| BufferGeometry、Material、Mesh | `BufferGeometry_setAttribute`、`Material_setParameter`、`Mesh_new` | Mesh 引用两个独立资源，材质参数改变无需重建顶点 |
| Light、Renderer | `DirectionalLight_new`、`Renderer_render`、`Renderer_setSize` | 同场景描述提交给软件或 WebGL 后端 |
| Loader、AnimationMixer | `Loader_parse`、`AnimationMixer_clipAction`、`AnimationMixer_update` | 解码和时间推进在 Wasm，宿主提供字节与时钟 |
| Raycaster、Clock | `Raycaster_setFromCamera`、`Raycaster_intersectObjects`、`Clock_getDelta` | 射线命中稳定排序，时钟可注入固定步验证 |

```js
// Draft for the current single-export linker.
import uxe_scene from "uxe_s3_v1"(i64, i64, i64) i64;
export uxe_frame(now:i64) i64 version 1;
// scene, camera and resources are validated generational handles.
```

修订(cdx2): GPU 命令包须固定版本、字节序、总长、资源代数、命令数、每条参数范围和错误返回。宿主 WebGL 桥验证后转发，禁止合成材质或自行排序。基座负责绘制队列、着色器、帧图、资源重建及能力下降策略。M4 必须实测坏包拒绝、回收和上下文丢失后重建。软件后端继续负责 RGBA8、深度和独立像素门；D3 已是 960x540，场景深度配置的上限为 524288 像素。

(4) 编译器是前置。M0 先补 `ujs.com` 缺的能力,单列于第 4 节。

(5) 性能预算。GPU 路径下 1280x720 场景 1 万三角形、200 实例、4 盏灯、一张阴影图,CDP 真时钟平均每帧小于 16 毫秒;软件路径 426x240 下 D1 同类场景每帧小于 33 毫秒。

## 4 M0:编译器前置(ujs.com)

| 编号 | 能力 | 验收 |
|---|---|---|
| C1 | `Float32Array`、`Uint16Array`、`Uint32Array`、`Int32Array`、`Float64Array` 视图与 `DataView` 读写 | 越界与对齐具名拒绝,`.cx` 金值 |
| C2 | f64 数学:sin、cos、tan、asin、acos、atan、atan2、pow、exp、log、sqrt 已有、cbrt、hypot | 与高精度金值误差上界,极值与 NaN 行为明确 |
| C3 | 数组与字典的元素为结构记录(字段偏移),或语法糖 `struct` 布局声明 | 嵌套读写一致,篡改偏移转红 |
| C4 | 闭包:逃逸、可变捕获 | 语料加反例 |
| C5 | 字符串:UTF-8、长度大于 255、拼接与切片、数字格式化 | 与 `.cx` 金值 |
| C6 | 多模块与符号可见性、跨模块常量与导出表的版本检查 | 0c 之上扩展 |
| C7 | 运算 u32、i32 位移与移位已有部分补齐 | 语料 |
| C8 | 性能:内联与循环展开基线,`ujs.com` 对 1e7 次浮点循环的耗时基线 | 记基线与目标,不得退化 |

修订(cdx2): 现有基础为 `f64_sqrt`、`f64_floor_i64`、有界 `ux_span`、u8/f32 连续视图、独立线性内存和声明式链接。不支持任意 JavaScript 或浏览器 `Float32Array` 语法。字典键最多 8 字节，ASCII 字符串字面量最多 255 字节，UTF-8 目前走有界字节而非通用字符串。闭包只证实了短例外层读取，无逃逸或可变捕获合同。WebGL 端大批顶点与纹理上传须先量测跨度搬运和桥接次数，避免逐顶点宿主调用。

## 5 开发计划(给 cdx2 的逐里程碑任务)

修订(cdx2): C1 的 `.ujs` 接口采用显式缓冲视图。对 Float32Array、Uint16Array、Uint32Array、Int32Array、Float64Array，先用 `ux_typed_index(base, count, index, width)` 得到字节偏移，再用 `ux_f32`、`ux_u16`、`ux_u32`、`ux_i32`、`ux_f64` 及其 `_put` 写入。DataView 切片通过 `ux_data_offset(base, bytes, offset, width)` 支持非对齐访问，目前接口使用小端字节序。负返回值为 `UX_VIEW_BOUNDS=-1` 或 `UX_VIEW_ALIGNMENT=-2`，调用者须先检查再读写。整数写入截为目标位宽，f32 写入舍入到单精度。入口仍由 `ujs2wasm.com` 编译，现有 u8、f32 与 span 指令序列保留。验收运行 `sh ./unisacc.com ujs/product/cli-host.cx -- --ux-views` 及 `--mutate-ux-views`。

修订(cdx2): C2 的 `f64-math.ujs` 由编译器随消费者一起编成 Wasm。三角函数的角度按弧度计，`sin/cos/tan` 对绝对值大于 1048576 的输入返回 NaN。正切在极点附近的误差受余弦分母影响。`hypot` 接收两个坐标。域外反三角函数、非正对数、指数上下溢和带符号零分别按独立金值检查。测试覆盖已列采样点，超大角度约简、全域正确舍入和可变参数 `hypot` 仍待补齐。

修订(cdx2): C3 使用 `struct Position { x: f64; y: f32; }` 形式声明缓冲布局。引用其他记录时类型须先声明。字段按自然对齐，`Position.size` 含尾部填充。`Mesh.origin.x` 给出嵌入字段的字节偏移，`Mesh.origin.size` 给出嵌入类型自身的大小。使用 `ux_record_index(base, count, index, Mesh.size, Mesh.align)` 检查记录数组，再经 C1 标量接口读写。字典可保存记录偏移，缓冲的生命周期仍由调用者负责。

修订(cdx2): C4 以 `closure(callback, initialValue, ...)` 显式建立可变捕获环境。回调的首个参数接收环境列表。闭包可作为函数返回值、局部变量或第一个函数参数，同一闭包的别名共享捕获，不同构造得到独立环境。环境分配在当前 Wasm 调用的受限堆内。当前最多 16 个 i64 捕获，回调类型在编译时确定。自动捕获外层词法变量、异构回调集合和跨宿主 ABI 保存闭包仍缺。

修订(cdx2): C5 接受最长 1024 字节的有效 UTF-8 字面量，运行时保留 UTF-8 拼接。`len(s)` 返回字节数。`str_slice(s, start, end)` 使用半开字节区间，越界或切入 UTF-8 字符内部会拒绝。`ux_format_i64` 输出十进制并覆盖 i64 两端。NUL 和反斜线转义沿用现有拒绝，浮点精度格式与 UTF-16 索引仍缺。

修订(cdx2): C6 保留每模块一个业务入口体，允许声明至多八个原生 Wasm 导出别名，例如 `alias uxe_double(1);` 将首个 i64 参数绑定为操作号，并保留其余参数和返回值的类型。将所有参数绑定后可提供零参数常量读取。`uxe_math_v2` 等独立命名空间按依赖顺序实例化，桥核对版本、导出名、签名和独立内存。当前别名仅绑定前缀 i64 字面量，不支持同一模块内任意多个独立导出体。

修订(cdx2): C7 为 `u32_` 和 `i32_` 两组提供 `wrap/add/sub/mul/and/or/xor/shl/shr/rotl/rotr`。输入先截低 32 位，操作数与移位量都按 Wasm 规则处理，结果以零扩展或符号扩展读回 i64。`i32_shr` 为算术右移，`u32_shr` 为逻辑右移。

修订(cdx2): C8 用 `sh ./unisacc.com ujs/tests/f64-bench.cx -- baseline 20000` 与 `-- unroll 12000` 检查本机编译与 host1 执行。两个输入都内联同一个加法函数并执行 10000000 次 f64 加法，`.cx` 金值为 1250001。三次实测普通循环中位数 15387 ms，手工四路展开中位数 8306 ms，生成物分别为 2191 和 2282 字节。上限单位为毫秒，供同机回归。该基线包含 host1 启动开销，未测浏览器吞吐，也未引入自动展开优化。`-- mutate` 把循环变为一次，原金值不变。

修订(cdx2): M1 Vector2 在基座缓冲区使用两个连续 f64，调用者为 clone 等操作提供目标偏移。三角函数沿用 C2 角度域，BufferAttribute 适配连续 f32 记录，随机数由调用者注入，宽高和遍历通过分量接口读取。用 `sh ./unisacc.com ujs/tests/vector2-math.cx` 复跑 55 项 r160 JSON 和独立金值对照，另检查越界数组索引。`--mutate` 用减法替换加法检验断言。此门和 M0 的编译器检查使用 `ujs/ujs2wasm.com`，须先用 `sh ./unisacc.com ujs/product/build-ape.cx` 构造未跟踪的 `ujs/ujs.com`。清单生成和 `--check` 只读受跟踪的源码与证据入口。

规则不变:小步提交,每步单独提交并贴输出,每个门含篡改红,既有门与 UXE 回归全绿后才提交,不放宽断言,不向 docs 发布(发布由我另请 cc 合入)。每个里程碑结束产出:对标表该里程碑行状态更新、门清单、可演示页(放 ujs/ 内测试页,不进 docs)。

| 阶段 | 任务 | 预计提交 | 出口 |
|---|---|---|---|
| S3 | 生成器 `ujs/tools-ux/gen-three-inventory.cx` 解析 `three.module.js` 导出表为 TSV,`--check` 与篡改自测;TSV 含 A、B、C 栏、里程碑列、现状列(默认缺)、门编号列;对标树渲染成 `ujs/prd_uxe_three.md` 附表;现状按严格口径重判 | 3 到 4 | 清单与基线百分比,A 栏已有项数、B 栏已有项数 |
| M0 | C1 到 C8,每项一提交 | 8 到 12 | 编译器门全绿,基线记录 |
| M1 | 数学类:顺序 Vector2/3/4、Matrix3/4、Quaternion、Euler、Color、Box、Sphere、Plane、Ray、Triangle、Frustum、Line3、Cylindrical、Spherical、SH3、MathUtils;每类一提交含 G-math 门与本地 three.js 参考 JSON | 18 到 24 | 数学 A 栏全部已有 |
| M2 | 对象模型与布局表生成器;EventDispatcher、Layers、Object3D(add、remove、attach、世界矩阵缓存、遍历、getWorldPosition 等方法族)、Group、Scene、Clock;BufferAttribute 14 种、BufferGeometry、InstancedBufferGeometry、InterleavedBuffer;B 栏 Usage 与 Type 常量 | 14 到 18 | G-struct 门,D1 的场景改用 Scene 与 Object3D |
| M3 | 相机四种与投影矩阵、视锥;23 个几何生成器各一提交,顶点与索引与 three.js 逐项对;Curve 系、Path、Shape、ShapeUtils(三角剖分) | 20 到 26 | G-math 对几何的顶点哈希 |
| M4 | GPU 命令包格式(写进 HOST_ABI 与 prd),桥 `host_gpu_submit` 的白名单扩展;渲染列表与排序;视锥剔除;状态机;RenderTarget;B 栏混合、深度、模板、比较、剔除常量 | 12 到 16 | 一个三角形、一个立方体、深度测试、混合像素对照 three.js |
| M5 | 着色器库(ShaderChunk 对标)与 Basic、Lambert、Phong、Standard、Physical、Toon、Matcap、Normal、Depth、Distance、Points、Line、Sprite、ShadowMaterial、ShaderMaterial、RawShaderMaterial;Texture 与 Mapping、Filter、Wrapping、颜色空间、色调映射 7 种 | 28 到 36 | G-pixel 门逐材质,PBR 对照 4% |
| M6 | 灯光 7 种;阴影四种;Fog 两种;PMREMGenerator 与环境贴图 | 16 到 20 | G-pixel 门含阴影 |
| M7 | Mesh、InstancedMesh、BatchedMesh、LOD、Sprite、Points、Line 三种、Bone、Skeleton、SkinnedMesh | 14 到 18 | 此后恢复 D4 迁移 |
| M8 | KeyframeTrack 7、Interpolant 系、AnimationClip、Action、Mixer、PropertyBinding、混合与循环模式 | 14 到 18 | G-anim 门,D4 四足步态改用 Mixer |
| M9 | LoadingManager 到 ObjectLoader 全系;PNG 与 JPEG 解码在 wasm;GLTFLoader(C 栏,含蒙皮与动画) | 18 到 24 | G-io 门含坏文件 |
| M10 | Raycaster 对全部网格族;OrbitControls、MapControls、PointerLockControls、TransformControls | 10 到 14 | G-ray 门 |
| M11 | 后处理(EffectComposer、RenderPass、Bloom、Outline、FXAA 等 C 栏)、Helper 13 种、Audio 6 种 | 16 到 22 | G-pixel 与 PCM 门 |
| M12 | 软件路径接入同一场景图;D1 到 D4 基于新基座重整;删除页面产品重复的光栅化;对标表最终判定与未达项原因表;99% 复核由独立 `.cx` 重算 | 8 到 12 | 所有者验收,D5 放行 |

合计约 200 到 260 个提交,按每天 12 到 20 个提交估,13 到 20 个工作日量级。此估计偏乐观,每个里程碑末复估一次。

修订(cdx2): D4 从 M7 恢复时先接场景树与城镇实例批，再接四腿刚体层级和步态，随后接跟拍相机及烟发射生命周期。已有本地 `town-motion` 的行走与跟拍位姿金值，线上还未迁移。M7 先验两节刚体 FK/IK、实例变换和遮挡，M8 再验混合器。烟的发射、排序与回收是另一个门，M7 若只交像素印章，保留在部分。M9 的压缩编解码器和第三方格式逐项估算。未纳入当前清单的插件、扩展及设备能力不能用提交数估计当作已覆盖。

## 6 风险与诚实声明

(1) 99% 不是对 three.js 全部运行时行为的保证,而是对清单与门的覆盖;任何未经门的项不算。
(2) `ujs.com` 的性能与表达力是最大风险。M0 的 C3、C4、C8 若无法达到,GPU 路径的基座规模会受限,需要回头改编译器而不是绕开。
(3) three.js 的 WebGL 着色器体量很大。ShaderChunk 逐块对标,不整体搬运;PBR 的 IBL 部分依赖 PMREM,排在 M6。
(4) 软件路径与 three.js 的像素对照不做,因为风格不同;它的目标是 D1 到 D4 观感不退。
(5) 许可:三.js 为 MIT,仅本地对照与清单来源,不拷贝其源码进 UXE 实现。实现一律独立编写。
