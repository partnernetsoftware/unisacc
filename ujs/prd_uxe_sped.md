# UXE 规格与 three.js 对标

从 [`prd.md`](prd.md) 切出。ABI 真源:[`uxe/HOST_ABI.md`](uxe/HOST_ABI.md)(不抄符号表)。对标 three.js **r187**(发布 2026-10-30,[GitHub r187](https://github.com/mrdoob/three.js/releases/tag/r187);API 名以 [three.js docs](https://threejs.org/docs/) 为准)。只对标 API,不抄源码,不引入 three 依赖。

本任务未重跑批次或 `.cx`。已端到端验证只记账 **gpu-tri 单三角 212B** 与批次名 `ujs-gpu-tri`(见 [`practice/uxe/demo/G1-submit.md`](practice/uxe/demo/G1-submit.md));其余标 UNMEASURED。历史 PASS 不并入本任务分子。

## 0. 从 prd 切出的 UXE 缺口

| 号 | 事实 | 计划 | 验收 |
|---|---|---|---|
| U1 | gpu-tri 真 GPU(单三角 212B,headless chrome 138 SwiftShader) | G1-submit.md | 批次 `ujs-gpu-tri` title+FNV+Lambert 容差 24+反例;本任务未重跑,历史记账 PASS |
| U2 | D1-D5 `.ujs` 无 `function`,未触 MAXFN=8 | 先表再改常量 | `rg '^function' ujs/tests/examples/{fly,water,ink,oneshot,ghibli}.ujs` 无输出;2026-11-17 五文件计数全 0 |
| U3 | 无 sqrt/sin/cos;`f64 mod` 拒绝 | G3-math.md | `./ujs/ujs.com` 含 `%` 的 f64 源码非零;页 JS 仍算旋转 |
| U4 | `run_step` 调 main;顶层 let 每帧清 | G4-step.md | 全局 yaw/wind 跨 step 仍在;顶层 let 不跨帧;persist 另见 G4 |
| U5 | D5 观感未达 | ghibli-world/DESIGN.md | AUTO `GHIBLI_OK`;观感人工 |
| U6/M7 | demo 发布位置=`docs/uxe/<id>/`,GitHub Pages 指 main 的 `docs/`,站点 https://partnernetsoftware.github.io/unisacc/ 。与 M2 并行优先。现状:首页只链 asteroid/drone/live 且线上 404,`docs/uxe` 文件数 0。回滚:恢复 docs/index.html 原文。 | pages.tsv + gen-uxe-pages.cx + 断链检查 | 未开工;合入 main 由集成者带 docs |


---

## 1. 范围

UXE = (1) 浏览器宿主 (2) UXEP / UXIN 字节契约 (3) 渲染器。

核:(1) wasm 组包 (2) 步进。宿主:(1) 提交 (2) 画。

核 vs 宿主:

(1) 核只调 `host_*`,不知 canvas / DOM / fetch / 键。符号以 HOST_ABI.md 为准。

(2) 一帧:`host_frame_begin` -> `host_input_read` -> 步进 -> `host_gpu_submit`(一整包 UXEP) -> `host_frame_present` -> `host_request_frame`。

(3) 禁止逐物体 `host_draw_*`;禁止核内 `import three`。

(4) 写包原语:`store_bytes` -> 线性内存窗口 -> `host_gpu_submit(ptr,len)`。不新开第二 import(G1-submit.md)。

---

## 2. 现状(代码路径与行号)

读自工作树,不是运行时探针。

| 文件 | 行号 | 事实 |
|---|---|---|
| `uxe/scene.js` | 3-13 | `PerspectiveCamera`:fovy/near/far/eye/target |
| `uxe/scene.js` | 16-44 | `InstanceCloud`:xyz/scale/yaw/pitch/roll/color/emissive/metalness/roughness/meshId/billboard |
| `uxe/scene.js` | 46-63 | `Scene`:clouds Map、clearColor、fog、ambient、最多结构上的 lights 数组 |
| `uxe/meshes.js` | 5-6,59,66 | mesh_id 0 八面体、1 船楔、2 盒、3 XZ 平面 |
| `uxe/meshes.js` | 8-81 | 预置 positions;`allMeshes()` 供双后端 |
| `uxe/math.js` | 2-51 | 列主序 mat4、identity、perspective、lookAt、mul;无 Vector/Quaternion |
| `uxe/packet.js` | 21-28,47-114 | UXEP 编码当前 v3(clear/camera/fog/ambient/lights/clouds+emissive/metal/rough/mesh_id);**不编码** yaw/pitch/roll/billboard |
| `uxe/packet.js` | 139-212 | decode 接受 v1-v3 |
| `uxe/input.js` | 19-37,84-140 | UXIN v2 36B;flags 指针在内/suicide/touch/look-stick |
| `uxe/renderer-webgl.js` | 103-154 | WebGL1 上下文、实例化扩展、dpr<=2 的 resize |
| `uxe/renderer-webgl.js` | 156-204 | 实例绘制;yaw/pitch/roll;meshId===3 时 uWave;billboard |
| `uxe/renderer-webgl.js` | 209-248 | DEPTH_TEST、清 color+depth、两盏方向光、FogExp2 近似、ink rim |
| `uxe/renderer-webgpu.js` | 90-202 | 创建设备、depth24plus、同样 dpr<=2 resize |
| `uxe/renderer-webgpu.js` | 204-330 | 实例绘制 **无** 欧拉/billboard/wave/ink;alpha 恒 1 |
| `uxe/host-browser.js` | 25-44 | `createBrowserHost`;prefer auto 则先 WebGPU 失败回退 WebGL |
| `uxe/host-browser.js` | 223-254 | `applyObjectPacket` 填 Scene/Camera 后 `renderer.render` |
| `uxe/host-browser.js` | 271-338 | `host_input_read` 合成 UXIN |
| `uxe/host-browser.js` | 342-353 | `host_gpu_submit` 解码 UXEP 或接受调试对象 |

host1 侧:`host_gpu_submit` 符号可导入,读线性内存后 no-op,**不是** 真 GPU。真 GPU 验收只覆盖浏览器 gpu-tri。细节不抄,见 HOST_ABI.md 与 G1-submit.md。

---

## 3. 对标表

字段:(1) three.js 特性 (2) 类别=核心/扩展 (3) UXE 现状=有/部分/无 (4) 差距 (5) 实现位置=核/宿主/核+宿主/— (6) 验收命令 (7) 优先级 (8) 对应 G/M 号。

**有**:代码路径覆盖该能力的可运行子集,且不靠 three。**部分**:同名能力只有切片。**无**:无实现。

验收命令列:现有 `.cx`/批次名,或 `无,待写`。挂 `ujs-gpu-tri` 的行表示该特性落在 gpu-tri 像素路径上;**本任务未重跑**,不得读成「本会话通过」。

| three.js 特性 | 类别 | UXE 现状 | 差距 | 实现位置 | 验收命令 | 优先级 | 对应 G/M |
|---|---|---|---|---|---|---|---|
| Object3D 场景图(父子/矩阵/可见性) | 核心 | 无 | Scene 只有 named InstanceCloud Map,无父子与 matrixWorld | 宿主 | 无,待写 | P2 | M4 |
| Object3D.add/remove/traverse | 核心 | 无 | 无节点 API | 宿主 | 无,待写 | P2 | M4 |
| Object3D.position/rotation/quaternion/scale | 核心 | 部分 | 实例只有 xyz+yaw/pitch/roll+均匀 scale,无四元数/非均匀缩放;二进制 UXEP 还不带欧拉 | 宿主 | 无,待写 | P1 | M4 |
| Object3D.matrix/matrixWorld 自动更新 | 核心 | 无 | 宿主每帧 lookAt+perspective 乘 VP,物体无局部矩阵 | 宿主 | 无,待写 | P2 | M4 |
| Group | 核心 | 无 | 无分组节点 | 宿主 | 无,待写 | P2 | M4 |
| Scene | 核心 | 部分 | uxe/scene.js:46-63 有 clouds/clearColor/fog/ambient/lights,不是 Object3D 容器 | 宿主 | 无,待写 | P1 | G1 |
| Scene.background(色/贴图/立方体) | 核心 | 部分 | 仅 clear rgba;无背景贴图 | 宿主 | `sh ./unisacc.com tools/check-ujs-gpu-tri.cx`(批次 `ujs-gpu-tri`;本任务未重跑) | P1 | G1 |
| Scene.environment / environmentIntensity | 核心 | 无 | 无 IBL | 宿主 | 无,待写 | 搁置 | H4 |
| PerspectiveCamera | 核心 | 部分 | scene.js:3-13 fovy/near/far/eye/target;无 aspect/film/layers/matrix | 宿主 | `sh ./unisacc.com tools/check-ujs-gpu-tri.cx`(批次 `ujs-gpu-tri`;本任务未重跑) | P0 | G1 |
| OrthographicCamera | 核心 | 无 | math.js 只有 perspective() | 宿主 | 无,待写 | P2 | M5 |
| Camera 基类(projectionMatrix/view) | 核心 | 部分 | 每帧现场算 P/V,无对象化 Camera | 宿主 | 无,待写 | P2 | M4 |
| CubeCamera | 核心 | 无 | 无立方体贴图相机 | 宿主 | 无,待写 | 搁置 | H4 |
| StereoCamera | 核心 | 无 | 无立体 | 宿主 | 无,待写 | 搁置 | — |
| ArrayCamera | 核心 | 无 | 无多视口相机数组 | 宿主 | 无,待写 | 搁置 | — |
| WebGLRenderer | 核心 | 部分 | renderer-webgl.js:103-250 WebGL1+ANGLE_instanced_arrays;非 WebGL2 类 API | 宿主 | `sh ./unisacc.com tools/check-ujs-gpu-tri.cx`(批次 `ujs-gpu-tri`;本任务未重跑) | P0 | G1 |
| WebGLRenderer 清屏(color+depth) | 核心 | 有 | renderer-webgl.js:211-214 clearColor+COLOR|DEPTH;gpu-tri 走此路径 | 宿主 | `sh ./unisacc.com tools/check-ujs-gpu-tri.cx`(批次 `ujs-gpu-tri`;本任务未重跑) | P0 | G1 |
| WebGLRenderer.setPixelRatio | 核心 | 部分 | resize 内 dpr=min(devicePixelRatio,2) 写死封顶,无 API | 宿主 | 无,待写 | P1 | H3 |
| WebGLRenderer.setSize / 窗口 resize | 核心 | 部分 | renderer-webgl.js:146-154;host-browser.js 监听 resize | 宿主 | 无,待写 | P1 | H3 |
| WebGLRenderTarget / 离屏 RT | 核心 | 无 | 只画到 canvas 默认缓冲 | 宿主 | 无,待写 | P2 | M6 |
| WebGLRenderer.antialias | 核心 | 部分 | getContext antialias:true;无开关/MSAA 档 | 宿主 | 无,待写 | P2 | M6 |
| WebGLRenderer.outputColorSpace / toneMapping | 核心 | 无 | shader 直出 vec4,无色彩管理 | 宿主 | 无,待写 | 搁置 | H4 |
| WebGPURenderer | 核心 | 部分 | renderer-webgpu.js:90-333;无 yaw;无 ink/wave/billboard;host 优先 WebGPU 失败回退 WebGL | 宿主 | 无,待写 | P1 | G1 |
| WebGPU 深度附件 | 核心 | 部分 | renderer-webgpu.js:184-191 depth24plus;ensureDepth 随 resize | 宿主 | 无,待写 | P1 | G1 |
| Renderer 后端选择 auto/webgpu/webgl | 核心 | 部分 | host-browser.js:27-44 与 HOST_ABI prefer;页参见 ABI 文,不抄 | 宿主 | 无,待写 | P1 | G1 |
| viewport / scissor | 核心 | 部分 | 仅 gl.viewport 全画布;无 scissor API | 宿主 | 无,待写 | P2 | M5 |
| BufferGeometry | 核心 | 部分 | meshes.js 预置 positions Float32Array,无 index/normal/uv/groups | 宿主 | 无,待写 | P1 | M4 |
| BufferAttribute | 核心 | 无 | 无通用 attribute 对象 | 宿主 | 无,待写 | P2 | M4 |
| InstancedBufferAttribute / InstancedBufferGeometry | 核心 | 部分 | 每云 xyz/scale(/yaw) 动态 ARRAY_BUFFER+divisor;非 three 类 | 宿主 | 无,待写 | P1 | M4 |
| InterleavedBuffer | 核心 | 无 | 属性分缓冲 | 宿主 | 无,待写 | 搁置 | — |
| BoxGeometry | 核心 | 部分 | meshes.js:37-59 MESH_BOX=2 单位盒三角,无分段参数 | 宿主 | 无,待写 | P1 | M4 |
| PlaneGeometry | 核心 | 部分 | meshes.js:61-66 MESH_PLANE=3 XZ 双三角;HOST_ABI 预置表未列 plane,以代码为准 | 宿主 | 无,待写 | P1 | M4 |
| SphereGeometry | 核心 | 无 | 无球面 | 宿主 | 无,待写 | P1 | M4 |
| CylinderGeometry | 核心 | 无 | 无柱 | 宿主 | 无,待写 | P2 | M4 |
| ConeGeometry | 核心 | 无 | 无锥 | 宿主 | 无,待写 | P2 | M4 |
| TorusGeometry | 核心 | 无 | 无环 | 宿主 | 无,待写 | 搁置 | M5 |
| CircleGeometry | 核心 | 无 | 无圆盘 | 宿主 | 无,待写 | 搁置 | M5 |
| OctahedronGeometry | 核心 | 部分 | meshes.js:8-18 MESH_OCTA=0 正八面体(岩),非参数化 | 宿主 | 无,待写 | P1 | M4 |
| TetrahedronGeometry | 核心 | 无 | 无 | 宿主 | 无,待写 | 搁置 | — |
| IcosahedronGeometry | 核心 | 无 | 无 | 宿主 | 无,待写 | 搁置 | — |
| CapsuleGeometry | 核心 | 无 | 无 | 宿主 | 无,待写 | 搁置 | — |
| TorusKnotGeometry | 核心 | 无 | 无 | 宿主 | 无,待写 | 搁置 | — |
| RingGeometry | 核心 | 无 | 无 | 宿主 | 无,待写 | 搁置 | — |
| ShapeGeometry / ExtrudeGeometry / LatheGeometry | 核心 | 无 | 无轮廓挤出 | 宿主 | 无,待写 | 搁置 | — |
| EdgesGeometry / WireframeGeometry | 核心 | 无 | 无边提取 | 宿主 | 无,待写 | 搁置 | — |
| TubeGeometry / PolyhedronGeometry | 核心 | 无 | 无 | 宿主 | 无,待写 | 搁置 | — |
| 自定义楔形(非 three 名) | 核心 | 部分 | meshes.js:20-35 MESH_SHIP=1 船楔;three 无此内置 | 宿主 | 无,待写 | P1 | M4 |
| Mesh | 核心 | 部分 | 预置 mesh_id 实例云,不是独立 Mesh 对象 | 宿主 | `sh ./unisacc.com tools/check-ujs-gpu-tri.cx`(批次 `ujs-gpu-tri`;本任务未重跑) | P0 | G1 |
| InstancedMesh | 核心 | 部分 | scene.js:16-44 InstanceCloud;WebGL drawArraysInstanced;WebGPU draw instance | 宿主 | 无,待写 | P1 | M4 |
| BatchedMesh | 核心 | 无 | 一云一种 mesh_id,无合并多几何批次对象 | 宿主 | 无,待写 | P2 | M5 |
| Points | 核心 | 无 | H3 余:UXEP points;shader 不写 gl_PointSize | 宿主 | 无,待写 | P1 | H3 |
| Line | 核心 | 无 | 无 LINE 拓扑 | 宿主 | 无,待写 | P1 | M5 |
| LineSegments | 核心 | 无 | 无 | 宿主 | 无,待写 | P1 | M5 |
| LineLoop | 核心 | 无 | 无 | 宿主 | 无,待写 | 搁置 | — |
| Sprite | 核心 | 部分 | WebGL billboardXZ+uBillboard;UXEP 二进制无 billboard 字段;WebGPU 无 | 宿主 | 无,待写 | P1 | H3 |
| SkinnedMesh / Skeleton / Bone | 核心 | 无 | 无蒙皮 | 宿主 | 无,待写 | 搁置 | — |
| LOD | 核心 | 无 | 无距离档 | 宿主 | 无,待写 | P2 | M6 |
| InstancedMesh.count 动态 | 核心 | 部分 | cloud.count 截断 subarray;无 per-instance color 数组(色在云级) | 宿主 | 无,待写 | P1 | M4 |
| Material 基类(side/transparent/opacity/visible) | 核心 | 无 | 无材质对象;云级 color/emissive/metal/rough | 宿主 | 无,待写 | P2 | M5 |
| MeshBasicMaterial | 核心 | 无 | 片元必做环境+两灯 Lambert 软化,无 unlit 开关 | 宿主 | 无,待写 | P2 | M5 |
| MeshLambertMaterial | 核心 | 部分 | WebGL FS / WebGPU fs_main: n·L * soft(rough);gpu-tri 按 Lambert 中心色验收(不含 rim) | 宿主 | `sh ./unisacc.com tools/check-ujs-gpu-tri.cx`(批次 `ujs-gpu-tri`;本任务未重跑) | P0 | G1 |
| MeshPhongMaterial | 核心 | 无 | 无镜面高光项 | 宿主 | 无,待写 | P2 | M5 |
| MeshStandardMaterial | 核心 | 部分 | metalness/roughness 只做 albedo 混合与漫反射 soft,非 Cook-Torrance | 宿主 | `sh ./unisacc.com tools/check-ujs-gpu-tri.cx`(批次 `ujs-gpu-tri`;本任务未重跑) | P1 | G1 |
| MeshPhysicalMaterial | 核心 | 无 | 无清漆/透射/IOR/sheen | 宿主 | 无,待写 | 搁置 | H4 |
| MeshToonMaterial | 核心 | 部分 | WebGL uInk rim 近似 NPR;非梯度贴图 toon | 宿主 | 无,待写 | P2 | M5 |
| ShaderMaterial | 核心 | 无 | 着色器写死在 renderer,核不可交 GLSL/WGSL | 核+宿主 | 无,待写 | 搁置 | M6 |
| RawShaderMaterial | 核心 | 无 | 无 | 宿主 | 无,待写 | 搁置 | — |
| PointsMaterial | 核心 | 无 | 无 | 宿主 | 无,待写 | P1 | H3 |
| LineBasicMaterial / LineDashedMaterial | 核心 | 无 | 无 | 宿主 | 无,待写 | P1 | M5 |
| SpriteMaterial | 核心 | 无 | billboard 仍走实体着色 | 宿主 | 无,待写 | P2 | H3 |
| MeshNormalMaterial | 核心 | 无 | 无 | 宿主 | 无,待写 | 搁置 | — |
| MeshDepthMaterial / MeshDistanceMaterial | 核心 | 无 | 无 | 宿主 | 无,待写 | 搁置 | — |
| ShadowMaterial | 核心 | 无 | 无阴影 | 宿主 | 无,待写 | 搁置 | M6 |
| MeshMatcapMaterial | 核心 | 无 | 无 | 宿主 | 无,待写 | 搁置 | H4 |
| 透明混合 + 排序 | 核心 | 无 | 片元 alpha 恒 1.0;无 blend/排序 | 宿主 | 无,待写 | P1 | M5 |
| 深度测试 | 核心 | 有 | WebGL DEPTH_TEST;WebGPU depth24plus less;gpu-tri 走深度清屏 | 宿主 | `sh ./unisacc.com tools/check-ujs-gpu-tri.cx`(批次 `ujs-gpu-tri`;本任务未重跑) | P0 | G1 |
| 深度写 / polygonOffset | 核心 | 部分 | 默认写深度;无 polygonOffset API | 宿主 | 无,待写 | P2 | M5 |
| 面剔除(side/front/back/double) | 核心 | 无 | 未设 CULL_FACE;双面画预置三角 | 宿主 | 无,待写 | P2 | M5 |
| 视锥剔除 frustumCulled | 核心 | 无 | 有 count 的云全画 | 宿主 | 无,待写 | P2 | M5 |
| AmbientLight | 核心 | 部分 | UXEP v2 ambient color+intensity;shader uAmbient*uAmbI | 宿主 | `sh ./unisacc.com tools/check-ujs-gpu-tri.cx`(批次 `ujs-gpu-tri`;本任务未重跑) | P0 | G1 |
| HemisphereLight | 核心 | 无 | 无天空/地面两项 | 宿主 | 无,待写 | P2 | M5 |
| DirectionalLight | 核心 | 部分 | 包内 lights[] 任意长,着色器只取 L0/L1 两盏 | 宿主 | `sh ./unisacc.com tools/check-ujs-gpu-tri.cx`(批次 `ujs-gpu-tri`;本任务未重跑) | P0 | G1 |
| PointLight | 核心 | 无 | 无位置衰减灯 | 宿主 | 无,待写 | P2 | M5 |
| SpotLight | 核心 | 无 | 无 | 宿主 | 无,待写 | P2 | M6 |
| RectAreaLight | 核心 | 无 | 无 | 宿主 | 无,待写 | 搁置 | — |
| Light 基类 / intensity/color | 核心 | 部分 | 方向光 dir/color/intensity 结构体,无 Light 类 | 宿主 | 无,待写 | P1 | G1 |
| LightShadow / 阴影贴图 | 核心 | 无 | 无 shadow map/PCF | 宿主 | 无,待写 | P2 | M6 |
| DirectionalLightShadow | 核心 | 无 | 无 | 宿主 | 无,待写 | P2 | M6 |
| PointLightShadow | 核心 | 无 | 无 | 宿主 | 无,待写 | 搁置 | M6 |
| SpotLightShadow | 核心 | 无 | 无 | 宿主 | 无,待写 | 搁置 | M6 |
| LightProbe / 球谐 | 核心 | 无 | 无 | 宿主 | 无,待写 | 搁置 | — |
| Fog(线性) | 核心 | 无 | 无 near/far 线性雾 | 宿主 | 无,待写 | P2 | M5 |
| FogExp2 | 核心 | 部分 | 1-exp(-(d*depth)^2) 近似;UXEP fog density+color | 宿主 | `sh ./unisacc.com tools/check-ujs-gpu-tri.cx`(批次 `ujs-gpu-tri`;本任务未重跑) | P1 | G1 |
| Texture | 核心 | 无 | H4:map_id·host_asset;现无采样器 | 宿主 | 无,待写 | P1 | H4 |
| Texture.wrapS/wrapT(repeat/clamp/mirror) | 核心 | 无 | 无 | 宿主 | 无,待写 | P1 | H4 |
| Texture.magFilter/minFilter | 核心 | 无 | 无 | 宿主 | 无,待写 | P1 | H4 |
| Texture.generateMipmaps / anisotropy | 核心 | 无 | 无 | 宿主 | 无,待写 | P2 | H4 |
| Texture.colorSpace / flipY | 核心 | 无 | 无 | 宿主 | 无,待写 | P2 | H4 |
| CubeTexture | 核心 | 无 | 无 | 宿主 | 无,待写 | 搁置 | H4 |
| DataTexture / Data3DTexture | 核心 | 无 | 无 | 宿主 | 无,待写 | 搁置 | H4 |
| CanvasTexture / VideoTexture | 核心 | 无 | 无 | 宿主 | 无,待写 | 搁置 | — |
| CompressedTexture / CompressedArrayTexture | 核心 | 无 | 无 | 宿主 | 无,待写 | 搁置 | H4 |
| FramebufferTexture | 核心 | 无 | 无 | 宿主 | 无,待写 | 搁置 | — |
| Raycaster | 核心 | 无 | 无射线/拾取;输入是 UXIN 轴 | 宿主 | 无,待写 | P2 | H3 |
| Clock | 核心 | 部分 | host_time()->performance.now 毫秒;无 getDelta 对象 | 宿主 | 无,待写 | P2 | G4 |
| Timer | 核心 | 无 | 无 | 宿主 | 无,待写 | 搁置 | — |
| Layers | 核心 | 无 | 无 32 位层掩码 | 宿主 | 无,待写 | 搁置 | — |
| EventDispatcher | 核心 | 无 | 无事件总线 | 宿主 | 无,待写 | 搁置 | — |
| Vector2 | 核心 | 无 | 输入用裸 mx/my;无类 | 核+宿主 | 无,待写 | P2 | G3 |
| Vector3 | 核心 | 部分 | eye/target/xyz 为 length-3 数组,无 Vector3 API | 核+宿主 | 无,待写 | P1 | G3 |
| Vector4 | 核心 | 无 | 无 | 核+宿主 | 无,待写 | 搁置 | G3 |
| Matrix3 | 核心 | 无 | 无 | 宿主 | 无,待写 | 搁置 | G3 |
| Matrix4 | 核心 | 部分 | math.js:2-51 mat4/identity/perspective/lookAt/mul 列主序 | 宿主 | 无,待写 | P1 | G3 |
| Quaternion | 核心 | 无 | 旋转用欧拉 yaw-pitch-roll | 核+宿主 | 无,待写 | P2 | G3 |
| Euler | 核心 | 部分 | 实例 yaw Y 后 pitch X 后 roll Z(scene.js:33);核无 sin/cos(G3) | 核+宿主 | 无,待写 | P1 | G3 |
| Color | 核心 | 部分 | rgb 三元组;无 Color 类/十六进制/HSL | 宿主 | 无,待写 | P2 | G3 |
| Box2 | 核心 | 无 | 无 | 核+宿主 | 无,待写 | 搁置 | G3 |
| Box3 | 核心 | 无 | 无 AABB | 核+宿主 | 无,待写 | P2 | G3 |
| Sphere(包围球) | 核心 | 无 | 无 | 核+宿主 | 无,待写 | P2 | G3 |
| Plane(数学平面) | 核心 | 无 | 与 MESH_PLANE 几何不是同一物 | 核+宿主 | 无,待写 | 搁置 | G3 |
| Frustum | 核心 | 无 | 无六平面 | 宿主 | 无,待写 | P2 | G3 |
| MathUtils | 核心 | 无 | 无 lerp/clamp/degToRad 库(核亦无) | 核+宿主 | 无,待写 | P1 | G3 |
| Ray / Triangle / Line3 | 核心 | 无 | 无 | 核+宿主 | 无,待写 | 搁置 | G3 |
| Spherical / Cylindrical | 核心 | 无 | 无 | 核+宿主 | 无,待写 | 搁置 | G3 |
| 核内 sqrt/sin/cos/f64% | 核心 | 无 | compiler 无内建;f64 % 拒绝;旋转在 Host/shader。见 G3-math.md | 核 | 无,待写 | P0 | G3 |
| AnimationMixer | 核心 | 无 | 无 | 宿主 | 无,待写 | 搁置 | — |
| KeyframeTrack | 核心 | 无 | 无 | 宿主 | 无,待写 | 搁置 | — |
| AnimationClip | 核心 | 无 | 无 | 宿主 | 无,待写 | 搁置 | — |
| AnimationAction | 核心 | 无 | 无 | 宿主 | 无,待写 | 搁置 | — |
| AnimationObjectGroup / PropertyBinding | 核心 | 无 | 无 | 宿主 | 无,待写 | 搁置 | — |
| Audio | 核心 | 无 | HOST_ABI 规划 host_audio,未实现 | 宿主 | 无,待写 | 搁置 | — |
| AudioListener | 核心 | 无 | 无 | 宿主 | 无,待写 | 搁置 | — |
| PositionalAudio / AudioAnalyser | 核心 | 无 | 无 | 宿主 | 无,待写 | 搁置 | — |
| LoadingManager | 核心 | 无 | 无 | 宿主 | 无,待写 | 搁置 | — |
| FileLoader / Loader 基类 | 核心 | 部分 | host_asset_read fetch 字节;非 Loader 类 | 宿主 | 无,待写 | P2 | H4 |
| TextureLoader | 核心 | 无 | 无 | 宿主 | 无,待写 | P1 | H4 |
| ObjectLoader / BufferGeometryLoader / MaterialLoader | 核心 | 无 | 无 JSON 场景加载 | 宿主 | 无,待写 | 搁置 | — |
| ImageLoader / ImageBitmapLoader / CubeTextureLoader | 核心 | 无 | 无 | 宿主 | 无,待写 | 搁置 | H4 |
| Cache(Loader.Cache) | 核心 | 无 | 无 | 宿主 | 无,待写 | 搁置 | — |
| GLTFLoader | 扩展 | 无 | 禁 three 与搬 GLB;世界表在 .ujs | 宿主 | 无,待写 | 搁置 | — |
| DRACOLoader | 扩展 | 无 | 无 | 宿主 | 无,待写 | 搁置 | — |
| KTX2Loader | 扩展 | 无 | 无 | 宿主 | 无,待写 | 搁置 | H4 |
| Meshopt 解码 | 扩展 | 无 | 无 | 宿主 | 无,待写 | 搁置 | — |
| OBJLoader(只列) | 扩展 | 无 | 不计划 | — | 无,待写 | 搁置 | — |
| FBXLoader(只列) | 扩展 | 无 | 不计划 | — | 无,待写 | 搁置 | — |
| OrbitControls | 扩展 | 无 | 相机由核包 camera.eye/target;Host 不轨道 | 宿主 | 无,待写 | P2 | H3 |
| TrackballControls | 扩展 | 无 | 无 | 宿主 | 无,待写 | 搁置 | H3 |
| PointerLockControls | 扩展 | 部分 | host-browser.js pointerLock 选项+movement 累加写 UXIN mx/my;非 three 控件对象 | 宿主 | 无,待写 | P1 | H3 |
| TransformControls | 扩展 | 无 | 无 gizmo | 宿主 | 无,待写 | 搁置 | — |
| MapControls / ArcballControls / FlyControls / FirstPersonControls / DragControls | 扩展 | 无 | 无 | 宿主 | 无,待写 | 搁置 | H3 |
| EffectComposer | 扩展 | 无 | 无后处理图 | 宿主 | 无,待写 | 搁置 | M6 |
| UnrealBloomPass / Bloom | 扩展 | 无 | emissive 只加色,无阈值模糊 | 宿主 | 无,待写 | 搁置 | M6 |
| SSAOPass / SAOPass / GTAOPass | 扩展 | 无 | 无 | 宿主 | 无,待写 | 搁置 | M6 |
| FXAA / SMAA / SSAA / TAA | 扩展 | 无 | 仅上下文 antialias | 宿主 | 无,待写 | 搁置 | M6 |
| BokehPass / DOF | 扩展 | 无 | 无 | 宿主 | 无,待写 | 搁置 | — |
| OutlinePass | 扩展 | 无 | ink rim 不是描边通道 | 宿主 | 无,待写 | 搁置 | M5 |
| RenderPass / OutputPass / ShaderPass / CopyPass / MaskPass / ClearPass | 扩展 | 无 | 无 composer pass | 宿主 | 无,待写 | 搁置 | — |
| CSS2DRenderer | 扩展 | 无 | HUD 是页 DOM,不经核 | 宿主 | 无,待写 | 搁置 | — |
| CSS3DRenderer | 扩展 | 无 | 无 | 宿主 | 无,待写 | 搁置 | — |
| TextGeometry | 扩展 | 无 | 无 | 宿主 | 无,待写 | 搁置 | — |
| FontLoader / TTFLoader | 扩展 | 无 | 无 | 宿主 | 无,待写 | 搁置 | — |
| Water / Water2 | 扩展 | 部分 | MESH_PLANE + shader uWave 正弦位移(仅 meshId===3);非 three Water 反射 | 宿主 | 无,待写 | P2 | M5 |
| Sky | 扩展 | 无 | 清屏色当天空 | 宿主 | 无,待写 | P2 | M5 |
| Reflector / Refractor | 扩展 | 无 | 无 | 宿主 | 无,待写 | 搁置 | H4 |
| Line2 (fat lines) | 扩展 | 无 | 无 | 宿主 | 无,待写 | P2 | M5 |
| BufferGeometryUtils | 扩展 | 无 | 无 merge/computeVertexNormals | 宿主 | 无,待写 | P2 | M4 |
| Stats | 扩展 | 部分 | host _stats clouds/bytes/backend 与 __UXE_SNAP__;非 mrdoob Stats FPS 面板 | 宿主 | 无,待写 | P2 | — |
| WebGPU TSL / NodeMaterial | 扩展 | 无 | WebGPU 用写死 WGSL,无节点图 | 宿主 | 无,待写 | 搁置 | — |
| PMREMGenerator / RoomEnvironment | 扩展 | 无 | 无 | 宿主 | 无,待写 | 搁置 | H4 |
| WebXR / XRManager | 扩展 | 无 | 无 | 宿主 | 无,待写 | 搁置 | — |
| SkeletonUtils / CCDIKSolver | 扩展 | 无 | 无 | 宿主 | 无,待写 | 搁置 | — |
| DecalGeometry / ConvexGeometry | 扩展 | 无 | 无 | 宿主 | 无,待写 | 搁置 | — |
| HTMLTexture | 扩展 | 无 | r184 起 three 有;UXE 无 | 宿主 | 无,待写 | 搁置 | — |
| Compute / Storage textures(WebGPU) | 扩展 | 无 | 无 compute pass | 宿主 | 无,待写 | 搁置 | — |
| 一次提交整包(对标 renderer.render(scene,cam)) | 核心 | 部分 | host_gpu_submit 一包;核侧须 store_bytes+UXEP。符号链 HOST_ABI.md,不抄 | 核+宿主 | `sh ./unisacc.com tools/check-ujs-gpu-tri.cx`(批次 `ujs-gpu-tri`;本任务未重跑) | P0 | G1 |
| UXEP v3 二进制(clear/camera/fog/lights/clouds) | 核心 | 部分 | packet.js 编码 v3;无 yaw/uv/map_id/alpha;单三角 212B 已作为 gpu-tri 口径 | 核+宿主 | `sh ./unisacc.com tools/check-ujs-gpu-tri.cx`(批次 `ujs-gpu-tri`;本任务未重跑) | P0 | G1 |
| UXIN v2 输入快照 | 核心 | 部分 | input.js:19-37 36B;H3 指针已有,H3 余 action bit 未做 | 宿主 | 无,待写 | P1 | H3 |
| host_input_read | 核心 | 部分 | host-browser.js:271-338 键鼠触屏合成 UXIN | 宿主 | 无,待写 | P1 | H3 |
| host_request_frame / rAF | 核心 | 部分 | rAF+50ms 兜底;非 Clock | 宿主 | 无,待写 | P2 | G4 |
| 核持久 globals(对标每帧 Object3D 状态) | 核心 | 部分 | G4 persist 记账已绿;堆 trap 未做;本任务未实跑 | 核 | 无,待写 | P0 | G4 |
| store_bytes 写包窗口 | 核心 | 部分 | 设计 n<=4096、STORE_WIN=1015808;容量 G2/C5;本任务未实跑 | 核 | 无,待写 | P0 | G2 |
| 字面 list 上限 256 | 核心 | 部分 | prd C5;D2 岸依赖此容量。本任务未实跑 list257 | 核 | 无,待写 | P0 | G2 |

表数据行数 = **181**(不含表头)。

---

## 4. 汇总

口径:(1) 分母 = 上表数据行数 = 181 (2) 分子 = 现状为「有」**且**验收命令已在本任务实跑通过。无依据不写百分比。

本任务实跑分子 = **0**(未跑 `check-ujs-gpu-tri.cx` / `run-batch`)。

历史记账(不并入分子):G1-submit.md 记批次 `ujs-gpu-tri` PASS、单三角 UXEP v3 **212B**、headless Chrome 138 SwiftShader、title+FNV+Lambert 容差 24+反例。U1/C3 按该范围。无 chrome 应 UNMEASURED 退出 2,不是 PASS。

### 4.1 类别 x 状态(行数)

| 类别 | 有 | 部分 | 无 | 小计 |
|---|---:|---:|---:|---:|
| 核心 | 2 | 45 | 99 | 146 |
| 扩展 | 0 | 3 | 32 | 35 |
| 合计 | 2 | 48 | 131 | 181 |

### 4.2 全量

分母 181。分子 0。不写百分比。

现状「有」行数 = 2(深度测试、WebGL 清屏等);即使挂了 gpu-tri 命令,本任务也未实跑,故不进分子。

### 4.3 D1-D5 子集

实践页:`practice/uxe/demo/`。合同见 prd §0.3 与 [`plans/demo-three-ports.md`](plans/demo-three-ports.md)。**本任务未实跑 AUTO**,下表「用到的特性」来自规格/源码推断,验收 UNMEASURED。

| Demo | 用到的对标特性(推断) | 核要交 | 本任务 |
|---|---|---|---|
| D1 `fly/` | PerspectiveCamera、InstancedMesh 切片、MESH_SHIP、Ambient/Directional、FogExp2 切片、UXIN 轴、持久 yaw(G4) | 金 `return x+100*alt`=3868 | UNMEASURED |
| D2 `water-keeps/` | PlaneGeometry 切片、Water 波切片、list 容量 G2、多云颜色 | 岸 128 + pal 8 | UNMEASURED(G4 记 water 曾 `list too long`,是否已消以实跑为准) |
| D3 `ink-walk/` | MeshToon/ink rim 切片、折线世界(非 Line API) | 三层 x,z | UNMEASURED |
| D4 `oneshot/` | BoxGeometry 切片、6 实例位姿 | 6 盒 xyz | UNMEASURED |
| D5 `ghibli-world/` | Scene 云+种子表;观感未达 | 表长>=8 | UNMEASURED |

gpu-tri 不是 D1-D5;它是 G1 真 GPU 夹具,单三角 212B。

---

## 5. 路线(每一项 = 一个单一提交能完成什么)

不把多缺口塞进同一提交。

| 项 | 单一提交完成 | 不做 |
|---|---|---|
| **M2** | 核驱动 fly 步进:persist yaw(或已有 persist)经 `run_step` 跨帧;`store_bytes`+`host_gpu_submit` 交一包;AUTO 仍只核金/表,不核截图 | 不扩几何;不改 D2-D5;不引进 three |
| **M3** | G3 最小数学:核内建 `sqrt` **或** `sin`/`cos` 之一,带 `.ujs` 夹具非零/金值;或显式冻结「旋转永在 Host」并加拒绝用例 | 不同时四则全上;不改 renderer |
| **M4** | UXEP 二进制补齐 yaw/pitch/roll(与 WebGL 已画字段对齐)或新预置一种几何(Sphere **或** Cylinder 只选一) | 不一次加全套内置几何 |
| **M5** | 一种新图元或透明:Points **或** LineSegments **或** alpha 混合排序,只选一,进 UXEP 版本策略 | 不上 EffectComposer |
| **M6** | 离屏 RT **或** 第二盏以上灯光在 shader 循环 **或** 阴影贴图占位,只选一 | 不上 TSL |
| **H3 余** | UXEP 点精灵 **或** UXIN 再扩 1 个 action bit,只选一 | 不重做已有指针/摇杆 |
| **H4** | 一张 albedo:`map_id` + `host_asset_read` 采样;wrap/filter 各一种默认 | 不上 KTX2/mip 全档 |
| **M7** | 单一提交把一款 demo 静态面落到 `docs/uxe/<id>/` 并让首页可点(pages.tsv + 生成器);不断链 | 不改核 ABI;不合入策略由集成者带 docs |

H1/H2/H3 指针已在 HOST_ABI.md 标完成(雾灯、金属粗糙多 mesh、UXIN v2)。本文件不重复抄 ABI。

---

## 6. 约束

(1) 提交面:`host_gpu_submit` + `store_bytes` + UXEP。不新 import,不把包塞进字符串字面量。上限见 G2:`store_bytes` n<=4096;字面 list 256。

(2) 数学 G3:核无 `sqrt`/`sin`/`cos`;左操作数 f64 的 `%` 拒绝。旋转/相机/水波在 Host 或 shader,直到 M3 选定内建。

(3) 容量 G2:场景表必须能进核 list/`store_bytes` 窗口;超限失败不截断。

(4) 持久 G4:跨 `run_step` 的玩法状态走 persist globals,不靠「新实例碰巧」;顶层未标 persist 的 `let` 每帧清。堆 trap 仍未做(G4-step.md)。

(5) 核不知宿主;GPU 与密钥停 Host。

---

## 7. 未验证项

本任务未跑,一律 UNMEASURED:

(1) 批次 `ujs-gpu-tri` / `tools/check-ujs-gpu-tri.cx`(历史记账 PASS,本会话未重跑)。

(2) D1-D5 `?auto=1` 标题金。

(3) WebGPU 后端像素(gpu-tri 历史在 SwiftShader/WebGL 口径;WebGPU 路径未在本任务测)。

(4) UXIN 触屏/指针锁。

(5) `list65.ujs` / `list257.ujs` / store_bytes 4096 边界。

(6) G4 persist 绿集与堆 trap。

(7) host1 `host_gpu_submit` 除 no-op 以外的任何 GPU 声称。

(8) npm `test:uxe:all`(另门,未跑)。

---

## 8. 表重算方法

不强制本任务写工具。可用 `.cx` 重算时:

(1) 抽第 3 节管道表数据行(以 `|` 起、非分隔行、非表头)。

(2) 分母 = 行数。按列「类别」「UXE 现状」交叉计数,得 §4.1。

(3) 分子 = 现状为「有」且验收命令可执行且本机实跑 rc=0 的行。命令为 `无,待写` 的行永不进分子。

(4) 禁止用「部分」充「有」;禁止把历史 PASS 写进未重跑会话的分子。

(5) D1-D5 子集单独计数,不与全量混分母,除非先声明每一行属于哪几个 demo。
