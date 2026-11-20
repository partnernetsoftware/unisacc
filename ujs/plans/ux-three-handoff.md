# UXE 对标交接

## 当前状态

合同、分母与里程碑见 [UXE 对标规划](../prd_uxe_three.md)。软件与 GPU 两条渲染路径共用一个基座 Wasm 模块。产品是第二个 Wasm 模块。宿主桥只转发 ABI，GPU 预算 600 行，软件预算 300 行。线上 D1、D2、D3 已使用两模块软件路径。D4 迁移排在 M7 后，D5 等 M12 验收。

| 里程碑 | 已有证据与待办 |
| --- | --- |
| M0 | C1 至 C14 的编译器基础已落库。GPU 导入有 ABI 门。新工作复现局部槽不足时先用有显式签名的原生函数隔离各作用域，既有 256 槽规格保持 |
| M1 | 数学基础有 .cx 金值与 r160 对照。对象边界及视锥对象接口另有浏览器联接门 |
| M2 | 对象与缓冲布局、固定对象树、事件、层、缓存等有结构门。完整的对象生命周期、序列化以及部分缓冲和图像格式依清单逐项补齐 |
| M3 | 相机、几何生成器、曲线、路径与形状有主干门。派生类 JSON 往返、复杂曲线与特殊几何继续按清单的证据缺口验收 |
| M4 | 排序、视锥剔除、状态、裁剪、五类目标和读回已验。私有 GPU 目标实验有 600 行预算，正式导出闭包仍须审阅登记。真实上下文恢复移到 M12 |
| M5 | 纯色 Basic、单方向平面 Lambert、Phong、Standard，线性与 Reinhard 色调有 r160 像素门。UXEP 操作 13 和 14 已对照二维与六面纹理。Toon 的 WebGL1 黑色已定位为参考着色器导数扩展问题，显式梯度与 r160 WebGL2 对照通过，默认导数过渡待补 |
| M6 至 M12 | 按规划的独立验收出口继续。M7 前保持 D4 发布包；M12 用真时钟检验上下文恢复与资源代数 |

M5 增补 Normal、Depth、Matcap、Distance、Points、三种直线、Sprite、ShaderMaterial、RawShaderMaterial 和 ShadowMaterial。点线使用 UXEP 操作 15。ShadowMaterial 已测矩形投影的透明输出，通用阴影图仍归 M6。Toon 已复跑 WebGL2 正例和篡改，仍为部分。Cineon、ACESFilmic、AgX 与自定义增益通过 r160 对照。主门 1682 项，输出色空间分门 `--colors` 123 项。Display P3 的 ColorManagement 数学转换通过，画布编码仍有缺口。

Display P3 画布已两次复现。r160 WebGL1 与每色空间独立的 WebGL2 上下文均有 4 个通道不符，光栅中心红、蓝分别读回 117、14，独立色域与传递函数金值为 119、25。日志为 `/tmp/devsvc-flow-21559245761-324647/out/x.out` 与 `/tmp/devsvc-flow-21559316131-338155/out/x.out`。单独保留 `--p3-readback` 的原断言，下一步隔离浏览器绘图缓冲色彩管理和 WebGLProgram 的输出编码。其他三种经 `--output-mutate` 验证 18 项转红。

M6 已对照 AmbientLight、HemisphereLight、SpotLight、LightProbe、Fog 与 FogExp2。入口为 `gpu-packet-struct.cx` 的 `--ambient`、`--hemisphere`、`--spot`、`--light-probe`、`--fog`、`--fog-exp2`，每门 41 项，均有同名的 `-mutate` 反例。方向光和点光沿用材质主门的双方向灯球体和点灯立方体，反例分别为 `--phong-light-mutate` 与 `--lighting-mutate`。

