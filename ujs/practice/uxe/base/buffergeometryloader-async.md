# BufferGeometryLoader 异步解码

`BufferGeomLoader_decodeAsync` 在 LoaderFamily 的 BufferGeometryLoader 请求就绪后，调用现有原生 BufferGeometryLoader.parse。浏览器宿主按共享请求记录执行 fetch，并通过 LoaderFamily 交还字节。成功和解析失败均释放响应借用。成功几何的数据保存在调用者提供的场地。

桥接检查几何、选项及其嵌套表、解析工作区和场地，防止写入覆盖加载请求及管理器。错误事件区与响应和解析目标分开。解析拒绝时由单独的 LoadingManager 结果区报告错误。传输阶段沿用已有进度与完成事件。已就绪请求上的地址检查失败可以修正后重试。

独立浏览器验收对拍 r160 BufferGeometryLoader.loadAsync，覆盖位置属性、索引、组、包围球、坏 JSON、坏数组类型和失败后的后续加载。篡改修改实际交付给产品的第一个顶点横坐标。
