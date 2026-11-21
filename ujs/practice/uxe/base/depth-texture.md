# DepthTexture 深度纹理

`DepthTexture_create` 连接一个 WebGL2 独立深度附件。宿主由 `createDepthTextureHost(gl, currentGeneration)` 产生。将其 `imports` 绑定到 `uxe_depth_texture_v1`，与同一 WebGL2 上下文的 `uxe_gpu_v1` 分别链接。调用方为对象分配清零的内存记录，为当前 GPU 代提供唯一 ID。

已支持 DepthFormat 的 UnsignedShortType、UnsignedIntType 和 FloatType，以及 DepthStencilFormat 的 UnsignedInt248Type。省略格式时传 0，选择 DepthFormat。省略类型时传 0，分别按 r160 选择 UnsignedIntType 或 UnsignedInt248Type。纹理使用最近过滤、关闭纵向翻转和 mipmap，对比函数为空。

`DepthTexture_bind` 将深度附件与全尺寸视口绑定到当前上下文。沿用普通 GpuPacket 清屏、深度状态和图元绘制。绘制时避免普通帧缓冲绑定命令替换该深度目标。读取请求通过实际采样着色器将指定坐标转换成 0 到 255 的归一化深度。深度和模板的附件精度分别由 WebGL2 格式控制。本入口读取深度通道。

`DepthTexture_setSize` 先分配新附件，成功后切换绑定并释放旧 WebGL 纹理和帧缓冲。尺寸相同则保留附件内容。显式释放清理附件并退役对象的 ID 和代号，尺寸及格式仍可读。调用方可传现有 EventDispatcher 和调用方拥有的快照缓冲接收释放事件。快照容量不足或跨度重叠时，附件仍然有效。旧代号由宿主拒绝。

适用范围为 WebGL2 独立深度目标、归一化深度通道采样和附件所有权管理。独立宿主目前内置固定采样 GLSL，经 texelFetch 读取指定深度并将结果写入 RGBA8 以供读回。这个着色器处于验证切片，后续需迁入 UXE 基座，由宿主执行基座提供的采样命令。当前证据只对应此切片的功能资格。

`cx -I ujs/tests ujs/tests/depth-texture-pixel.cx --` 运行真 WebGL2 对拍。末尾添加 `--mutate` 互换产品的近远深度读回，预期第 31 项失败。
