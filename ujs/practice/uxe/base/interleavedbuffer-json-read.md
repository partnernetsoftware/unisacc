# InterleavedBuffer 字段 JSON 读回

`InterleavedBuffer_fromJSON` 读取现有 `IBJsonMeta` 和 `IBJsonRegistry`，以同一个 backing ID 共享登记表的 ArrayBuffer 字节快照。读回后的视图借用快照，向任一共享视图写入数据会被其它共享视图看到。不共享 backing ID 的视图保持独立。

`InterleavedBuffer_readWords` 把 r160 的 arrayBuffers 条目解析为内存中的 Uint32 原始字节。调用者按字典 UUID 解析一次并提供已解析的 backing，之后用 `InterleavedBuffer_readJSON` 绑定原始 descriptor。读取器验证 descriptor 的 buffer 引用，依据 r160 的 typed-array 类型和分量步长重建原生记录。与 r160 一样，JSON 只保留 ArrayBuffer 与步长，恢复视图从完整 backing 起始位置开始。原生记录要求类型对齐和完整的整数行。

验收对拍实际 r160 BufferGeometry.toJSON 和 BufferGeometryLoader.parse，保留独立数值金值和坏输入写保护。派生 InstancedInterleavedBuffer 的读取未在本切片接入。r160 的 BufferGeometryLoader 以基类恢复交错视图。
