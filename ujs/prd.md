# UJS —— 产品规格 v1.3

> **唯一耦合活文档**（规格 · 目标 · 思维树 · 记忆宫殿）。与代码同会话更新。  
> **版本计划**：[`plans/`](plans/)（`ujs.com` **0.0.7** ✓；[`plans/v0.0.7.md`](plans/v0.0.7.md) lex 扫描器切片已交付，不是 IntNet，不是其余九网）  
> **目录板块**：[`ARCHITECTURE.md`](ARCHITECTURE.md) · 开箱：[`README.md`](README.md) · Host：[`uxe/HOST_ABI.md`](uxe/HOST_ABI.md)  
> 归档：[`archive/`](archive/)

---

## 0. 一句话

**UJS** = 闭合 JS 子集 + 构造法确定性推理（理论）+ **`ujs.com` 生产力工具**（产品）。  
验收：版本计划门禁与可机读收据；不以截图猜 UI。

### 0.0 版本（`ujs.com` 0.0.x）

| 版 | 状态 | 计划 |
|---|---|---|
| **0.0.1** | ✓ host1 基线 | [`plans/v0.0.1.md`](plans/v0.0.1.md) |
| **0.0.2** | ✓ 生产力合同 | [`plans/v0.0.2.md`](plans/v0.0.2.md) |
| **0.0.3** | ✓ **`ujs/` 无 `.c`／`.py`** | [`plans/v0.0.3.md`](plans/v0.0.3.md) |
| **0.0.4** | ✓ **host1 wasm 验面** | [`plans/v0.0.4.md`](plans/v0.0.4.md) |
| **0.0.5** | ✓ **host1 跑面**（`--run`） | [`plans/v0.0.5.md`](plans/v0.0.5.md) |
| **0.0.6** | ✓ **日用一条链**（`FILE.ujs --run`） | [`plans/v0.0.6.md`](plans/v0.0.6.md) |
| **0.0.7** | ✓ **lex 扫描器切片**（`method_construct`；不是 IntNet，不是其余九网） | [`plans/v0.0.7.md`](plans/v0.0.7.md) |
| **0.0.8** | 计划：**host1 步进诚实**（文档+共享 runtime；不升版号） | [`plans/v0.0.8.md`](plans/v0.0.8.md) |

版号真源：`ujs.com --version`（**`ujs.com 0.0.7`**）。

**两个名词**：**host image** = APE／六目标／验面／`host1_package`；**method emit** = 表／流→`\0asm`。勿混。

**语言债**：`ujs/` 内 `.c`／`.py` = **0**；遗留 [`archive/ujs-legacy-py-c/`](../archive/ujs-legacy-py-c/)；续迁 [`plans/cx-migration.md`](plans/cx-migration.md)。

### 0.1 出货主产物（主人 2026-10-25）

| | |
|---|---|
| **主产物** | **`ujs.com`**——**ujs + wasm 类别的生产力工具**（跨架构单文件 `.com`；编／装／跑／自举收进同一壳） |
| **状态** | **0.0.7 已交付**（`ujs.com 0.0.7`）：无 `.c`／`.py`；host1 编／验／跑仍在；另加 lex 扫描器切片（`method_construct`）。不是 IntNet，不是其余九网（[`plans/v0.0.7.md`](plans/v0.0.7.md)） |
| **不是主产物** | 单独的 `ujs2wasm` CLI、`wat2wasm`、`compile.mjs`、零散 `.wasm`——是 **`ujs.com` 的子功能／实践脊**，应收进或由它调度 |

### 0.1b 产品 · 论文 · unisacc 关系（主人 2026-10-25 定调）

> 与旧稿「B 只 cite A、出货脚注」并存时，**以本小节为准**；论文正文改口另裁，不挡 `ujs.com` 产品推进。

| 层 | 角色 |
|---|---|
| **`ujs.com`（产品）** | 打造 **ujs+wasm 生产力工具**：日用入口、门禁、子功能收敛；证据标签 `host1_package`（及日后产品能力标签） |
| **Paper B（理论）** | **对齐并阐述**该产品类别的底层理论（闭合子集 · 构造法确定性推理 · 表／流→`\0asm`）；主张服务产品可辩护性，不是反过来用产品绿冒充理论完成 |
| **Paper A / `unisacc.com`** | **由 B 侧问题派生的 C 实践与形式化承载**；`unisacc.com` 的方向正确性是**同理论在另一语言面的佐证**，不是 UJS 的父产品。形式核／Lean 样本可仍落在 A 文，但**谱系叙述：B→A 派生，A 佐证回 B／unisacc** |

**三面收据仍勿混用**（Grok 评审采纳）：`host1_package` ≠ `m3_scaffold` ≠ `method_construct`。生产力工具主线推 host1；方法弧（m4／构造）写进 B 时单独标 `method_construct`。

**子功能 ↔ 今日入口（将收敛进 `ujs.com`）**

| 子功能 | 今日怎么用 | 将来在 `ujs.com` 里 |
|---|---|---|
| 编 `.ujs`→`\0asm` | **`ujs.com FILE.ujs`**（host1）· `node compile.mjs`／`compiler_core`（m3） | 默认驱动已进 `ujs.com`；全表 emit 见 archive／0.0.4+ |
| WAT→二进制 | 外部 **`wat2wasm`** 或常驻 wabt（仓外床实证） | **内部子步骤**，用户不直接当产品 |
| 跑／fold／acc | archive 遗留 CLI；日用走 `ujs.com`／`unisacc.com -run` | 子命令或 `-run` 类动态路径 |
| path-A 演示核 | `web-build` → `ujs_full.wasm` | 可选子产物，非 ship 必经 |
| 玩法步进核 | **`sim.wasm`** + Pages Host | 由 `ujs.com` 写出或加载的应用核；Pages 仍可吃 `.wasm` |
| UXE Host | `engine.js` + `{app}` | 浏览器侧不变；桌面侧由 `ujs.com` 提供工具链 |

**纪律**：对外说「UJS 产品」时，指 **`ujs.com`**（APE／六目标已可经 `.cx` 构建；覆盖见 §0.1 状态）。说「更全表／Pages」时，指下表实践脊。勿把 `wat2wasm`／`ujs2wasm` 说成最终出货物。进度与宫殿只维护**本文件**，不写进根 `prd.md`。

### 0.2 今日实践脊（已交付能力，非最终产品形）

| 层 | 今日交付 | 不是 |
|---|---|---|
| **core** | `bootRuntime` / `wasm_run` / `compile` · path-A `ujs_full` · **`compiler_core`** | 页内训练 · 完整 ES · 把 M3 说成 IntNet · **冒充已是 ujs.com** |
| **UXE** | Host + `{app}` · Pages 步进 **`sim.wasm`** · 共享 `engine.js` | Three 移植 · 合包 · ship 必经 `engine.wasm`（P0 已弃） |
| **构造** | `construct/` gold → 权重；全表 `ujs2wasm`；`web-build` → path-A | 应用运行时依赖 · 把 Python CLI 当最终产品 |

**路径 A**（共享 VM · 方法/演示）：`web-build` → `ujs_full.wasm`（曾用名 **`engine.wasm`**；**非** P0 ship 必经；内核仍 C+zig）。  
**路径 B**（直出 · 出货步进）：`compile.mjs` → **`compiler_core.wasm`** → `sim.wasm`（已交付；玩法核）。  
开发可先 B 写 `.ujs`，再把 UXE `{app}` 接到 B（§目标）。

**语言（闭合）**：表内全功能，表外永久拒绝。有字面量 / list·dict / let·const / if·while·for·switch / function·箭头 / rest·spread / 算术比较 / 索引成员 / `len`·`keys` / 只读外层闭包。无 `undefined` 双轨、`var`/hoisting、`this`/`class`/prototype、`eval`、async/generator、RegExp 字面量。条款全表见 [`archive/prd-v1.0.md`](archive/prd-v1.0.md)。

**不交付**：完整 ES · Three-in-wasm · 合包 · 用户 LLM key 进核/packet · UJS 直播主线 · **把外部 `wat2wasm` 当 UJS 品牌出货物**。

