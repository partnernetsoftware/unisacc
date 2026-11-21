# RenderTarget 二维目标

`render-target.ujs` 提供单颜色 RGBA8 二维目标及可选 16 位深度缓冲。调用方为对象保留记录，为生命周期操作保留独立数据包和描述符，并在同一 GPU 代内分配互异的 ID。`RenderTarget_prepareDescriptor` 和 `RenderTarget_create` 创建实际帧缓冲附件。`RenderTarget_bind` 向绘制包追加绑定、视口和裁剪状态。

`RenderTarget_setSize` 在尺寸变化时先绑定调用方指定的恢复目标，再分配新附件，最后释放旧附件。调用方须提供空闲的新 ID，并为新旧附件同时存活留出预算。分配失败时记录与旧附件保持有效。相同尺寸只重置视口和裁剪框，保留附件像素及裁剪开关，与 r160 一致。

`RenderTarget_clone` 复制尺寸、深度配置、视口和裁剪，并分配独立空附件。`RenderTarget_copy` 将这些字段复制到已初始化的目标，分配独立空附件并释放目的目标的旧附件。复制保留目的对象的观察器，不发送公共释放事件。`RenderTarget_dispose` 解绑后提交实际资源释放。所有权变化以宿主同步提交成功为准。上下文换代后，过期记录须由调用方依现有 GPU 恢复规则重建。

`RenderTarget_watchDispose` 绑定调用方的 `RenderTargetDisposeObserver`。它指向现有 EventDispatcher 注册表，保存事件令牌、稳定对象身份、快照输出、监听器容量和序号。尺寸确实变化或显式释放成功后，快照包含稳定对象身份和匹配的监听器令牌，由调用方分派。容量预检失败、同尺寸重设和复制均不更新序号。克隆不继承注册表。编译时加入 `eventdispatcher.ujs` 及 `object-layouts.ujs` 中的 `EventDispatcher` 声明。独立验收原样提取该声明，使输入满足现有 24 布局上限。观察器、注册表和输出区须与对象、数据包及描述符互相分离。

当前 GPU 数据包只描述固定 RGBA8 纹理、固定过滤和可选深度缓冲。高级纹理格式、独立深度纹理、模板和多重采样仍待支持。公开释放会退役 GPU 记录，下次使用需重新创建；重复释放返回错误。

`sh ./unisacc.com ujs/tests/render-target-pixel.cx --` 比较实际 Wasm 宿主附件与 r160 WebGLRenderTarget 的像素和公共状态。在命令末尾添加 `--mutate` 会偏移裁剪区域，添加 `--mutate-copy` 会错误执行自复制。两个负例均由金值拒绝。
