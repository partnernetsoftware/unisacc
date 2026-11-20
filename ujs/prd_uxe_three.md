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
| M1 数学 | Vector2、Vector3、Vector4、Matrix3、Matrix4、Quaternion、Euler、Color、ColorManagement、Box2、Box3、Sphere、Plane、Ray、Triangle、Frustum、Line3、Cylindrical、Spherical、SphericalHarmonics3、MathUtils | 21 | 修订(cdx2):21 项已有且有 G-math 门。Interpolant 系按开发计划归 M8 |
| M2 核心图 | EventDispatcher、Object3D、Group、Scene、Layers、Clock、Cache、Source、Uniform、UniformsGroup、BufferAttribute 全家(Int8 到 Float64、Uint8Clamped、Float16、Instanced、Interleaved、GLBufferAttribute 共 14)、BufferGeometry、InstancedBufferGeometry、DataUtils、ImageUtils | 约 32 | 部分。基座已有父子变换，产品接入和各类剩余方法以清单为准 |
| M3 相机与几何 | Camera、PerspectiveCamera、OrthographicCamera、ArrayCamera、CubeCamera、StereoCamera，21 个 r160 核心几何类及 ConvexGeometry、TextGeometry 两个扩展类，Curve 与 11 个派生曲线类，Path、Shape、ShapePath、ShapeUtils，另含扩展 BufferGeometryUtils | A 栏 43 项，C 栏 3 项 | 基础实现经独立金值及 r160 对照，具体缺口见下文和生成清单 |
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

(3) 产品调用。产品 wasm 通过 0c 的模块链接导入基座 API,不再有产品自己的光栅化。软件桥 300 行,GPU 桥 600 行,两者均须通过独立计数与白名单结构门。详见 [页面 JS 白名单](prd.md)。

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

