# FramebufferTexture GPU 捕获

`FramebufferTexture_gpuCapture` 从现有二维 RenderTarget 的颜色附件零读取矩形，用独立的 GPU 数据包执行 GpuTarget_read，然后把 RGBA8 结果登记为 GpuTextureTable 句柄。坐标以左下为原点。纹理采用最近采样和夹边，保留捕获时的像素。绘制仍经现有纹理数据包上传和执行。

对象、纹理表、RenderTarget、工作区和 RGBA8 缓冲由调用方分配。工作区预留 `FramebufferGpuWork.size + packetCapacity` 字节，前者保存两个描述符和缓冲尺寸，后者放独立读取包。RGBA8 缓冲须覆盖全部矩形且与这些记录及仍在使用的纹理缓冲分离。

`FramebufferTexture_gpuUpdate` 要求当前表版本及另一个 RGBA8 缓冲。读取成功后才发布下一版本，旧缓冲届时可由调用方回收。失败保留现有句柄、版本与像素。`FramebufferTexture_gpuDraw` 与 `FramebufferTexture_gpuVersion` 均核对句柄代数。释放退役句柄，由调用方回收其像素缓冲。仍有效的独立快照可在源 RenderTarget 释放后继续绘制。

`FramebufferGpuObject_init`、`copy` 和 `clone` 通过 `FramebufferGpuShared` 管理共享源，分别保存对象版本。在共享源尚未捕获时，来源与尺寸可用，纹理表句柄仍为空。r160 同一缓存键的共享源在首绑时分配空纹理，捕获后各引用读到同一像素。UXE 以首次捕获建立可绘制的表记录。调用方可用 `FramebufferGpuObject_source` 得到捕获记录，并在选择工作区和像素缓冲时避开整个共享记录及其对象。使用共享对象时由 `FramebufferGpuObject_dispose` 释放引用，最后一个引用才退役表句柄。

验证使用现有 WebGL 宿主创建并清理真实帧缓冲，将捕获结果重新上传和取样，逐像素对照 r160 的 copyFramebufferToTexture。运行 `UXE_FRAMEBUFFER_TEXTURE_WASM_CACHE=缓存绝对路径 cx -I ujs/tests ujs/tests/framebuffer-texture-gpu-pixel.cx -- --prepare` 准备基座。移除末尾参数执行功能门。参数 `--mutate` 让 C6 小探针将源横向偏移多加一格。
