# AnimationLoader 异步解码

AnimationLoader 经共享 LoaderFamily 开始请求并接收字节。宿主执行 fetch，将响应交还给已经打开的请求。原生 `AnimationLoader_decodeAsync` 只接受本类已就绪的请求，复用现有批量解析器及其数值、布尔和字符串等轨道处理。成功或解析拒绝都会释放响应借用，已发布片段保存在调用者场地。

解析拒绝会通过独立结果区提供 LoadingManager 错误事件，传输阶段的进度和完成记录沿用 LoaderFamily。它们是分别消费的两组事件。参数、地址重叠或请求代数坏值拒绝本次调用，保留尚可消费的当前响应。

浏览器对拍实际 fetch 与 r160 AnimationLoader.loadAsync，覆盖同一管理器上的并行请求、坏 JSON、坏轨道、错误后的再次请求、旧回复拒绝及响应失效后的片段读取。篡改改变到达产品的 fps。