M6 未完成的是 RectAreaLight 和四类通用阴影图。面积光需接入 LTC 取样，并用同一宽高与朝向对照 r160。阴影图从 `gpu-target.ujs` 的产生和读回接到 `gpu-packet.ujs` 的纹理输入。验收应同时取遮挡内、无影区与边缘样点，再分别比较 Basic、PCF、PCFSoft、VSM。M5 透明收影测试只覆盖矩形投影，清单未把它算作 BasicShadowMap 能力。

[清单事实表](../practice/uxe/three-r160-annotations.tsv) 每一行包含该项的覆盖范围与尚缺方法族，含 M1 至 M5 所有已登记的部分项。[生成清单](../practice/uxe/three-inventory.tsv) 可用 `rg -n '部分' ujs/practice/uxe/three-inventory.tsv` 按项查看。A 栏分母 214、已有 29，B 栏分母 200、已有 20。有限场景通过的类继续标部分。

## 直接续作

(1) M5 纹理生命周期。`gpu-texture-table.ujs` 已验证 Wasm 固定容量、句柄代数、更新版本、释放与千次回收，纹理描述与像素地址由调用者提供，绘制时复制进包。WebGL 纹理随绘制创建释放，下一步补持久 GPU 句柄、失效与回收，以及颜色空间和像素存储状态。CubeTexture 六面、DataTexture 和 CanvasTexture 的基础像素已有 r160 对照。面内方向、其他数据格式和 Canvas 字节取回仍待补。Texture 矩阵复用 `Matrix3_setUvTransform`。增加宿主能力时更新 `uxe-host-budget.cx` 中审阅后的闭包摘要。

(2) M5 Phong 和 Standard。已在同一正交相机对照球体双方向灯与立方体点灯，使用几何生成器的法线算逐片元光照。`gpu-lit.frag` 用固定的对照材料和光源，下一步把参数接入材质与场景句柄，覆盖金属、贴图、阴影及环境光。

(3) 并行窗口建议从稳定尖端分开工作树。M8 可先做插值器和时间采样，沿现有向量与原生导出接口写 G-anim；M9 先做有界文件字节及 GLB 容器解析，沿现有资源 ABI 写 G-io；M10 可先做由 M1 的 Ray 与 M3 相机矩阵得到射线并排序的 G-ray；M11 可复用基座 PCM 做有界混音及空间衰减对照。场景树、GPU 命令包和清单生成表的写入在集成时串行。各窗口新增的事实行保持独立，重算生成表后一起核对。

## 复跑

在仓内 ujs 工作树运行。忽略的 `ujs/ujs.com` 由 `sh ./unisacc.com ujs/product/build-ape.cx` 构造，依赖它的门会重新编译 .ujs。r160 对照使用仓内 `ujs/practice/web/game/three.module.js`，对照和夹具留在 ujs。

```sh
uptime
sh ./unisacc.com tools/devsvc.cx -- x --par 1 sh ./unisacc.com ujs/tests/gpu-packet-struct.cx
sh ./unisacc.com tools/devsvc.cx -- x --par 1 sh ./unisacc.com ujs/tests/gpu-packet-struct.cx -- --sampled-mutate
sh ./unisacc.com tools/devsvc.cx -- x --par 1 sh ./unisacc.com ujs/tests/gpu-packet-struct.cx -- --targets
sh ./unisacc.com ujs/tests/uxe-host-budget.cx -- --gpu-fixture
sh ./unisacc.com ujs/tools-ux/gen-three-inventory.cx -- --check
sh ./unisacc.com ujs/tools-ux/check-three-inventory.cx
sh ./unisacc.com tools/gen-uxe-pages.cx -- --check
```

纹理采样反例预期非零。一次只跑一路 Chrome。向主线提合入前先合最新 origin/main，再跑 M1 至 M3 全部门、C9 至 C14、CLI、清单、UXE 回归十项、页面生成和 doc-links。集成路径只含 ujs 与重算后的 build/doc-links.tsv。

交接时无待提交文件。回归入口加 `-- --wrap-mutate` 可复跑环绕反例，加 `-- --phong-light-mutate` 或 `-- --lighting-mutate` 分别复跑 Phong、Standard 的光源反例。
