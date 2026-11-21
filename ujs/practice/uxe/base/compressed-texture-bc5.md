# BC5 软件解码

`CompressedTexture_bc5Level` 验证 RGTC2 的 mip 描述符。`CompressedTexture_decodeBC5` 复用 BC4 软件解码核心。每个 16 字节块按先红后绿读取两个独立端点表与索引流。蓝通道为 0，alpha 和整数舍入沿用 BC4 的 RGBA8 UNORM 或 RGBA8 SNORM 合同。

验收用独立金值和 r160 mip 描述核对通道顺序、两种符号模式、边缘裁剪与六值和八值插值。篡改只更改绿通道选择器。运行本格式需同时链接 `compressed-texture-bc4.ujs`。硬件压缩上传另行验证。