### 0.3 实践 demo（host1 + UXE,不挡 `ujs.com` 版号）

demo-v2 原始需求见 [`plans/demo-v2/requirements-original.md`](plans/demo-v2/requirements-original.md)。下表记录早期实践页验收点。当前四页的分层、出货和迁移以 0.3c 为准。


| 编号 | 页 | 核要交什么 | 完成时玩家应看到 |
|---|---|---|---|
| D1 | `fly/` | 金 `return x+100*alt`=3868;可选 `yaw`/`turn` 全局 | 追机;AUTO 标题 `FLY_OK` |
| D2 | `water-keeps/` | 岸 128 + pal 8;`turn`/`dist`/`hdg` | 八界灯舟;AUTO `WATER_OK` |
| D3 | `ink-walk/` | 三层 x,z 折线;`cam`/`turn` | NPR 可逛;AUTO `INK_OK` |
| D4 | `oneshot/` | 6 盒 xyz;`ang`/`turn` | 一发转轨;AUTO `ONESHOT_OK` |
| D5 | `ghibli-world/` | 种子表长>=8 草稿 | 未达观感合同;AUTO 暂 `GHIBLI_OK` |

另: `host1-games/` 是 host1 金闸(`HOST1_GAMES_OK_GL`),不是叙事世界。

**D5** 保留草稿证据，暂停开发。D1-D4 暂停新增页面细节，先整改三层架构。

### 0.3b 已核实缺口(2026-11-17)

记账:`practice/uxe/demo/G1-submit.md` `G3-math.md` `G4-step.md`。

#### UXE

规格、three.js 对标与路线见 [`prd_uxe_sped.md`](prd_uxe_sped.md)。摘要:浏览器宿主 + UXEP/UXIN + 渲染器;核组包步进、宿主提交与画;已端到端验证仅 gpu-tri 单三角 212B(批次 `ujs-gpu-tri`);ABI 真源 [`uxe/HOST_ABI.md`](uxe/HOST_ABI.md)。M7 demo 发布 `docs/uxe/<id>/` 见 sped U6。



#### ujs.com

| 号 | 事实 | 计划 | 验收 |
|---|---|---|---|
| C1 | 五切片 APE;`ujs.com` gitignore;无 win/arm64 | build-ape.cx | `sh ./unisacc.com tools/check-ujs-ape.cx` 末行 `UJS_APE_OK` failed=0。批次 `ujs-ape-slices` |
| C2 | `--smoke` 曾用 `build.cx` 盖 APE | run.cx 收尾 `build-ape.cx`+check | smoke 结束后 check-ujs-ape 仍绿 |
| C3 | host1 唯一 import 拷包交 renderer;其它 import 拒绝 | G1-submit.md | 同 U1;fly `--run` 3868 |
| C4 | 2026-11-17 实测(`--check --files` 两文件):`compiler_min.cx` **96**(同记录,未降;基数 96);`host1_vm.cx` **34**(tsv 行仍记 36,未改写基线;曾记 36)。故 host1<=34 已到,compiler<=50 仍差 46。注意全仓扫描当前另有超基线:`ujs/product/build.cx` 21>17、`ujs/product/ujs_main.cx` 60>52,`gold-{ic,lex,parse,rest,type}.cx`、`net-{gold,lex,parse}-check.cx`、`pack-nets.cx` 无基线行,`--check` 因它们 FAIL;两目标文件本身未超 | 本行;不并 G1 | 到期前 `tests/cxraw.cx --check --files` 两文件 |
| C5 | 字面 list 元素上限 **256**(`compiler_min` `n>256` -> `list too long`)。字符串字面量 0-255 非 NUL 仍是另一上限(prd 完成判据 / `str_lit256`);`compiler.ujs` 的 `DKEY=list(256)` 是键缓冲。动态 `list(n)` 不受字面 256 限,`store_bytes` n<=4096 | 本行 | `list65.ujs` 可编;`list257.ujs` 非零且含 `list too long`; oracle `uxep-tri-oracle.tsv` |




---

### 0.3c UXE 三层合同与迁移 S1

S1 记录现状和目标合同。S2 等督导与所有者确认，先补编译器，再逐页迁移，D1 优先。three.js 对标以功能验收为准，UXE 使用自己的版本化 API。下表现状以四页和相关实验为证据，标为部分的能力可能仍在 JS。已有的单项能力也须在基座 Wasm 中重新举证。

#### 基座 Wasm

拥有场景图、资源句柄、渲染和混音策略。负责所有权、句柄生命周期和内存边界。产品通过基座 API 建场景、提交状态、查询结果。

| 能力 | three.js 对标 | 现状 | 基座验收门 |
|---|---|---|---|
| 场景图 | Scene、Object3D、Group | 部分，scene.js | 挂载和移除后父子变换、可见性正确 |
| 相机 | PerspectiveCamera、OrthographicCamera | 部分，透视与路轨相机在 JS | 透视与正交的近远裁剪、屏幕投影金值 |
| 几何与材质 | BufferGeometry、InstancedMesh、MeshToonMaterial、MeshStandardMaterial | 部分，网格、toon、描边在 JS，PBR 待补 | 索引和实例、toon 三阶和描边、标准材质的金属度和粗糙度有像素门 |
| 光照与阴影 | DirectionalLight、PointLight、LightShadow | 部分，方向光与阴影贴图在 JS | 遮挡体移动时阴影跟随，点光衰减与多光源叠加可量测 |
| 雾与色调 | Fog、FogExp2、WebGLRenderer 色调映射与输出色空间 | 部分，U1 空气透视和调色在 JS | 雾随距离变化，色调与线性到 sRGB 有像素金值 |
| 纹理与精灵 | Texture、DataTexture、Sprite | 部分，UXEP v4 贴图有读回 | UV、过滤、透明遮罩、面向相机精灵各有像素样点 |
| 粒子 | Points、PointsMaterial 及发射器调度 | 部分，点绘制已有门，烟和浪由页面步进 | 确定性发射、生命周期、混合、排序和回收 |
| 动画与骨骼 | AnimationClip、AnimationMixer、SkinnedMesh、Skeleton | 部分，D3/D4 过程动画在 JS，缺骨骼 | 两骨蒙皮、插值、混合与暂停续播有顶点读回 |
| 拾取 | Raycaster | 缺可复用接口证据 | 屏幕射线、多命中排序、近远裁剪与空命中 |
| 音频 | Audio、AudioListener、PositionalAudio | 部分，D1 WebAudio 事件已有读回 | 解码或合成、混音与声像有 PCM 包络验收 |
| 输入 | EventDispatcher、Controls 适配 | 部分，UXIN 与页面键鼠已有证据 | 事件顺序、失焦清键、触屏与录制回放 |
| 装载 | LoadingManager、TextureLoader、GLTFLoader | 部分，宿主取字节和静态 GLB 实验已通 | 基座解析纹理和 GLB 网格、材质、贴图、蒙皮、动画，坏文件拒绝及释放 |

