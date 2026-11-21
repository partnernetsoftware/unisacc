# WebGLUtils 非压缩上传映射

`WebGLUtils_textureFormat(out, format, kind, srgb)` 接受 r160 像素格式和数据类型常量。srgb 为 0 时用线性传递，为 1 时映射到 sRGB 存储。成功返回对齐的输出地址，并写入 WebGL2 的内部格式、外部格式和数据类型。

映射来自 `webgl-utils-formats.tsv`。生成器读取既有颜色、深度、整数和传统格式表，以及 r160 数据类型常量。覆盖的组合在 Chromium 中实际创建和取样。整数 RG/RGBA 的参考使用 r160 的显式内部格式，深度参考直接观察 r160 的内部存储格式，三个枚举均由产品的真实 WebGL2 分配独立验证。

拒绝码为 INVALID_ARGUMENT (-1)、UNSUPPORTED_COMBINATION (-2)、COMPRESSED_EXTENSION (-3) 和 LEGACY_WEBGL1 (-4)。每个拒绝保留旧输出。压缩格式的诊断只表示需另设扩展路径，硬件压缩上传和采样仍待独立验收。

`cx ujs/tests/webgl-utils-generate.cx -- --check` 核对派生表字节。`cx -I ujs/tests ujs/tests/webgl-utils-pixel.cx --` 运行 WebGL2 对拍。添加 `--mutate` 将产品 sRGB 映射改为线性存储，预期枚举和采样均转红。
