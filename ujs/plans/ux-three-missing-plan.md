# UXE 缺项实施计划

| 栏 | 甲 可复用入口 | 乙 元数据验收 | 丙 有前置缺口 | 合计 |
| --- | --- | --- | --- | --- |
| B | 14 | 1 | 66 | 81 |
| A | 6 | 0 | 16 | 22 |

B 栏甲原有 90 项。混合因子 15 项、混合方程 5 项、深度函数 8 项、模板函数 8 项、模板操作 8 项、面剔除 4 项、混合模式 6 项、材质面 3 项、纹理环绕 3 项与纹理过滤 10 项已通过 WebGL2、r160 与独立金值的验收。轨道插值与边界常量 6 项已通过 r160 的轨道与首尾区间求值对照，现剩 14 项。B 栏甲按四波推进。纹理环绕与过滤对照带边缘和 mip 层的纹理。混合、深度、模板与剔除用重叠几何，分别读像素、深度或模板。动画常量读同一轨道在指定时刻的值。格式、色域与投影分别读纹素、色值及裁剪坐标。现有 API 可提供入口，仍要逐项核对公开常量与调用参数的映射。

乙仅有 REVISION。核验正向读回 r160 字符串，不靠拒绝来升格。丙含 32 项压缩纹理格式，权限与扩展可用性、真实格式上传和采样都要有证据。其余 34 项依赖软件语义，例如深度纹理比较、整数通道、环境映射与着色语言版本。先实现缺失语义再验，现阶段保留缺项。

A 栏甲为 RenderTarget、Interpolant、Light、Material、DepthTexture 和 FramebufferTexture。按公共对象的创建、读写、复制及释放逐类对照。丙包含三个压缩纹理类、两个浏览器宿主入口，以及 11 个软件语义缺口。A 栏没有仅凭接受或具名拒绝即可升格的项。两栏丙类共 82 项，其中压缩格式及压缩纹理类 35 项、浏览器宿主 2 项、软件语义缺口 45 项。软件缺口按真实功能排入后续里程碑。VideoTexture 依赖宿主媒体解码及帧时序，createCanvasElement 依赖浏览器 DOM 对象身份。两者仅列为候选扣除，清单状态及分母仍按现行口径。

## 甲类初始事实

`ux-three-b-semantic-facts.tsv` 写明纹理、材质预设、面模式和插值的逐项映射。已有的 equation、factor、depth、stencil、op、face 事实行复用 `ujs/practice/uxe/base/gpu-state-constants.tsv`，生成器按甲类名字相交，排除 depth packing 与 compare 等丙类项。颜色、纹理格式和投影的公开值与现有入口也已列在该表。两份来源覆盖初始 90 个 B 栏甲类名字，含已验收的混合因子。状态字段尚无直接映射的项先实现，再做语义验收。A 栏六个类的类级期望见 `ux-three-a-semantic-facts.tsv`。

| 家族 | 字段与公开值来源 | .cx 独立算法 |
| --- | --- | --- |
| 混合方程与因子 | `equationRgb/Alpha`、`sourceRgb/Alpha`、`destinationRgb/Alpha`；公开值从状态事实读取 | 源 RGBA 取 0.2/0.6/0.4/0.25，目标取 0.7/0.3/0.5/0.75，常量取 0.35/0.65/0.15/0.5。分量分别乘因子，再做加、正减、反减，最后钳到 0 至 1；MIN 与 MAX 直接用未乘因子的源目标。将一侧因子设 ONE 或 ZERO，确保被测因子可区别。 |
| 深度和模板比较 | `depth`、`stencil`、`stencilReference`、`stencilReadMask`；公开值从状态事实读取 | 用 1/4、1/2、3/4 对预写 1/2 做三值真值表。模板比 `ref & readMask` 与 `stored & readMask`；为三种比较结果读颜色哨兵。 |
| 模板操作 | `stencilFail`、`stencilDepthFail`、`stencilPass`；分别命中各分支 | 起值 0、127、255。增长与缩减分别测钳位和模 256，取反是 255 减起值，替换取参考值，保留取起值。写入时保留写掩码外的位。 |
| 剔除和面模式 | `cull`、`clockwise`；CullFace 四值复用状态事实 | 在不同屏区画一正一反三角形，按二维有向面积符号以及 frontFace 反转选择可见屏区，读两处哨兵像素。 |
| 环绕及滤波 | `GpuTextureSource.wrapS/wrapT/filter`；公开值与别名来自甲类事实 | 对 UV -0.375、0.375、1.375 分别做钳位、周期小数和奇周期反折；Nearest 取 floor(u*w)，Linear 在 u*w-0.5 两侧按小数权重。mip 固定 0.25 与 0.75 两个 LOD，区分最近层与层间加权；两级纹理配不同颜色。 |
| 轨道插值与结尾 | `KeyframeTrack.interpolation`、`Interpolant.settings` | 轨道用四个不同值和非等距时刻，独立查区间；离散取左值，线性按时间比例，三次按 Hermite 系数并分别代入自然、零斜率和环绕端导数。 |
| 颜色与旧编码 | r160 读取 NoColorSpace、NoToneMapping、LinearTransfer、SRGBTransfer、P3Primaries、Rec709Primaries、LinearEncoding、sRGBEncoding；走颜色输出或转换 | 以 0.0031308 为线性至 sRGB 阈值，两侧各取一个值验分段函数；用三原色和中灰验 P3 与 Rec709 矩阵与无转换分支。旧编码须走实际材质入口。 |
| 目标格式与投影 | r160 读取 UVMapping、RGBAFormat、DepthFormat、DepthStencilFormat、TrianglesDrawMode、WebGLCoordinateSystem | 双色 UV 纹理、四通道非对称颜色、深度写入和模板哨兵读回；独立检查由 Near/Far 送到 WebGL NDC -1 至 1。三角形入口输入六顶点，应得到两个独立图元。 |

## 验收生成器设计

事实表每行记录公开名字、r160 数值或字符串、语义标签、正常用法入口、金值规则和反例。别名引用同一语义事实，保留各自公开名称。生成器独立解析仓内固定 r160 的导出，校验成员、数值、别名和重复行后，产出 .ujs 映射、.cx 门夹具及清单事实行。未通过实际验收的条目保持缺或部分。

生成器提供 `--check`，逐字节比对三个产物。篡改自测在独立临时目录更改一项枚举映射、删一项别名或交换两项模式，分别要求生成物漂移或语义门非零退出。金值取自独立 .cx 或 r160 实读，测试不能用被测映射生成期望。硬件门另绑定设备、扩展、驱动与测试图样的身份。缺设备时记录缺口，具名拒绝只作为诊断证据。

缺项计划由 `cx ujs/tools-ux/plan-three-missing.cx -- B --write` 与 `cx ujs/tools-ux/plan-three-missing.cx -- A --write` 重算，改用 `--check` 比对表。语义验收生成器按上面的设计另行实现。
