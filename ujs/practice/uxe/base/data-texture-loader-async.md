# DataTextureLoader 异步 RGBA8 回调

共用 LoaderFamily 请求链处理路径和二进制响应。调用 `DataTextureLoader_decodeAsync` 时提供一个同步 UJS 解析闭包。闭包收到响应指针、字节数、解析记录地址、暂存区地址及容量。成功时在记录中写入 RGBA8 像素地址、长度、宽和高，并返回 0。像素必须位于当前响应或调用者提供的暂存区内。

桥接校验尺寸、容量及指针边界，然后复制到独立的调用者像素区，并使用现有 DataTextureLoader RGBA8 接口发布 GPU 纹理描述。失败保留旧纹理描述和像素。参数冲突保留尚可消费的响应，解析结果成功或拒绝均释放响应。解析错误通过 `DataTextureAsyncWork.error` 报告，传输进度和完成记录沿用 LoaderFamily。

验收使用实际 fetch 和独立 r160 DataTextureLoader 子类。小型测试格式同时覆盖解码后暂存字节和直接借用响应字节。像素篡改发生在响应转交原生前。验证范围为 RGBA8 纹理描述和字节读回。具体纹理文件格式、压缩及 GPU 上传由其各自路径验证。