修订(cdx2): M4 使用 [UXEP 第 5 版合同](practice/uxe/HOST_ABI.md#m4-的-uxep-第-5-版)。24 字节小端包头声明版本、总长、资源代数和命令数。基座发出视口、清色、状态、三角形与清深度命令，宿主整包校验后转发。首批验收用同机位和同输入对照本地 r160 的三角形、立方体、深度和混合像素。首版内联命令包上限为 64 KiB，规模门依赖后续资源引用命令。

修订(cdx2): C15 仅允许在 `uxe_gpu_v1` 显式声明有状态返回值的 `host_gpu_submit(i64,i64) i64`，旧式 `env.host_gpu_submit` 仍由原内建入口生成。`gpu-import-abi.cx` 校验导入、拒绝非法模块及签名。`gpu-packet-struct.cx` 在单个真实 Chrome 中测四个场景、九个拒绝结果及拒绝后的视口和像素，共 198 个读回值。着色器文本由基座资源进入命令包。M4 渲染列表、剔除、持久资源和上下文重建仍待实现与验证。

修订(cdx2): M4 后续门覆盖不透明、透射及透明队列的稳定排序、世界包围球剔除、混合、深度、模板、剔除面、偏移、颜色掩码及裁剪下的三缓冲清屏。`gpu-state-constants-struct.cx` 与 r160 对照 64 个接受或具名拒绝项。`gpu-packet-struct.cx -- --targets` 验证五类 WebGL 目标、读回、代数换代和上下文丢失拒绝。真实浏览器恢复后重建缺口移至 M12。目标桥的导出行数仍需验收。

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
| C9 | 显式非内联函数与独立 Wasm 局部槽 | 256/257 旧规格保持，跨函数独立槽，递归与深度拒绝，篡改转红 |
| C10 | 多个 UJS 源文件编译成一个 Wasm 模块 | 跨文件调用、重名与缺源具名拒绝，连接边界和篡改对照 |
| C11 | 混合整数与浮点表达式及局部槽类型校验 | 二元算术与比较用 wasm-validate 和金值复核，浮点取余与异型重声明具名拒绝 |
| C12 | 内联局部绑定隔离 | 同型、异型、嵌套与循环验算，篡改转红，D1 至 D3 冻结 Wasm 重编字节相容 |
| C13 | 单一基座内多个原生函数 | 标准网格 JSON 读写同编译模块。接受 128 个函数，拒绝 129 个。host1 与 Wasm 格式门读回，保留每函数 256 槽及调用深度上限 |
| C14a | 编译容量诊断与发码分配区 | 具名报告发码区、原生函数、局部槽、字符串池、内联符号的用量与上限。主机发码区逐级扩至 8 MiB，多一字节具名拒绝 |
| C14b | 原生函数可达性剔除 | 同一入口及 ABI 导出只发出直接或间接调用的原生函数，保留冻结构造的原生函数排列 |

修订(cdx2): 现有基础为 `f64_sqrt`、`f64_floor_i64`、有界 `ux_span`、u8/f32 连续视图、独立线性内存和声明式链接。不支持任意 JavaScript 或浏览器 `Float32Array` 语法。字典键最多 8 字节，ASCII 字符串字面量最多 255 字节，UTF-8 目前走有界字节而非通用字符串。闭包只证实了短例外层读取，无逃逸或可变捕获合同。WebGL 端大批顶点与纹理上传须先量测跨度搬运和桥接次数，避免逐顶点宿主调用。

## 5 开发计划(给 cdx2 的逐里程碑任务)

GPU 主门通过 `sh ./unisacc.com tools/devsvc.cx -- x --par 1 sh ./unisacc.com ujs/tests/gpu-packet-struct.cx` 运行。旧合并输入在编译阶段 10203 毫秒退出 124。最小 GPU 包基座探针编译为 678 毫秒，Chrome 新配置打开空页为 504 毫秒。拆分后冷编译的管线与图元、材质与纹理、逐片元光照三个输入分别为 4025、6650、3722 毫秒，Chrome 含像素读回为 5682 毫秒。单片编译仍限 10 秒，Chrome 仍限 20 秒。三个测试模块各自持有 Wasm 线性内存，沿原顺序提交至同一 WebGL 上下文，汇总仍检查 43 个场景的并集和原有 1682 个返回值。用 `-- --scene-omit-mutate` 验证缺少场景 111 时汇总失败。产品基座的单模块合同沿用本节下文。

重复运行可让 devsvc 的子命令通过 `env UXE_GPU_WASM_CACHE=/tmp/自己的已创建目录` 传入私有缓存目录。缓存键包含编译器和入口、声明、着色器生成物及所有源文件的有序内容哈希。命中仍校验载荷 SHA-256，并重新运行 Chrome 与全部断言。显式 `-- --check` 重编三个模块，对比各自缓存包哈希后执行同一门。截断记录会在进入 Chrome 前具名失败。GPU 探针的原生调用共用线性内存，当前的版本化模块 ABI 不支持跨实例传递这些地址，因此缓存三份编译产物，各模块内的场景复用同一实例。

修订(cdx2): 从仓根运行 `sh ./unisacc.com ujs/product/build-ape.cx` 构造未跟踪的 `ujs/ujs.com`。C9 本机五切片构造实测约 39 秒。`ujs/ujs2wasm.com` 用它编译 `.ujs`。CLI/host1、G-math、Wasm 链接、C8 基准及需要重编产品源的验证依赖这个构造。清单生成与 `--check` 只读已跟踪的对标输入，检查冻结发布包的哈希也可独立执行。

修订(cdx2): C9 使用 `native function name(x:i64, y:f64) f64 { ... }` 生成独立 Wasm 函数和 `call`。stage0 原 `MAXL=256` 及 `invocation_slot` 把传统内联调用的临时量计入调用方。host1 的 `h1_exec` 逐帧检查局部声明，`h1_call` 为每帧保留 256 槽。C9 保持每帧 256 槽和原 257 槽拒绝；原有 `function` 仍内联。原生函数参数和返回值使用 i64/f64，最多 8 个参数和 8 层原生调用，只允许调用已声明的原生函数。递归、前向引用和深度超过上限在编译时具名拒绝，host1 操作数栈溢出也具名拒绝。`cli-host.cx -- --native-calls` 检查两份 256 槽帧、独立 Wasm 函数数目及拒绝，`--mutate-native-calls` 验证金值保持。

修订(ccapi): 数学、场景、几何、材质各由独立 UJS 源文件维护，按顺序编译为单一 base.wasm，共用基座线性内存。产品 Wasm 仍为第二个模块，通过 0c 的版本化入口传递标量和句柄。C10 的命令为 `ujs/ujs2wasm.com abi.ujs math.ujs scene.ujs geometry.ujs material.ujs -o base.wasm`，ABI 声明放最前，符号按编译单元的声明顺序可见，重复函数与布局具名拒绝。该入口目前最多接受 64 个源文件，总源文件合计最多 100000 字节。D1 私有场景探针实测 base.wasm 为 23694 字节。探针超过 4 MiB 时输出 `D1_SCENE_BASE_BUDGET_EXCEEDED` 并失败。该体积是探针基线，后续基座增加功能须重测。

修订(cdx2): C12 默认给内联局部绑定独立的活动作用域。用完的槽只在类型匹配时复用。查找仍支持已在活动父作用域的值，同一作用域的异型重声明仍具名拒绝。默认模式试编 D1 和 D3 的旧产品时槽索引发生变化，因此既有 D1 投影、D3 漫游包的构造明确指定 `--legacy-inline` 以重现冻结字节。新基座和新产品使用默认作用域规则。D2 继续使用默认模式，前后字节一致。

修订(cdx2): M3b 的网格 JSON 读写分别能编译，合在一个基座时共声明 19 个原生函数，触发旧 16 个函数上限。C13 把阶段零的原生函数容量设为 128，把 host1 的定义函数与签名容量分别设为 192 与 160，并检验定义函数引用的签名索引。C9 每函数局部槽 256 与原生调用深度 8 保持。网格读写合编探针为 27742 字节。检查入口为 `sh ./unisacc.com ujs/tests/native-capacity.cx`，依赖本节构造的 `ujs/ujs.com` 和 `wasm-validate`。

修订(cdx2): C14a 确认 PlaneGeometry 与 ExtrudeGeometry 的合编实际耗尽 256 KiB 发码区。主机从 256 KiB 起按需扩大实际分配，最多 8 MiB，现有分配用偏移在扩容后重取地址。独立 Wasm 编译器保留稳定的线性内存地址，并按同一规则限制分配。8 MiB 是编译暂存上限，基座产物另守 4 MiB 预算。`compiler-capacity.cx` 检查具名拒绝和原超限样本，`compiler-arena.cx` 检查扩容数据与上限加一。

修订(cdx2): C14b 先验证所有原生定义，再从同一程序与 ABI 入口沿原生调用边计算可达集合，重排类型、函数表与调用索引。`--legacy-inline` 同时保留冻结构造的原生函数排列。检查入口 `native-reachability.cx` 覆盖死定义加入前后字节一致、传递调用与 ABI 导出读回。内联表达式和宿主固定导出仍按既有编译规则处理。

修订(cdx2): C1 的 `.ujs` 接口采用显式缓冲视图。对 Float32Array、Uint16Array、Uint32Array、Int32Array、Float64Array，先用 `ux_typed_index(base, count, index, width)` 得到字节偏移，再用 `ux_f32`、`ux_u16`、`ux_u32`、`ux_i32`、`ux_f64` 及其 `_put` 写入。DataView 切片通过 `ux_data_offset(base, bytes, offset, width)` 支持非对齐访问，目前接口使用小端字节序。负返回值为 `UX_VIEW_BOUNDS=-1` 或 `UX_VIEW_ALIGNMENT=-2`，调用者须先检查再读写。整数写入截为目标位宽，f32 写入舍入到单精度。入口仍由 `ujs2wasm.com` 编译，现有 u8、f32 与 span 指令序列保留。验收运行 `sh ./unisacc.com ujs/product/cli-host.cx -- --ux-views` 及 `--mutate-ux-views`。

修订(cdx2): C2 的 `f64-math.ujs` 由编译器随消费者一起编成 Wasm。三角函数的角度按弧度计，`sin/cos/tan` 对绝对值大于 1048576 的输入返回 NaN。正切在极点附近的误差受余弦分母影响。`hypot` 接收两个坐标。域外反三角函数、非正对数、指数上下溢和带符号零分别按独立金值检查。测试覆盖已列采样点，超大角度约简、全域正确舍入和可变参数 `hypot` 仍待补齐。

修订(cdx2): C3 使用 `struct Position { x: f64; y: f32; }` 形式声明缓冲布局。引用其他记录时类型须先声明。字段按自然对齐，`Position.size` 含尾部填充。`Mesh.origin.x` 给出嵌入字段的字节偏移，`Mesh.origin.size` 给出嵌入类型自身的大小。使用 `ux_record_index(base, count, index, Mesh.size, Mesh.align)` 检查记录数组，再经 C1 标量接口读写。字典可保存记录偏移，缓冲的生命周期仍由调用者负责。

修订(cdx2): C4 以 `closure(callback, initialValue, ...)` 显式建立可变捕获环境。回调的首个参数接收环境列表。闭包可作为函数返回值、局部变量或第一个函数参数，同一闭包的别名共享捕获，不同构造得到独立环境。环境分配在当前 Wasm 调用的受限堆内。当前最多 16 个 i64 捕获，回调类型在编译时确定。自动捕获外层词法变量、异构回调集合和跨宿主 ABI 保存闭包仍缺。

修订(cdx2): C5 接受最长 1024 字节的有效 UTF-8 字面量，运行时保留 UTF-8 拼接。`len(s)` 返回字节数。`str_slice(s, start, end)` 使用半开字节区间，越界或切入 UTF-8 字符内部会拒绝。`ux_format_i64` 输出十进制并覆盖 i64 两端。NUL 和反斜线转义沿用现有拒绝，浮点精度格式与 UTF-16 索引仍缺。

修订(cdx2): C6 保留每模块一个业务入口体，允许声明至多八个原生 Wasm 导出别名，例如 `alias uxe_double(1);` 将首个 i64 参数绑定为操作号，并保留其余参数和返回值的类型。将所有参数绑定后可提供零参数常量读取。`uxe_math_v2` 等独立命名空间按依赖顺序实例化，桥核对版本、导出名、签名和独立内存。当前别名仅绑定前缀 i64 字面量，不支持同一模块内任意多个独立导出体。

修订(cdx2): C7 为 `u32_` 和 `i32_` 两组提供 `wrap/add/sub/mul/and/or/xor/shl/shr/rotl/rotr`。输入先截低 32 位，操作数与移位量都按 Wasm 规则处理，结果以零扩展或符号扩展读回 i64。`i32_shr` 为算术右移，`u32_shr` 为逻辑右移。

修订(cdx2): C8 用 `sh ./unisacc.com ujs/tests/f64-bench.cx -- baseline 20000` 与 `-- unroll 12000` 检查本机编译与 host1 执行。两个输入都内联同一个加法函数并执行 10000000 次 f64 加法，`.cx` 金值为 1250001。三次实测普通循环中位数 15387 ms，手工四路展开中位数 8306 ms，生成物分别为 2191 和 2282 字节。上限单位为毫秒，供同机回归。该基线包含 host1 启动开销，未测浏览器吞吐，也未引入自动展开优化。`-- mutate` 把循环变为一次，原金值不变。

修订(cdx2): C8 浏览器补测用 `sh ./unisacc.com ujs/tests/f64-bench-chrome.cx -- baseline` 与 `-- unroll`。同两份源码经 `ujs2wasm.com` 编译，在真实 Chrome WebAssembly 实例内预热一次，`performance.now()` 计七次调用，普通循环中位数 16.1 ms，范围 16.0 至 16.3 ms。四路展开中位数 16.4 ms，范围 16.2 至 21.0 ms。本次 Wasm 编译加实例化分别为 0.6 和 1.2 ms，单列于执行之外。两种循环读回 1250001。`-- mutate` 清零循环次数，读回 1 并退出 1。

修订(cdx2): M1 Vector2 在基座缓冲区使用两个连续 f64，调用者为 clone 等操作提供目标偏移。三角函数沿用 C2 角度域，BufferAttribute 适配连续 f32 记录，随机数由调用者注入，宽高和遍历通过分量接口读取。用 `sh ./unisacc.com ujs/tests/vector2-math.cx` 复跑 55 项 r160 JSON 和独立金值对照，另检查越界数组索引。`--mutate` 用减法替换加法检验断言。此门和 M0 的编译器检查使用 `ujs/ujs2wasm.com`，须先用 `sh ./unisacc.com ujs/product/build-ape.cx` 构造未跟踪的 `ujs/ujs.com`。清单生成和 `--check` 只读受跟踪的源码与证据入口。

修订(cdx2): M1 Vector3 使用三个连续 f64。矩阵沿用列主序，相机变换接收两份矩阵，Euler 记录以三个 f64 角度加一个 u32 顺序码表示。分量读取、随机源、连续 f32 属性和角度域与 Vector2 使用同一合同。`sh ./unisacc.com ujs/tests/vector3-math.cx` 检查 74 项独立金值与 r160 JSON，`--mutate` 将点积的右操作数换为左操作数。Vector2 与 Vector3 共享舍入、限幅和金值读回。

修订(cdx2): M1 Vector4 使用四个连续 f64。矩阵乘法保留齐次分量 w，角轴转换的输入为单位四元数或列主序纯旋转矩阵。`sh ./unisacc.com ujs/tests/vector4-math.cx` 检查 51 项独立金值与 r160 JSON，包括恒等与半周奇异点附近的分支，`--mutate` 移除矩阵变换后仍用原金值。

修订(cdx2): M1 Matrix3 接收行序参数并存成列主序 f64。乘法支持目标与任一输入重合，求逆对奇异矩阵写零，法线矩阵从 Matrix4 的左上 3×3 构造。`sh ./unisacc.com ujs/tests/matrix3-math.cx` 检查 28 项独立金值与 r160 JSON，`--mutate` 交换矩阵乘法顺序。

修订(cdx2): M1 Matrix4 接收十六个行序标量并存成列主序 f64。为整体传入十六个矩阵值，传统内联调用的参数暂存上限扩为 32，局部槽仍为 256。`decompose` 使用 C9 原生函数，把构造和分解留在同一个 G-math 用例。分解适用于仿射位姿，投影使用显式坐标模式。用 `sh ./unisacc.com ujs/tests/matrix4-math.cx` 检查 44 项独立金值与 r160 JSON，`--mutate` 调换矩阵乘法顺序。

修订(cdx2): M1 Quaternion 使用 xyzw 顺序的四个 f64。插值采用短弧，零范数归一化为单位四元数。数组与 JSON 的数值载体是明确的缓冲位置，随机样本由调用者注入，对象变更回调归 M2。`quaternion-math.cx` 实跑 32 项独立金值和 r160 JSON，`--mutate` 调换四元数乘法顺序。

修订(cdx2): M1 Euler 使用三个 f64 角度与一个 u32 顺序码。纯旋转矩阵可按六种顺序提取角度，奇异点使用各顺序规定的零轴。四元数转换与重排由调用者提供独立临时缓冲。变更回调归 M2。`euler-math.cx` 比对 19 项独立金值和 r160 JSON，`--mutate` 将 YXZ 矩阵按 XYZ 提取。

修订(cdx2): M1 数学层的验收入口是 `ujs/tests/<小写类名>-math.cx`。对应 r160 夹具位于 `ujs/practice/uxe/test/`，每项独立编译并读回。MathUtils 使用调用方持有的 Mulberry32 状态，并接收外部随机样本和 UUID 熵。`mathutils-math.cx` 对照 37 项，含六种固有欧拉顺序、类型归一化、两步随机序列、负模和 UUID。

修订(cdx2): Color 的 Wasm 接口按缓冲类型分别提供 RGB、十六进制、样式和已有颜色入口。CSS 解析使用显式长度字节视图，覆盖 r160 的整数或百分比 RGB、逗号 HSL、短长十六进制和命名颜色。148 个不透明色作为事实表，`sh ./unisacc.com ujs/tools-ux/gen-color-names.cx -- --check` 核生成表，`color-style-math.cx` 比对 CSS 读写。颜色事实来源为 [CSS Color Module Level 4 命名颜色表](https://www.w3.org/TR/2026/CRD-css-color-4-20261118/#named-colors)。

修订(cdx2): M1 组合源实测触发 `too many fns`。`compiler_min.cx` 的 `MAXFN` 为 128，`MAX_NATIVE` 为 16。包围球与平面检查只装载各自实际需要的向量和包围盒源，并保留相交测试的原生早退。M2 若需要把所有数学类合在一个编译单元，先用 0c 模块链接划定边界，再实测函数预算。

修订(cdx2): M1 出口以确认的 21 个数学类为准。对标树曾把 5 个 Interpolant 同时列在 M1 数学与 M8 动画，清单现在按照逐阶段开发计划归入 M8，仍保留在 A 栏分母中。Box3 对象入口读取展开的子树包围盒、顶点和世界矩阵，Frustum 对象入口读取局部包围球与世界矩阵。M2 的场景对象负责更新并填充这些记录。两个对象适配分别对照 r160 树遍历与视锥查询，篡改精确包围盒或世界矩阵均返回失败。

规则不变:小步提交,每步单独提交并贴输出,每个门含篡改红,既有门与 UXE 回归全绿后才提交,不放宽断言,不向 docs 发布(发布由我另请 cc 合入)。每个里程碑结束产出:对标表该里程碑行状态更新、门清单、可演示页(放 ujs/ 内测试页,不进 docs)。

| 阶段 | 任务 | 预计提交 | 出口 |
|---|---|---|---|
| S3 | 生成器 `ujs/tools-ux/gen-three-inventory.cx` 解析 `three.module.js` 导出表为 TSV,`--check` 与篡改自测;TSV 含 A、B、C 栏、里程碑列、现状列(默认缺)、门编号列;对标树渲染成 `ujs/prd_uxe_three.md` 附表;现状按严格口径重判 | 3 到 4 | 清单与基线百分比,A 栏已有项数、B 栏已有项数 |
| M0 | C1 到 C14b，编译器诊断与根因分别提交 | 15 到 19 | 编译器门全绿，基线记录 |
| M1 | 数学类按 Vector2/3/4、Matrix3/4、Quaternion、Euler、Color、ColorManagement、Box2/3、Sphere、Plane、Ray、Triangle、Frustum、Line3、Cylindrical、Spherical、SH3、MathUtils 推进，每类含 G-math 门与本地 three.js 参考 JSON | 18 到 24 | 21 个 M1 数学项全部已有 |
| M2 | 对象模型与布局表生成器;EventDispatcher、Layers、Object3D(add、remove、attach、世界矩阵缓存、遍历、getWorldPosition 等方法族)、Group、Scene、Clock;BufferAttribute 14 种、BufferGeometry、InstancedBufferGeometry、InterleavedBuffer;B 栏 Usage 与 Type 常量 | 14 到 18 | G-struct 门，D1 私有探针验证 Scene 与 Object3D 接线 |
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

M5 Toon。对照使用色值 0x804008、白色方向灯强度 pi、法线与光线点积 0.25，纹理红通道为 179、179、179、255。r160 的 WebGL1 默认着色器报 GL_OES_standard_derivatives 未启用。默认、RedFormat、RGBA8 三组黑色，RedFormat 后读回 GL 错误 1282，详见 `/tmp/devsvc-flow-21556397298-3996671/out/x.out`。同参数的 r160 WebGL2 三组皆为 [108,53,6,255]，无着色器错误，详见 `/tmp/devsvc-flow-21556442652-4004264/out/x.out`。UXE 已对齐显式 RGBA8 梯度，点积改为 0.5 的反例转红。默认梯度在非零导数下的过渡仍待补。

M12 上下文恢复缺口。M4 已测资源代数换代与丢失时的拒绝。M12 须用真时钟验证 WebGL 恢复事件、重建同一渲染目标、旧句柄拒绝及新目标像素读回。

合计约 200 到 260 个提交,按每天 12 到 20 个提交估,13 到 20 个工作日量级。此估计偏乐观,每个里程碑末复估一次。

修订(cdx2): M2 基座布局由 gen-object-layouts.cx 生成。固定树用 object3d-struct.cx 对照 4 个节点的世界矩阵、可见性、遍历与世界空间查询。scene-bounds-struct.cx 在浏览器链接独立的场景与查询模块，读回 Box3 的对象边界与 Frustum 的对象裁剪。几何记录由消费模块持有，场景模块交付矩阵标量。D1 仍使用既有产品渲染路径，发布包接入场景模块尚未验证。图像编码和序列化等未验证的方法保留清单的部分状态。

修订(ccapi): D1 出口使用 `d1-scene-probe.cx` 在私有页启动两个 Wasm 模块。产品建立 Scene 及 Object3D 父子树，同一基座完成世界矩阵更新和投影。独立 CX 按既有 D1 相机方程计算开场和三个转向代表状态，逐项读回三维相机空间坐标和二维像素坐标。正式 D1 场景图切换排在 M4 GPU 路径之后。

修订(cdx2): M3b 的 `g_geometry.cx` 对 23 类网格生成器及 BufferGeometryUtils 各追加 15 个值。编译产物将生成缓冲转为通用网格，再读回克隆数据、组、平移后包围盒与源数据。独立 CX 从顶点金值算预期，本地 r160 用相同原顶点复算。类型专属参数 JSON、完整相机与路径序列化等仍列部分。

修订(cdx2): M3 的基础口径是 A 栏 43 项和 C 栏 3 项都有真实入口与独立门。23 个几何类包括 21 个 r160 核心几何类，以及 ConvexGeometry 和 TextGeometry 两个扩展。BufferGeometryUtils 是另计的扩展工具。各构造器的顶点、法线、UV 和适用的索引由 CX 用独立金值对照浏览器读回数据。ConvexGeometry 的输出按坐标和法线规整顺序后比对。测试页在 `ujs/practice/uxe/test/`。

修订(cdx2): 严格覆盖仍标为部分。CubeCamera 缺渲染目标更新，ArrayCamera 缺渲染器多视口接入，几何类还缺若干材质分组、序列化与更多输入场景。ShapeUtils 支持单个凹轮廓和一个内孔，多孔与通用归并待补。ExtrudeGeometry 支持无倒角简单轮廓，Path 支持线段和贝塞尔段。TextGeometry 消费已解析字形轮廓，字体加载属于 M9。ConvexGeometry 覆盖有界一般位置点云，共面多边形面待补。BufferGeometryUtils 覆盖同布局网格合并。

修订(cdx2): 复跑 C 栏门时，用本地 three.js r160 的 `examples/jsm` 中四个文件，按原文件名平放在一个工作目录，将目录作为 `convexgeometry-struct.cx`、`textgeometry-struct.cx`、`buffergeometryutils-struct.cx` 的第一个参数。第二个参数 `--mutate` 启用反例。文件分别来自 `geometries/ConvexGeometry.js`、`math/ConvexHull.js`、`geometries/TextGeometry.js` 和 `utils/BufferGeometryUtils.js`。扩展源码只用于对照。 [three.js r160 扩展源码](https://github.com/mrdoob/three.js/tree/r160/examples/jsm)

修订(cdx2): D4 从 M7 恢复时先接场景树与城镇实例批，再接四腿刚体层级和步态，随后接跟拍相机及烟发射生命周期。已有本地 `town-motion` 的行走与跟拍位姿金值，线上还未迁移。M7 先验两节刚体 FK/IK、实例变换和遮挡，M8 再验混合器。烟的发射、排序与回收是另一个门，M7 若只交像素印章，保留在部分。M9 的压缩编解码器和第三方格式逐项估算。未纳入当前清单的插件、扩展及设备能力不能用提交数估计当作已覆盖。

### 5.1 M7 网格、实例与蒙皮

接口草案为 `Mesh_init(node, geometry, material)`、`Mesh_setMorphInfluence(mesh, index, value)`、`InstancedMesh_setMatrixAt(mesh, index, matrix)`、`BatchedMesh_addGeometry(batch, geometry)`、`LOD_addLevel(lod, node, distance, hysteresis)`、`Skeleton_update(skeleton)`、`SkinnedMesh_bind(mesh, skeleton, matrix, mode)`。Mesh、Sprite、Points、Line 与 Bone 复用 M2 对象前缀，几何与材质继续使用独立句柄。实例矩阵内存按列主序连续存放，颜色和形变权重各用独立视图。批次记录容量、活跃槽、几何范围与可见标记，槽复用须检查代数。LOD 按距离排序并保存前一层级用于迟滞。Skeleton 持有骨句柄、逆绑定矩阵和矩阵调色板。Attached 与 Detached 模式分别使用当前世界矩阵或绑定时矩阵求逆。

独立结构门读取父子世界矩阵、实例局部与世界变换、包围体、批次槽复用和 LOD 阈值两侧的选择。r160 拼同一场景，包括共用几何的两个 Mesh、遮挡中的三个实例、跨距离往返的 LOD，以及两骨骼四权重的顶点。像素门补点大小、线段断点、闭环端点与面朝相机的精灵。篡改实例索引、移除迟滞、替换逆绑定矩阵或互换绑定模式，各应命中对应门。网格拾取的最终覆盖证据由 M10 补齐。

预计提交顺序为 Mesh 与形变，InstancedMesh，BatchedMesh 的几何容量与实例表，LOD，Sprite，Points，Line 三种，Bone，Skeleton，SkinnedMesh 与两种绑定模式，最后接场景遍历与 GPU 绘制对照。保留低性能实现和缺少语义的条目为部分。

### 5.2 M8 动画数据与求值

接口草案为 `Track_init(name, times, values, stride, interpolation)`、`Interpolant_evaluate(track, t, cursor, out)`、`Clip_init(tracks, duration, blend)`、`AnimationMixer_clipAction(mixer, clip, root)`、`AnimationMixer_update(mixer, dt)` 与 `PropertyBinding_bind(root, path)`。时间与值的视图各自记录类型、长度和步幅，字符串使用 UTF-8 字节跨度。插值缓存保存区间索引，向前与反向跳时均须重定位。Action 记录本地时间、权重、时间缩放、循环计数、混合模式与启停状态。Mixer 在帧内按绑定槽聚合，保留原属性值和恢复所需引用计数。

G-anim 独立门覆盖七类 Track、离散与线性、三次插值端点、四元数最短弧与归一化。固定时间序列对照 r160，包括倒放、跨多个循环边界、PingPong、一次播放、权重淡入淡出、加法层与原值恢复。以整数步序驱动，验收关键帧间的数值、事件顺序和停止后的场景属性。篡改关键帧时间、求值方向、归一化或加法顺序，各自失败。

预计依次交付视图与插值器、数值类轨道、四元数轨道、布尔与字符串轨道、Clip 与轨道处理、绑定路径与属性写入、Action 循环、Mixer 调度、混合与事件，以及 D4 四腿从固定姿态到混合姿态的探针。

### 5.3 M9 加载器与图像解码

接口草案为 `LoadingManager_begin/end/error`、`FileLoader_request/receive`、`ImageCodec_probe/decode`、`ObjectLoader_parse` 与 `GLTFLoader_parse`。宿主只交付字节、时钟与请求结果。基座读头、校验偏移、解码与组装；产物通过事务暂存表发布。加载记录包含资源 URI、请求代数、完成状态与引用计数。图像记录包含格式、宽高、行步幅、像素存储范围及色彩元数据。GLB 的 chunk、bufferView 和 accessor 分别校验总长、偏移、分量类型、对齐与元素数量。

G-io 使用小型 PNG、JPEG、嵌入 PNG 的 GLB、带蒙皮动画的 GLB 和 r160 场景 JSON。PNG 查尺寸、滤波还原与 RGBA 精确值；JPEG 单列有损误差口径并按参考像素确认。GLB 对照规范化网格、材质、UV、骨架和动画轨道；失败门包括截断、非法 offset、整数溢出、稀疏索引越界、错误扩展以及失败后无半注册资源。来源和许可证随夹具登记。

PNG 解码切片支持非隔行 RGBA8，以及存储、固定 Huffman 和动态 Huffman 三种 Deflate 块。Wasm 验证 IHDR、块 CRC、zlib Adler-32，恢复五类扫描行滤波后才写 RGBA 输出。`png-header-io.cx`、`png-stored-io.cx`、`png-fixed-io.cx` 与 `png-dynamic-io.cx` 使用程序合成的小图和浏览器像素对照。动态树需要额外 3008 字节工作区。调色板、其他色型和隔行仍待覆盖。

JPEG 首个像素解码切片支持单分量 8 位基线灰度，包括 DC 和 AC 熵解码、反量化、二维逆变换及边缘裁切。`jpeg-gray-io.cx` 与 r160 ImageLoader 对照同一张合成 JPEG，并验证截断、填充位错误和失败后输出未发布。彩色、渐进与重启间隔仍待覆盖。

GLB 当前支持 2.0 清单与内嵌 BIN、密集访问器、三角形位置与索引、标准材质的颜色与标量、内嵌 RGBA8 PNG、蒙皮关节与逆绑定矩阵，以及平移动画通道。各切片独立检查字节或几何范围，失败时保留旧输出记录。切片门对照 r160 核心类，整景装配和 r160 GLTFLoader 插件端到端门仍待完成。外部 URI、稀疏访问器、网格其他属性、JPEG 纹理接线、骨骼实例化、其他动画通道与统一事务回收仍待覆盖。

预计提交顺序为请求管理，字节视图与加载器基类，PNG 容器与解码，JPEG 头与基线解码，图像及纹理装载，场景 JSON 两阶段链接，GLB 容器与访问器，材质与图像关联，蒙皮动画连接及失败回收。每种编码范围独立记录；渐进 JPEG、压缩扩展等只有实测通过才升格。

### 5.4 M10 拾取与控制

接口草案为 `Raycaster_setFromCamera(ray, camera, ndc)`、`Raycaster_intersect(root, query, hits)`、`Controls_feed(control, events)`、`Controls_step(control, dt)`。查询记录包含 near、far、层过滤、递归标记和点线阈值。命中记录包含距离、世界点、网格及实例句柄、索引、UV 与面索引。控制器保存目标、球坐标、速度、输入按钮和指针代数，拖动、滚轮与触屏共同消费既有输入 ABI。

G-ray 用透视和正交及带旋转缩放的对象树，对照 r160 的命中顺序和数值。覆盖材质单面、层过滤、近远裁剪、实例 ID、LOD 代理、线点阈值及蒙皮顶点。控制门播放同一拖拽、捏合、滚轮和固定步长轨迹，与 r160 同事件输入对照相机终值与事件顺序。反例分别改矩阵逆、近远限制、层掩码和指针结束处理。

预计依次交付相机射线、Mesh 命中与结果排序、实例与批次、点线精灵与 LOD、蒙皮、Orbit 与 Map、PointerLock、Transform 控制及浏览器输入读回。样本命中与对应 HUD 或选中效果一起核对。

### 5.5 M11 后处理、辅助对象与音频

接口草案为 `EffectComposer_addPass/render/resize`、`Pass_execute(input, output)`、`Helper_update(node, source)`、`AudioMixer_render(frames, dst)`。Pass 描述采样输入、目标格式、尺寸、清屏、混合及交换标记，统一通过 M4 目标和资源代数。Helper 只生成场景可见的调试数据。音频布局分声源、监听器和 PCM 输出环；使用帧索引、通道数、采样率与距离模型参数，宿主只负责提交输出。

后处理门从单一小图到逐格像素，依次对照 RenderPass、Bloom、Outline、FXAA 及调色，含 resize、旁路、目标生命周期和多 Pass 顺序篡改。辅助对象读回边界盒、相机视锥、骨和灯光方向，并以 r160 同变换场景对照。音频门读取合成脉冲和两路混音的 PCM，检查音量、循环、衰减、定位、分析窗口与关停静音；用独立公式和 r160/WebAudio 离线图对照。

预计依次交付目标复用与 Pass 调度、各 Pass 与像素门、几何辅助族、灯光和相机辅助族、骨架辅助、AudioListener 与缓冲播放、PositionalAudio 与分析、跨场景生命周期。

### 5.6 M12 双路径整合与发布前复核

两条路径从相同的 Scene 更新和渲染列表获取对象与资源记录。GPU 继续发版本化命令包，软件端消费同一清单并保留项目像素风采样与混合。产品持有输入、玩法、镜头与样式规则。每页仍为静态壳、宿主桥、基座和产品 Wasm，按页登记资源及导出哈希。

先验基座同输入的变换、可见性、排序和资源计数，再按 D1、D2、D3、D4 各一批私有页面与对照。D1 比飞行战斗、音频事件、开场与三航向，D2 比三航点、桥宽、岸线和标题倒影，D3 比三位置和可见操作，D4 比行走、跟拍、步态、烟与机械腹部。每页保留既有真时钟 CDP 输入、终值和篡改门，软件桥 300 行，GPU 桥 600 行，双路径分别跑白名单。WebGL 上下文丢失后验证资源重建、旧代数拒绝和同机位像素回读。

预计提交为共享场景消费接口，软件渲染接线，GPU 上下文恢复，四页分别迁移，每页独立证据审阅，最终清单重算与缺口可复现性复核。输入输出制品逐项确认后才申请发布。D5 的放行以独立重算口径和所有者确认作准。

### 5.7 M8 至 M11 并行边界

M8 可先做纯数据插值及虚拟属性绑定，M9 可先做二进制解析和图像解码，M10 可先基于 M1 的 Ray 和 M3 的相机实现静态几何拾取，M11 可先做 PCM 和离屏 Pass 夹具。各线独立维护自身模块、事实行及门；共同对象布局、GPU 包、宿主桥和生成表由集成窗口串行重算。

M8 对真实骨骼的绑定等 M7 句柄稳定，M9 的 GLB 蒙皮与动画组装消费 M7 和 M8 的已验记录，M10 的实例及蒙皮拾取消费 M7 更新后的矩阵和顶点，M11 的骨辅助依赖 M7，后处理依赖 M4 目标、颜色输出和阴影资源。跨边界数据先以定版夹具对读，合并后再跑真实模块交互和已有门，避免将夹具结果视为整体验收。

## 6 风险与诚实声明

(1) 99% 不是对 three.js 全部运行时行为的保证,而是对清单与门的覆盖;任何未经门的项不算。
(2) `ujs.com` 的性能与表达力是最大风险。M0 的 C3、C4、C8 若无法达到,GPU 路径的基座规模会受限,需要回头改编译器而不是绕开。
(3) three.js 的 WebGL 着色器体量很大。ShaderChunk 逐块对标,不整体搬运;PBR 的 IBL 部分依赖 PMREM,排在 M6。
(4) 软件路径与 three.js 的像素对照不做,因为风格不同;它的目标是 D1 到 D4 观感不退。
(5) 许可:三.js 为 MIT,仅本地对照与清单来源,不拷贝其源码进 UXE 实现。实现一律独立编写。
