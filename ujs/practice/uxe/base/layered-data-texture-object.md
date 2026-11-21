# Data3DTexture 与 DataArrayTexture 公共对象

两类对象沿用 r160 默认设置。放大和缩小用最近邻过滤，三轴用边缘夹取，解包行对齐为 1，默认关闭翻转、mipmap 和预乘 alpha。公开格式从 RGBA 开始，调用 `LayeredTextureObject_red` 后可将借用的 R8 数据用于现有 WebGL2 读回。R8 基础级上传要求对齐为 1，关闭翻转、mipmap 和预乘 alpha。

copy 保留目标标识和事件注册表，并共享源对象的 Source。clone 需要新标识和独立空注册表。两者都会增加目标版本和共享 Source 版本。像素仍由调用者持有。通过任一对象修改同一 Source 的像素，另两个对象都能看到变化。r160 的 Texture.copy 保留目标当前的 wrapR，clone 使用构造默认值，因此 wrapR 的处理与 wrapS、wrapT 不同。

`LayeredTextureObject_json` 写出所建模的过滤、解包状态、格式、寻址及 Source UUID 字段。UUID 由调用者按共享 Source 提供。字段切片省略对象 UUID、名称、元数据、UV 变换和像素数组。r160 不将 wrapR 写入 Texture JSON。dispose 分发事件，保留记录、像素和 Source，重复调用可继续分发。r160 这两个类均未提供局部层更新 API。

`LayeredTextureObject_readJSON` 把同一字段集导入已经绑定 Source 的对象。若 JSON 带有 image 字段，其值须与调用者为当前 Source 提供的 UUID 一致。缺失字段采用构造默认值。导入保留对象标识、对象及 Source 版本、像素和 wrapR。当前字段集支持默认 UV 映射、通道 0、UnsignedByteType、无色彩空间、各向异性 1，以及 RGBA 或 Red 格式。显式传入其它值会被拒绝。坏输入不会发布字段修改。导入使用调用者提供的 1104 字节工作区，按 8 字节对齐，UUID 最长 1024 字节。导入不会累积申请运行期堆内存。

浏览器测试对拍独立 r160 实例和共享 R8 记录。3D 跨层采样及数组整数层采样均有真实 WebGL2 读回。负控把克隆接到另一个 Source，因而改变共享身份、版本、JSON 引用及两个纹理类别的读回值。

导入验收还覆盖 r160 导出后由 UJS 导入、重新导出并读回。每类纹理在同一 Wasm 实例连续导入 256 次。另用丢失 wrapT 的篡改验证此字段经过导入链。
