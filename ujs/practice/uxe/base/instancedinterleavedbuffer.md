# 实例交错缓冲

实例记录继承 `InterleavedBuffer` 前缀，尾部保存 `meshPerAttribute`。它用实例序号除以重复数取得交错行，拒绝越界。复制拥有独立存储，克隆沿用共享 backing 注册表。序列化记录复用共同的 JSON 元数据和 backing 表，再附实例标记与重复数。两个较长的原生方法名使用清单中列明的缩写入口。

`cx -I ujs/tests ujs/tests/instancedinterleavedbuffer-struct.cx` 验真实 r160 实例、两次克隆共享及 JSON 字段。追加 `-- --mutate` 改取前一实例行，预期退出 1。GPU 属性上传接线仍待补。