能力对标据 [three.js 官方 API](https://threejs.org/docs/)。仓内证据见 [UXE 专项规格](prd_uxe_sped.md)、[Host ABI](practice/uxe/HOST_ABI.md) 和 [素材实验](plans/demo-v2/asset-sources.md)。

#### 产品 Wasm

每页一份产品 Wasm，提供初始化、步进、销毁。只导入版本化的 UXE API，负责玩法状态、交互语义、随机状态、场景资料及 HUD。通用几何、矩阵、软件像素缓冲、投影、光栅与合成归基座。

| 页 | 产品逻辑 |
|---|---|
| D1 fly-next | 飞行与重开、追尾镜头、岛屿和海岸、敌机和波次、武器、命中、HP、KO、尾迹、HUD 及音效事件和音色数据 |
| D2 water-keeps | 操舵、碰岸减速、水道和植被布局、水色及倒影参数、桥灯、标题和粒子触发 |
| D3 ink-walk | 石桥、屋、山和植被布局、材质参数、漫游路线、风与视角交互 |
| D4 oneshot | 机械城镇与腹部布局、四足步态、行走约束、镜头跟拍、烟囱发射参数 |

#### 页面 JS 白名单和行数门

每页只加载共享 `host-abi.js`。HTML 保留静态画布、样式和模块 URL 数据。S2 将下表目标符号和签名登记到版本化 ABI 表。

| 函数 | 允许动作 |
|---|---|
| `boot` | 取 Wasm 字节、实例化、绑定两模块导入导出、按固定生命周期转发 |
| `host_canvas_open`、`host_canvas_size`、`host_frame_present` | 获取画布和尺寸，上传像素或交换画面 |
| `host_gpu_submit` | 校验基座定稿的命令与宿主对象句柄，转发 WebGL 创建、绑定、上传、编译、绘制和释放 |
| `host_input_read` | 采集键鼠、触屏、按钮及失焦原始事件，编码为 UXIN |
| `host_time`、`host_request_frame` | 读取单调时间，转发及取消帧回调 |
| `host_audio_write` | 将基座给出的 PCM 交给浏览器，处理音频设备状态和浏览器解锁 |
| `host_asset_request`、`host_asset_read` | 请求与完成状态、字节搬运、边界和错误转发 |
| `host_log` | 宿主 ABI 诊断转发 |

场景遍历、可见性、资源解码、着色器内容、键位解释、步进策略、音色和产品逻辑均由 Wasm 决定。桥内辅助函数只能服务于表内调用，按函数核对职责和宿主调用点。AUTO 从 Wasm 状态读回，测试探针置于测试目录。

每个导出页内联 JS 与事件属性各 0 行，其他 JS 文件 0 个，共享桥的传递闭包至多 300 物理行，每行最多 120 字符。按唯一规范路径计一次，包含注释、空白和无末尾换行的末行。禁止压缩或生成形式规避检查，禁止动态代码生成、脚本注入、非静态依赖和路径逃逸。独立 `.cx` 门解析实际导出 HTML 与 JS，沿依赖核函数符号、签名、宿主调用点和行数，输出页、路径、行数与违规条款。隔离副本分别加入超预算一行、非白名单函数、内联脚本和传递依赖中的外加 JS，皆须返回非零。

#### 编译器现状和前置缺口

0a 新增 `./ujs/ujs2wasm.com FILE.ujs -o OUT.wasm`。薄启动脚本复用 `ujs/product/build-ape.cx` 已构建的 `ujs/ujs.com`，原样转发参数和退出码。`--run` 由 host1 迷你 VM 执行。现有生成器已通过该入口重编 D1。`ujs/product/run-host.cx` 实跑 28 例通过，`ujs/product/corpus-host.cx` 实跑 28 个正例、8 个源码拒绝例及坏 Wasm 拒绝通过。S1 全套 smoke 的日志已到 `build-ape.cx: ok`，外层 57 秒截止在末项 `check-ujs-ape.cx` 完成前触发。历史 `compile.mjs`、归档 Python 和旧语料不作为目标构建入口。

| 特性 | 现入口的证据与限制 |
|---|---|
| 标量、控制流与 f64 | i64、f64 基础算术和比较、分支、循环、switch 和三元式有语料。`Math.sqrt(9.0)` 拒绝，退出 1，诊断 `dot non-dict`。所需数学内建及误差界待补 |
| 数组与字典 | list 下标、更新、f64 元素，dict 字面量、点访问有语料。字典键上限 8 字节。`new Float32Array(4)` 拒绝，退出 1，诊断 `expected ;`。连续 f32 和批量字节搬运待补 |
| 闭包 | 除无捕获语料外，实跑 `let offset = 3; let f = x => x + offset; return f(4);` 得 i64 7。此例覆盖外层读取，逃逸与可变捕获未测 |
| 字符串 | ASCII 字面量和拼接有语料。单字面量至多 255 字节，拒绝 NUL、非 ASCII 与转义，UTF-8 需有界字节 ABI |
| import 与链接 | 已有特设 `env.host_gpu_submit(i32 ptr,i32 len)`。`import { host_time } from "env"; return host_time();` 拒绝，退出 1，诊断 `expected =`。声明式宿主导入、跨模块链接与内存交接待补 |

用 `wasm-objdump -x` 核对当前 D1 导出，只有上述一个 import。导出有 `memory`、`main_export`、`run_step`、`host_reset`、`host_set_global`、值构造和标签读取等，页面依元数据定位输入槽，收 11 字节 v3 飞行提交。`tools/gen-d1-airframe.cx` 调 `sh ./ujs/ujs2wasm.com ujs/tests/d1-flight.ujs -o <临时 Wasm>` 并发布测试夹具。现有 `d1-flight.wasm` 来自该 `.ujs`，生成工具承担调用与比对。D2/D3/D4 的现行导出目录无产品 Wasm。

0b 为正式入口增加下列 ABI。上表的 `Math.sqrt` 和 `Float32Array` 仍是常规 JS 写法的拒绝例。

| 能力 | 0b 合同 |
|---|---|
| f64 | `f64_sqrt(f64)` 编为 Wasm f64.sqrt。host1 运行金例使用数值误差门。基座 `math.ujs` 提供 `ux_sin(angle)`，弧度范围含 -2048 到 2048，范围外返回 NaN。余弦用相位偏移 pi/2 后调用同一函数，偏移后受同一范围限制。独立样点绝对误差上限 2e-9 |
| 连续视图 | 每实例占用线性内存 [2097152, 8388608) 作为 ABI 缓冲区。偏移单位为字节。`ux_u8`、`ux_u8_put` 读取和写入 0 到 255。`ux_f32`、`ux_f32_put` 读写四字节小端 IEEE 754 binary32，UJS 标量读回类型为 f64，允许未对齐 |
| 有界跨度 | `ux_span(offset,count)` 先检查 i64 范围再返回线性内存地址，可取末尾零长度段。负数、超长和窄化前越界会陷阱。使用 ABI 视图的模块在装箱分配前也检查堆边界。模块间的内存所有权与交接仍由 0c 定义 |
| 文本 | 以偏移和明确字节数传递 UTF-8，NUL 可作为内容。`text.ujs` 的 `ux_utf8(offset,length)` 返回 1 或 0，拒绝截断、孤立续字节、过长编码、代理区与超出 U+10FFFF。越界跨度陷阱。字符串字面量原合同保持 |

0b 测试将 `.ujs` 库和测试片段拼为单源后编译，跨模块链接在 0c。验收入口为 `ujs/product/cli-host.cx`，预期来自独立 `.cx` 数值、位模式和 UTF-8 用例。用 `--mutate-ux-abi` 改一项数学输入、一个边界偏移和一个 UTF-8 续字节，沿用原期望并要求非零。

#### 构建、顺序与验收

基座入口规划为 `ujs/practice/uxe/base/uxe.ujs`。产品入口规划为 `ujs/practice/uxe/products/{d1,d2,d3,d4}/main.ujs`，各自目录存模块和资源事实表。S2 交付编译器后，分别用 `./ujs/ujs2wasm.com ujs/practice/uxe/base/uxe.ujs -o <阶段目录>/uxe.wasm` 和 `./ujs/ujs2wasm.com ujs/practice/uxe/products/d1/main.ujs -o <阶段目录>/product.wasm` 构建，余页同形。可以复用现编译器实现并保留 `ujs.com` 入口。两份 Wasm 都须由 `.ujs` 经正式的 `ujs2wasm.com` 编译，禁止手写 Wasm 字节、由 `.cx` 直接生成 Wasm 字节或用 JS 代替。生成器记录编译器、依赖和 ABI 版本，并以重编字节比较作为检查。授权发布时由 `ujs/practice/uxe/pages.tsv` 明列两个 Wasm、共享桥和数据，经 `gen-uxe-pages.cx` 导出与复核哈希。候选先导出到临时目录。

| 顺序 | 改动 | 机器门与观感验收 |
|---|---|---|
| 0a | 正式编译入口、固定输入身份 | 同源重编字节一致，兼容旧 D1 金值，坏源码拒绝且无产物 |
| 0b | f64 数学、连续字节与 f32 视图、有界内存和文本 ABI | `.cx` 独立金值，坏编码、越界及运算篡改须红 |
| 0c | import/export、模块链接与内存所有权 | 两份真实 `.ujs` 编译后在浏览器互调，坏版本、签名、越界、缺模块须红 |
| 1 | 基座像素缓冲、输入、资源、帧及 PCM 最小切片，迁 D1 | 飞行、武器、HP、KO、波次、重开、音频，固定开场与三航向比图 |
| 2 | 基座补混合和粒子，迁 D2 | 操舵、碰岸减速、桥宽 40%、倒影与标题，三航点比图 |
| 3 | 基座补场景、材质、阴影、雾与投影，迁 D3 | 路径、桥拱、厚度、叶区像素，沿轨三位置比图 |
| 4 | 基座补动画和实例，迁 D4 | 前后行走、跟拍、烟、齿轮圆度和面积，三位置比图 |
| 5 | 补齐剩余对标能力 | 依基座表逐项提交样例、机器结果和反例 |

每页沿用线上视口、时间和录制输入做同机位并排，保留已有区域阈值与反例。另跑路由相关既有门、JS 白名单和行数、Wasm 导入检查、CDP 实键终值、全量导出检查与十项 UXE 回归。关键行为移除须转红。

#### 逐文件归属

按现有导出清单定位源文件，随所属页迁移并验收对应 JS 退出。多页共用文件的其他副本随各页处理。

| 页 | 当前文件 | 迁移归属 |
|---|---|---|
| D1 | d1-depth-combat.js、d1-depth-audit.js | 战斗、地形与敌机为产品，通用像素图元归基座 |
| D1 | d1-pixel-aircraft.js、d1-solid-aircraft.js | 机体几何、配色与姿态为产品，光栅和投影归基座 |
| D1 | d1-audio.js、d1-audio-data.js | 音色和事件为产品，合成和混音归基座，PCM 输出归宿主 |
| D1 | index.html 内联、host1-runtime.js | 调度、追尾、HUD 和重开进产品，装载与宿主转发进共享桥 |
| D2 | water-reflection-audit.js、index.html | 水道、操舵、倒影与灯舟配方进产品，图元与像素操作进基座，HTML 留静态壳 |
| D3 | d3-composition.js、d3-painted.js、d3-bridge-brush.js、d3-worn-deck.js | 产品构图、桥面和叶丛配方 |
| D3/D4 | d3-arch.js、d3-foliage.js | 产品侧共享的桥拱与植被配方，通用网格方法进基座 |
| D3 | index.html 内联 | 造景、漫游和风步进进产品，装载进共享桥 |
| D4 | d4-town.js、d4-gait.js、d4-framing.js、index.html 内联 | 城镇、步态、跟拍和烟进产品，粒子与相机原语进基座 |
| D3/D4 | renderer-webgl.js、scene.js、meshes.js、math.js、font5x7.js | 渲染、场景、几何、数学和字库进基座，浏览器 WebGL 只作 ABI 转发 |
| D3/D4 | uxe-u2b.js | 投影方法进基座、尺度参数进产品、测量断言进测试 |

## 目标(按杠杆)


**主线按 `plans/0.0.x` 推进 `ujs.com` 生产力工具**；Pages／`{app}` 仍要可玩，但是实践脊，不挡 0.0.x 产品版号。门禁先于功能。不开新门面游戏。

**北极星（两层）**  
1. **产品形**：把 **`ujs.com`** 打成 **ujs+wasm 生产力工具**（编译／组装／运行／自举／调度），对标该类别的日用壳，而非「只有 ujs2wasm」。  
2. **理论形**：Paper B 写清支撑该工具的底层理论；方法弧（构造／m4）可并行，**收据分标签**；不以 host1 绿宣称 B 主张中心已卸。  

**工具链底座**：日常仍用根 **`unisacc.com` + `.cx`（`-run`）** 造 `ujs.com`（先例与同仓宿主）；`.cx` 只转调 Python **不算**消除依赖。这是工程依赖，**不是**「UJS 从属于 Paper A 产品」。

**域分工**：ujs／`ujs.com` = **JS+wasm 生产力主线** · unisacc／`unisacc.com` = **同理论的 C 佐证与工具链** · **同理论**（构造法确定性推理=编译）· **同产品形纪律**（跨架构 `.com`）· **同工具链纪律**（`unisacc.com`+`.cx`）。

### 自举阶梯

1. **今** Python construct 出 A（`engine`）+ B 工具（`ujs2wasm`）；Asteroid + drone Pages 步进默认 B
2. **#2b / M1 + M1b** 玩法步进直出（B）— **Asteroid ✓ · drone ✓**；build 仍一次 Python emit（M2 去掉）
3. 更多 construct／测试／胶水迁入 **`unisacc.com` + `.cx`** 与 UJS/wasm（front/emit 可在 wasm host 上用）
4. **自举** UJS 编译器：UJS→wasm；并交付 **`ujs.com`**；Python 只留构造/对照，不挡出货

**反漂移**：下一步必须把能力推进 **`unisacc.com`+`.cx`／JS/wasm／`ujs.com`**；Python construct **只许缩或持平**，禁止新增「仅 Python 才出货或才验收」的依赖；禁止用 `.cx` 外壳转调 Python 冒充迁移完成。

### 何时可弃 Python（退出条件）

分两层，**先弃出货、后弃构造**。没达到门闩就还不能喊「脱离 Python」。底座路径：**`unisacc.com` + `.cx`**（动态 `-run`），不是再发明第三套脚本语言。

| 层 | 弃什么 | 做成什么之后可以弃 | 现在卡在 |
|---|---|---|---|
| **P0 弃出货 Python** | `web-build` / `ujs2wasm` CLI / 预编译 embed **不再是 ship 必经** | ① Pages 玩法步进只加载直出 `.wasm`（B），不再 `wasm_run`+bytecode ② 改 `.ujs` 用 **JS/wasm 或将来 `ujs.com`** 重编译，而不是 `python3 -m ujs …` ③ `npm run ship:*` 里无 `python3` | **✓** ship 停拷 `ujs_full`→`engine.wasm`；A 核仅 `FORCE_WEB_BUILD` / `ujs.sh` 重建 |
| **P1 弃构造 Python** | gold / oracle / acc／床上探针也不靠 Python | 构造臂与验收胶水迁到 **`unisacc.com` + `.cx`**（及自举后的 `ujs.com`）；枚举门禁仍在；实际 `-run` 执行、保留拒绝与恢复合同 | unisacc 工具链仍在收口；刻意后置，**不预堆 0.0.44** |

**P0 可检里程碑（每步弃一点）**

| 里程碑 | 做成什么 | 门闩 | 弃掉什么 |
|---|---|---|---|
| **M1** | 运行时步进只跑 B-wasm | `ujs2wasm_step` 绿 · asteroid+drone **默认** B · 热路径无 `wasm_run` | 运行时对 bytecode/`wasm_run`（A）的依赖（**Asteroid+drone ✓**） |
| **M2** | 编译器本身是 wasm，在 Node/浏览器调用 | 输入 `.ujs`→`\0asm`；fold；**ship 脚本无 python3** · `./tests/ujs2wasm_compiler.sh` | 开发/CI 出货对 `python3 -m ujs` 的依赖（生成该编译器 wasm 的一次性 zig/C 尚可忍） — **✓** |
| **M3** | 编译器用 UJS 写并自举 | stage0→stage1→stage2 **字节一致** | Python 作为编译器实现语言 |

**一句话**：能扔掉出货 Python ≠ emit.py 更全，而是 **M1→M2→M3 门闩全绿**；P1＝构造／胶水走上 **`unisacc.com`+`.cx`**，再收进 **`ujs.com`**。

### 产物清单（理论 → 交什么）

**最终交件**问：是不是 **`ujs.com`**（或由其确定性写出的六目标／应用核）？  
**里程碑交件**问：是否把能力推进 JS/wasm／自举，并**可被 `ujs.com` 吸收为子功能**？

| 里程碑 | 必须落地的产物 | 不是产物（别做厚） | 状态 |
|---|---|---|---|
| **已有脚手架** | path-A `ujs_full` · ship-js Host · `ujs2wasm`／`compile.mjs` 等**子功能入口** | 再扩 Python 专属出货链；把 CLI 当最终品牌 | 脚手架 ✓；**≠ ujs.com** |
| **M1** | ① **`sim.wasm`** ② Pages 默认 B 步进 ③ `direct_step.js` ④ `uxe_ship_js` | 只 fold 绿却仍默认 A | **Asteroid ✓ · drone ✓** |
| **M2** | ① **`compiler.wasm`／`compiler_core`** ② **`compile.mjs`** ③ ship 无 python3 emit | 继续把 emit 只留在 `.py` 当日常「产品」 | **✓**；仍是子功能脊 |
| **M3** | ① **`compiler.ujs`** ② stage2≡stage1 ③ 自举门禁 | 「差不多能编」无字节一致 | **v16✓** · **P0✓** |
| **P1** | gold/oracle/acc／验收胶水在 **`unisacc.com`+`.cx`**（或自举构造器） | 在 ujs 里把 Python 构造成产品；`.cx` 转调 Python 冒充完成 | 后置 |
| **ujs.com host1** | 本机原生 CLI；`build.cx` | 冒充已覆盖 core | **✓** stage0 |
| **ujs.com host1-six** | `build-six.cx` 六目标原生 | 冒充 APE 一包 | **✓** |
| **ujs.com APE** | `build-ape.cx` 五片多形态；对齐 `unisacc.com` 容器形 | 冒充已嵌入 core；依赖用户机 Node／Python | **✓**（本机烟测；跨机切片另证） |

**三条线 → 收束到一个 `.com`**

```
用户程序线（玩法）   .ujs ──► sim.wasm ──► Pages          ┐
编译器线（工具）     compiler_core / compiler.ujs           ├──► 最终由 ujs.com 调度或写出
构造线（理论）       gold/acc（对照；P1→unisacc）          ┘
今日工厂             ujs2wasm · wat2wasm · compile.mjs     ──► 子功能，不是主产物名
```

**论文 / 对外叙事：两条脊（同纪律，不同产物）**

| 脊 | 是什么 | 诚实边界 |
|---|---|---|
| **构造 / jtape** | gold→IntNet→oracle；`acc` / fold / icfold；全表 `ujs2wasm` | 方法验收；表阶段仍欠全量 P-2 Lean（继承 Paper A） |
| **M3 出货** | 手写 `compiler.ujs`→`compiler_core.wasm`；stage2≡ · body≡ · ship 无 python emit | **不是** IntNet；交付面 = **UJS-1_ship**（渐扩），≠ 全表 UJS-1 |

写 Paper B / Release 时：出货写 **`ujs.com`**（host1）与／或 core（M3 实践）；`web-build` 是 path-A 残线；**host1 门** = `ujs/tests/run.cx`；UXE／Pages 另门；旧 `ujs.sh` 已归档。

```
0 emit 齐 · 2a ship-js     ✓
        ↓
M1 (=#2b) 步进默认 B       **Asteroid ✓ · drone ✓**
        ↓
M1b drone 切 B             ✓（对称产物）
        ↓
M2 编译器 wasm 化          **✓**（compile+host+sim+ship emit 无 python3）
        ↓
M3 UJS 自举                **v16✓** sim+drone body≡ · ! · &&/|| · ship via core
        ↓
P0 ship 无 A 核            **✓** 停拷 ujs_full；web-build 非 ship 必经
        ↓
P1 构造迁 unisacc          可选
```

（换核模板 / 门禁契约 / Host 加厚仍按杠杆穿插；不改变上表弃 Python 顺序与产物线。）

#### M3（协议；交付面见 v12…v16）

**协议**：stage0=`compiler.wasm`(C) 编 `compiler.ujs` → stage1 core；stage1 再编 `compiler.ujs` → stage2；**main body 字节一致**。host 拼进 `compiler_rt_stub.wasm`。

**v11 基线（仍成立）**

| 件 | 说明 |
|---|---|
| `compiler.ujs` | id>8 drain · GKEYS64 · f64 `(i-j)*TWO52/j` 无溢出 |
| `compiler.wasm` | len u32 · f64 lex ratio · MEM 128 |
| 门禁 | **sim+drone body≡stage0** · stage2≡stage1 |

**P0（✓ ship）**：Pages/ship 只依赖 `compiler_core.wasm` + `sim.wasm`；`compile.mjs` **默认** core。

#### M3 v16（✓ unary ! · &&/|| short-circuit · fold corpus on core）

| 件 | 说明 |
|---|---|
| `compiler_min.c` / `compiler.ujs` | v16：`!e` → i64.eqz+extend（0/1）；`a&&b`/`a\|\|b` 经 IF0/IF1/IF2 + i64 `lscratch`/`lsc` 短路（JS 值语义；仅 i64；ujs op 200/201）；一元 `-`/`!` 用 UNY base3 LIFO（避免额外 list local 撑破自举） |
| 门禁 | fold **logic**→7；stage2≡stage1 · sim+drone body≡stage0；setidx_globals 默认 core |
| 回落 stage0 | `UJS_COMPILER=wasm` 或 `UJS_REQUIRE_COMPILER_WASM=1`（body≡ 对照 / 重建 core） |

#### M3 v17（✓ 短 str · fold corpus on core）

| 件 | 说明 |
|---|---|
| `compiler_min.c` / `compiler.ujs` | v17：`"…"` 字面量 **0–7 ASCII、无转义**（界在字面量，非拼接结果）→ OP_SCONST/pack_key；`str+str` → `$str_cat`（func 18）；str handle 经 OP_HRET 返回 |
| `compiler_rt_stub.wasm` | 与 stage0 同版 host（含 str_cat）；`compile.mjs` splice 模板 |
| 门禁 | fold **str**→`hello ujs`（及边界回归）；stage2≡stage1 / body≡ = **code `fi===1` mainBody**；sim+drone 同 |
| 回落 stage0 | `UJS_COMPILER=wasm` 或 `UJS_REQUIRE_COMPILER_WASM=1`（body≡ 对照 / 重建 core） |

#### M3 v18（✓ ASCII 长 lit 0–255 · fold corpus on core）

| 件 | 说明 |
|---|---|
| 完成判据 | `"…"` **0–255 非 NUL ASCII、无转义**；NUL / ≥256 / `\` / byte>127 → parse reject；**dict/dot 键 ≤8**（RT `dict_get` 按前 8 字节 i64 比；更长显式拒绝，不静默碰撞）；stage0≡core 执行；stage2≡stage1；sim+drone；**不开** `fn` / tinyvm execute |
| `compiler_min.c` | 字面量池 `sdata`；`OP_SCONST a=(off<<16)\|len`；拒 NUL；dict/dot `kn>8` → `dict key too long` |
| `compiler.ujs` | `DKEY` 256（lit）；dict/dot 键 cap 8；len≥64 LEB |
| 门禁 | accept empty/7/8/32/255/cat + `dict_key8`→2；reject 256/escape/nonascii/NUL/`dict_key9` |

#### Practice twin：tinyvm（✓ 可选 module validate）

| 件 | 说明 |
|---|---|
| 角色 | 伴生 `\0asm` 解释器；**校验面** only——证明模块通过该校验器，**不是**语义执行孪生；不是第三 IR |
| 门禁 | `ujs2wasm_compiler.sh`：若找到 `tinyvm`，对 fold + sim/drone 做 `module validate`；`UJS_REQUIRE_TINYVM=1` 时缺二进制 = FAIL |
| 查找 | `PATH` · `../tinyvm/target/{release,debug}/tinyvm` · `$HOME/repos/tinyvm/...` |

**下一刀**：v18 收尾后，**只选一个**由应用推动的缺口（优先 `fn`），写清输入与完成判据；不同时开 tinyvm 执行层。

#### M3 v15（✓ else-if · fold corpus on core）

| 件 | 说明 |
|---|---|
| `compiler_min.c` / `compiler.ujs` | v15：`else if (...)` 复用 if 路径（C：`pstmt`；ujs：CST=4 + 收尾 IF2）；`else {` 不变 |
| 门禁 | fold 含 **elseif**；stage2≡stage1 · sim+drone body≡stage0；setidx_globals 默认 core |
| 回落 stage0 | `UJS_COMPILER=wasm` 或 `UJS_REQUIRE_COMPILER_WASM=1`（body≡ 对照 / 重建 core） |

#### M3 v14（✓ unary minus · fold corpus on core）

| 件 | 说明 |
|---|---|
| `compiler.ujs` | v14：`-x` / `-(e)` / `--x` → OP_NEG（i64.const -1;mul）或 OP_FNEG（f64.neg）；字面量 `-N` 仍走原 lex |
| 门禁 | fold 含 **unary_minus**；stage2≡stage1 · sim+drone body≡stage0；setidx_globals 默认 core |
| 回落 stage0 | `UJS_COMPILER=wasm` 或 `UJS_REQUIRE_COMPILER_WASM=1`（body≡ 对照 / 重建 core） |

#### M3 v13（✓ `[…]` + `dict.field` · fold corpus on core）

| 件 | 说明 |
|---|---|
| `compiler.ujs` | v13：`[e,…]` → MKLIST/BOX/LSET；`.` → SCONST+DOT；`CST[slot]` 区分 dict/f64-list |
| 门禁 | fold 子集经 **compiler_core**；stage2≡stage1 · sim+drone body≡stage0；**setidx_globals 默认 core** |
| 回落 stage0 | `UJS_COMPILER=wasm` 或 `UJS_REQUIRE_COMPILER_WASM=1`（body≡ 对照 / 重建 core） |
| splice | `rebuild-main.mjs` 把 stub 的 `host_set/get_global` 边界从 ng=1 补到 MAXG=64（否则 inject 只写得进 idx0） |

#### M3 v12（✓ core→meta · default compile=core）

| 件 | 说明 |
|---|---|
| `compiler.ujs` | v12：`OUT` 尾部名表 + `MG`；供 host 写 `.meta.json` |
| `compiler_core.wasm` | stage1 入树；`compile.mjs` **默认** |
| ship | asteroid+drone bridge=`compiler_core.wasm` |

#### M2（✓）

**已交付**

| 件 | 路径 | 说明 |
|---|---|---|
| `compiler.wasm` | `ujs/core/compiler.wasm` | C 子集编译器（`native/compiler_min.c`）→ zig 一次性构建 |
| 重建 | `./ujs/scripts/build-compiler-wasm.sh` | 需 zig；**不是**运行时依赖 |
| 宿主 | `ujs/compile.mjs` | 有 artifact → wasm（`UJS_REQUIRE_COMPILER_WASM=1` 禁止回落 Python） |
| ship | `build-{asteroid,drone}-pages.mjs` | `compile.mjs` → `sim.wasm`+meta；**无 python3 emit** |
| 门禁 | `./tests/ujs2wasm_compiler.sh` | PATH 无 python3 · fold · setidx_globals · sim · **ship builders 无 emit_wasm** |

覆盖：`let` / `while` / `if`/`else` / `===` / `return` / i64+f64（混算+比较）/ list·`len`·下标·`setidx` / dict·dot / globals inject / `host_*`·`run_step` / **Asteroid+drone ship Pages**。

**仍非 M2 范围（可后移）**

- 未烘焙 gold/catalog；无通用长 str / fn
- `web-build` / `ujs_full.wasm` 仍一次 Python+zig（引擎 Host，非玩法核 emit）
- 吐出模块与完整 `emit_wasm` 未字节同构

Python 直出管线（对照 / 全量仍用）：

```
.ujs → front + jtape → emit_wat → wat2wasm → \0asm + meta
```

**加速实证（Paper B 床 · 2026-10-25 · 仓外 · 不并表 unisacc 7×18）**

28 语料、NEED5（lex/parse/scope/isel/enc）＋丢 type/shape/ic 死问；字节对拍 b-cli；arith SHA `0b99b103…`。

| 档 | 热语料墙钟 | 做法 |
|---|---|---|
| cli `wat2wasm`×文件 | ≈3.2–3.3s | 每文件一进程 |
| `--jobs=8` 并行 cli | 冷 ≈0.75–0.87s | 只削 spawn 排队 |
| 常驻 wabt.js（`--asm=wabtjs`） | ≈0.09–0.10s | 进程内组装；批量 IPC 无再降 |

热路径拆相（常驻后，约 0.10s）：

| 段 | 墙钟 | 占热 |
|---|---|---|
| front（lex+parse+scope ask＋胶水） | ≈0.02s | ≈20–25% |
| emit_wat（isel+enc ask＋拼 WAT） | ≈0.013s | ≈13–15% |
| 组装（wabt：parseWat 为主） | ≈0.04–0.06s | ≈60% |

四流十网：决策网 ask 合计 ≈0.025s（lex/parse/isel/enc）；type/shape/ic=0；irsel/reloc 直出不问。**encode 流的物理写出不在网里。**

CLI `wat2wasm` 内因：×28 时 spawn／加载 ≈1.9–2.1s（约 2/3）；语料工作增量 ≈0.9s。进程内再拆：parseWat ≈57%、validate ≈16%、toBinary ≈11%。

**REJECT**：跨文件 WAT 缓存（28 互异命中 0）；`lower_wasm.encode_wasm`→`UJS1` 非浏览器 `\0asm`；大 JSON 批量 IPC（不降或更慢）；本刀回灌 unisacc（产品直写映像、无 fork 组装）。

**下一刀唯一明显杠杆**：真直出 `\0asm`（不经文本 WAT），且须逐字节同基线。其余为毫秒级。范式：**先按流／相拆墙钟，先打 spawn／IO，再动网。** 证据 `/tmp/ujs-accel-link-20261025/`（C-batch／J-PHASE／ASM-*／O-wabt*）。未 tip、未改出货默认路径。

**`compiler.wasm` ABI**（已钉）：

```
// JS — 产品契约
compile(src: Uint8Array|string) →
  { wasm: Uint8Array, meta: { locals:string[], globals:string[], slots:string[] } }
  | throw Error(message)

// wasm 导出
// memory; alloc(n)→ptr; compile(src_ptr, src_len)→i32 status
// status=0: out_ptr/out_len + meta_ptr/meta_len（UTF-8 JSON）
// status≠0: err_ptr/err_len
```

| # | 做什么 | 完成判据 / 测试 | 状态 |
|---|---|---|---|
| **0** | `emit_wasm`：`setidx` · `f64` · game-ready | `./tests/ujs2wasm.sh` | ✓ |
| **2a** | Asteroid ship-js（砍 `uxe_*.c` 出货） | `./tests/uxe_ship_js.sh` · `npm run ship:engine` | ✓ |
| **2b / M1** | Asteroid 步进默认 `sim.wasm`（path B） | `ujs2wasm_step` + `uxe_ship_js`（含 sim.wasm） | **✓** |
| **M1b** | drone 对称切 B（`ship/drone/sim.wasm`） | `uxe_ship_js`（drone sim.wasm + directSim） | **✓** |
| **M2** | `compiler.wasm` + 无 python ship | `ujs2wasm_compiler.sh`；ship 无 `python3` emit | **✓** |
| **M3** | `compiler.ujs` 自举 | body≡stage0 → stage2≡stage1 | **v16✓** + fold logic/elseif/… on core |
| **P0** | ship 无 web-build / 无 `engine.wasm`（A） | `ujs2wasm_compiler.sh` · `ship-engine` 拒 A 核 | **✓** |
| **3** | 换核模板 | 按下表抄路径绿 | 骨架 ✓ |
| **4** | 门禁即契约 | **host1**：`ujs/tests/run.cx`。实践脊／UXE 另门；旧 `ujs.sh` 见 `archive/testing-c43-legacy/` | host1 套件 ✓ · UXE 另门 |
| **5–6** | Host / llm | 有玩法再开 | 搁置 |

### 换核模板（#3 可抄）

```
1. 写 ujs/web/game/{name}.ujs          # 只算状态，不画
2. 写 ujs/uxe/app-{name}.js            # 只调 host_* · wasm_run/预编译
3. 写 ujs/uxe/ship/{name}-host-entry.js + build-{name}-pages.mjs
4. package.json 加 ship:{name}；scripts/ship-pages.sh 挂上
5. ./tests/uxe_ship_js.sh 扩展检查 · npm run test:uxe:all
```

对照实现：`asteroid-host-entry.js` · `build-asteroid-pages.mjs` · `drone-host-entry.js` · `build-drone-pages.mjs`。

**已交付**：Host ABI v0 · UXEP/UXIN · WebGL/WebGPU · Pages（Asteroid+无人机 ship-js）· `test:uxe:all` · H1–H3 · snapshot · **ujs2wasm 直出（setidx/f64 + game-ready）** · 双 ship-js 无 C 游戏核。

**门禁（必绿）**

| 门 | 命令 |
|---|---|
| **`ujs.com` host1** | `./unisacc.com -run ujs/tests/run.cx -- --smoke`；发版材料另跑 `--complete` |
| **ship-js 合同**（实践脊） | 旧 `uxe_ship_js`／Pages 另门（脚本或在 archive；不挡 host1） |
| UXE 无人 | `cd ujs && npm run test:uxe:all`（另门） |
| Pages | `npm run ship:pages` 后抽玩感 |
| 旧语言／全表 | `archive/testing-c43-legacy/tests/ujs*.sh`（考古；非 host1 合同） |

**工具（摘）**：`python3 -m ujs web-build` · `ujs2wasm` · `./ujs/scripts/release-artifacts.sh` · `ship-pages.sh` · `uxe-gate.sh`。  
指纹：`core/BUILD.json`。二进制走 GitHub Release，不进主树。

**代理观测**：断言写在数上。`__UXE__` · `__UXE_HOST__` · `host_debug_snapshot` / `__UXE_SNAP__` · `test:uxe:snap`。

---

# 一、思维树（markdown-tree-dag）

> 开工用。约定：`├─` 包含（树），`══>` 跨树依赖（DAG），`⟦门禁⟧` 检查点。

## T1. 系统树 —— 三层切分

任何一行代码必须能归到其中一类，归不了就是走偏。

```
UJS
│
├─ ① 经典代码（algebra）—— 绝不「训练填表」
│   ├─ 前端走查          construct/front/{lex,compile}
│   ├─ jtape / VM        jtape.py · vm.py · bc_vm.py
│   ├─ UXE Host 胶水     host-browser · packet · input · renderer-*
│   ├─ C VM               native/ujs_vm.c（路径 A）；uxe_asteroid.c 遗留不出货
│   └─ 页壳 / ship-js     demo/ · ship/{asteroid,drone} · engine.js
│
├─ ② 模型数据（constructed tables）—— 离散 key → 类
│   ├─ gold 表           catalog → gold 笛卡尔积
│   ├─ IntNet 权重       构造代数（非 SGD 出货）
│   └─ isel / enc …     ujs2wasm 决策表（WAT 形态）
│
└─ ③ 出货面（artifacts）
    ├─ core/ujs_full.wasm (+ compiler.gen.js)   路径 A（非 P0 ship）
    ├─ core/compiler_core.wasm                 M3 出货编译器（默认 bridge）
    ├─ ship/*/sim.wasm                         路径 B 玩法核
    └─ docs/uxe/{engine.js,{game}/}             Pages（无 engine.wasm 必经）
```

**DAG 边**

```
gold ══构造══> 权重 / isel·enc          ← 构造脊
② ══驱动══> ① 走查 / emit（Oracle 只回类名）
① ══产出══> ③ path-A / 全表对照
compiler.ujs ══M3══> compiler_core ══> sim.wasm   ← 出货脊
A (ujs_full) ══Host══> demo（可选）
B (sim.wasm) ══directSim══> Pages
```

## T2. 目录树 —— 文件到职责

```
ujs/                         ★ 树内无 .c／.py；套件在本树
├─ prd.md · README.md · ARCHITECTURE.md · plans/
├─ tests/                    ★ host1 产品套件（不在根 tests/）
│   ├─ run.cx · host1-pass/refuse · host1-run-expect
│   └─ corpus/ · neg/        语料夹具（自根 tests/ujs2wasm 迁入）
├─ product/                  ★ host image（unisacc.com + .cx）
│   ├─ ujs_main.cx · host1_vm.cx · build*.cx · ape-pack.cx · complete.cx
│   ├─ corpus-host.cx · run-host.cx · verify-host.cx · wasm-verify.cx
│   └─ --verify／--run       \\0asm 验面／迷你 VM 跑面
├─ seed/stage0/              host1 编译器（.cx）
├─ seed/construct|vm/        占位 README；源在 archive/ujs-legacy-py-c/
├─ iterate/                  M3 实践脊（另标签）
├─ practice/ · weights/      实验／表位（无 .c／.py）
├─ ujs.com                   本机构建产物
└─ archive/                  旧规格文稿（非 legacy 源）

仓外：archive/ujs-legacy-py-c/   0.0.3 迁出的 .py／.c 考古
```

**易错 DAG**

```
web-build ══> core/ujs_full.wasm     （A-demo / FORCE_WEB_BUILD；非 ship）
ship B ══> compiler_core → sim.wasm  （无 engine.wasm / 无 python3 emit）
{app} ══只调══> host_*          （不知 canvas / fetch / key）
UXEP ══一次一包══> host_gpu_submit
UXIN ══> host_input_read
ujs/tests/run.cx ══调══> ujs/product/{build,corpus-host,complete}.cx
```

## T3. 出货 DAG —— host image vs 实践脊

```
.ujs 源
 │
 ├──── host image（ujs.com · host1_package）───────────┐
 │    stage0 → \\0asm；APE／六目标                       │
 │    ⟦门禁⟧ ujs/tests/run.cx · host1-pass/refuse       │
 │                                                     │
 ├──── 实践脊 M3（m3_scaffold · 另门）─────────────────┤
 │    compile.mjs → compiler_core → sim.wasm           │
 │    旧门禁已归档；不挡 host1 版号                      │
 │                                                     │
 └──── 方法弧（method_construct · 另弧）───────────────┤
      construct／m4；≠ host1 绿                        │
                                                       ▼
                              Pages／UXE 另门（npm test:uxe）
```

## T4. UXE 积木树

```
UXE Shell
├─ {app|game} 核     调度 · 组 UXEP · 读 UXIN · 只调 host_*
├─ sim.wasm          出货玩法核（path B · UJS-1_ship · core 编）
├─ ujs_full（可选）  path-A 演示 VM；**非** ship 必经（曾名 engine.wasm）
├─ Host（engine.js） GPU / 输入 / 时间 / 资源 /（规划）llm
└─ Pages             docs/uxe/engine.js + docs/uxe/{name}/
```

```
demo：*.ujs → compile(ujs_full) 或页内测 → host_*
ship：index.html → engine.js + game.js + sim.wasm（directSim；无 engine.wasm）
```

## T5. 验收 DAG

```
host1 绿 ── ujs/tests/run.cx ──┬── --smoke（build + corpus/verify + run）
                               └── --complete（APE + oracle + six）

实践另门 ── M3／uxe（归档脚本或 npm）；不挡 0.0.x 版号
历史对照 ── archive/testing-c43-legacy/tests/ujs*.sh

UXE 绿 ── test:uxe:all ────┬── CDP 探针 · snapshot 数（**另门**）
                           └── 禁代理猜 UI；不挡语言/出货绿

出货 ──── ship:pages ─────── docs/ 与 BUILD 指纹对齐 Release
```

## T6. 三条红线

```
① 双 wasm 永不合包（若仍跑 path-A） → {app} ≠ ujs_full
② 核只调 host_*                      → 无 canvas / key / fetch
③ 出货权重构造、不训练填表            → --drive 默认 built；勿擅跑 train
   + 出货脊 ≠ IntNet；UJS-1_ship ≠ 全表 UJS-1
```

---

# 二、记忆宫殿（mermaid-flowchart-memory-palace）

> 记全局用。按 **门厅 → 书库 → 工坊 → 熔炉 → 展厅 → 哨所 → 装船口** 走一圈。

## 宫殿全景

```mermaid
flowchart TD
    subgraph R1["门厅 · 命题"]
        A1["主产物：ujs.com（APE host1 ✓）"]
        A2["工具链：unisacc.com + .cx；无产品链 .c/.py"]
        A3["实践脊：core / UXE / construct（非冒充 .com）"]
        A1 --> A2 --> A3
    end

    subgraph R2["书库 · 语言与契约"]
        B1["闭合：表内全功能 / 表外永久拒绝"]
        B2["HOST_ABI：host_* · UXEP · UXIN"]
        B3["核不知宿主；GPU/密钥停在 Host"]
        B1 --> B2 --> B3
    end

    subgraph R3["工坊 · 构造"]
        C1["catalog → gold 笛卡尔积"]
        C2["Oracle：ask → 类名；不读 logits"]
        C3["web-build → core/ · ujs2wasm → \\0asm"]
        C1 --> C2 --> C3
    end

    subgraph R4["熔炉 · UXE 运行时"]
        D1["{app} 核 ══host_*══> Host"]
        D2["ship: sim.wasm · demo: ujs_full"]
        D3["一次一包 UXEP · UXIN 读输入"]
        D1 --> D2 --> D3
    end

    R1 --> R2 --> R3 --> R4

    classDef room fill:#1f2937,stroke:#6366f1,color:#e5e7eb
    class A1,A2,A3,B1,B2,B3,C1,C2,C3,D1,D2,D3 room
```

```mermaid
flowchart TD
    subgraph R5["展厅 · Pages"]
        E1["docs/uxe/engine.js 共享"]
        E2["docs/uxe/asteroid · drone"]
        E3["索引 docs/index.html"]
        E1 --> E2 --> E3
    end

    subgraph R6["哨所 · 观测与门禁"]
        F1["__UXE_SNAP__ 一帧 JSON"]
        F2["分层：ujs/tests/run.cx · UXE/Pages另门 · 旧脚本归档"]
        F3["截图非默认；人只审手感"]
        F1 --> F2 --> F3
    end

    subgraph R7["装船口 · 发版"]
        G1["release-artifacts → GitHub Release"]
        G2["BUILD.json 指纹对齐"]
        G3["ship:pages → docs/"]
        G1 --> G2 --> G3
    end

    R5 --> R6 --> R7

    classDef room fill:#1f2937,stroke:#f59e0b,color:#e5e7eb
    class E1,E2,E3,F1,F2,F3,G1,G2,G3 room
```

## 管线真图

```mermaid
flowchart LR
    SRC[".ujs"] --> FRONT["front / 或 M3 compiler"]
    FRONT --> JT["jtape · 构造脊"]
    FRONT --> M3["compiler_core · 出货脊"]
    JT --> VM["path A：ujs_full 演示"]
    M3 --> SIM["path B：sim.wasm"]

    VM -.-> DEMO["demo / FORCE_WEB_BUILD"]
    SIM --> PAGES["Pages · directSim"]
    PAGES --> HOST["Host engine.js"]
    DEMO -.-> HOST
    HOST --> GPU["WebGL | WebGPU"]
    HOST --> IN["UXIN"]
    SNAP["__UXE_SNAP__"] -.门禁.-> GATE["uxe另门"]

    classDef net fill:#312e81,stroke:#818cf8,color:#e0e7ff
    classDef cls fill:#064e3b,stroke:#34d399,color:#d1fae5
    class FRONT,JT,M3 net
    class VM,SIM,HOST,GPU,GATE cls
```

**读图**：紫 = 决策/编译路径（构造脊或 M3）；绿 = Host / Pages。出货主杠杆是 **sim.wasm + core**，不是 `engine.wasm`。

## 门禁状态机

```mermaid
stateDiagram-v2
    [*] --> Lang
    Host: ujs/tests/run.cx（ujs.com host1）
    Practice: M3/core · uxe_ship_js（另门）
    Uxe: test:uxe:all（另门）
    Ship: ship:pages
    Feel: 人审手感

    Host --> Feel: host1 绿即可推进产品
    Practice --> Ship: Pages 实践脊
    Ship --> Feel: docs 可玩
    Feel --> [*]
    Ship --> Uxe: UXE 另验，不挡 host1

    Host --> Host: 卡住改 stage0/.cx；勿训练填表；勿把 M3 当 IntNet
```

## 目标推进图

```mermaid
flowchart TD
    U2W["ujs2wasm 语料"] --> E0["0 setidx+f64+game-ready ✓"]
    E0 --> SJS["2a ship-js 砍 C ✓"]
    SJS --> M1["M1/M1b sim.wasm ✓"]
    M1 --> M2["M2 compiler.wasm ✓"]
    M2 --> M3["M3 core 自举 v16 ✓"]
    M3 --> P0["P0 无 A 核 ship ✓"]
    P0 --> HOST["ujs.com APE host1 ✓"]
    HOST --> FN["stage0 arrow/fn+rest ✓"]
    FN --> NEXT["可选：嵌 core · P1 construct"]

    classDef now fill:#064e3b,stroke:#34d399,color:#d1fae5
    classDef next fill:#1e3a5f,stroke:#60a5fa,color:#e0f2fe
    class E0,SJS,M1,M2,M3,P0,HOST,FN now
    class NEXT next
```

## 随身卡

| 记 | 是什么 |
|---|---|
| **两脊** | 构造/jtape（IntNet）∥ M3 出货（`compiler_core`，非 IntNet） |
| **A / B** | path-A `ujs_full` 演示 / path-B `sim.wasm` 出货步进 |
| **UJS-1_ship** | M3 已交付子集 ⊂ 全表 UJS-1 |
| **门禁** | `ujs/tests/run.cx`（smoke／complete）· UXE／Pages 另门 · 旧 `ujs.sh` 已归档 |
| **host_*** | 核唯一出入口；GPU/key 停 Host |
| **UXEP / UXIN** | 一次一包提交 / 输入快照 |
| **BUILD.json** | 与 Release 对指纹 |
| **构造不训练** | 出货权重由 gold 代数生成；M3 另用孪生字节门禁 |
| **ujs.com** | **ujs+wasm 生产力工具**；版号见 §0.0／`plans/` |
| **Paper B** | 底层理论对齐该工具；谱系 B→A 派生，A／unisacc 佐证 |
| **计划** | `ujs/plans/v0.0.N.md`；根仓 0.0.4x 是 unisacc 底座 |
| **子功能** | `ujs2wasm`／`compile.mjs`／`sim.wasm`… = 应收进壳的实践脊 |
| **脱 Py** | 新胶水／构造迁 **`unisacc.com` + `.cx`（`-run`）**；只许缩 Python，不许转调冒充 |
| **直出税** | 热语料大头曾是 `wat2wasm` spawn，非常驻后是 parseWat；网 ask 非主税。见「加速实证」 |

## 走出宫殿前默念

```mermaid
flowchart LR
    L1["① ship 无 A 核"] --> R1["sim.wasm · 非 engine.wasm"]
    L2["② 核只调 host_*"] --> R2["无 canvas/key"]
    L3["③ 两脊分清"] --> R3["M3 ≠ IntNet"]
    L4["④ 先拆相再砍网"] --> R4["spawn／IO／parse 税 ≠ IntNet"]

    classDef red fill:#7f1d1d,stroke:#f87171,color:#fee2e2
    class L1,L2,L3,L4 red
```

---

## 文档角色（收束后）

| 文档 | 角色 |
|---|---|
| **本文件** | 规格 · 目标 · 思维树 · 记忆宫殿 · **当前版号指针** |
| [`plans/`](plans/) | **`ujs.com` 0.0.x 版本计划**（范围／判据／进度） |
| [`uxe/HOST_ABI.md`](uxe/HOST_ABI.md) | Host 符号 / UXEP·UXIN |
| [`README.md`](README.md) | 开箱与分发 |
| [`product/README.md`](product/README.md) · [`tests/README.md`](tests/README.md) | 产品壳 · 套件入口 |
| [`archive/`](archive/) | 历史；不当真源 |

升版：按 `plans/v0.0.N.md` 完成判据 bump `ujs.com` 版本串；破坏性语言/API 另记；旧活规格摘入 `archive/`。
