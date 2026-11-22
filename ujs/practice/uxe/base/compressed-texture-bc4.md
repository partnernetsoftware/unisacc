# BC4 软件解码

`CompressedTexture_bc4Level` 验证 RGTC1 的 mip 描述符。`CompressedTexture_decodeBC4` 将一个 mip 写入紧密排列的四通道字节。无符号输入输出 RGBA8 UNORM，有符号输入输出 RGBA8 SNORM 二补码。红通道采用区间插值，绿和蓝为 0，alpha 分别为 255 和 127。整数插值向零截断。有符号端点 -128 取值为 -127，选择插值分支时按原始有符号端点比较。对规范允许实现差异的 -127、-128 组合，本实现选择原始次序。

验收用 Khronos Data Format 1.4 的 RGTC 规则独立构造金值，并核对 r160 保留 mip 描述与枚举。用例覆盖两种符号模式、两种端点顺序、非四倍数尺寸、别名和容量拒绝。软件解码门覆盖本地内存输出，硬件压缩上传另行验证。
