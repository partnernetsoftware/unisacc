# ShaderMaterial 与 RawShaderMaterial 字段 JSON

两个原生构造器复用 Material 公共记录，保存各自的类标记，并初始化 r160 的默认顶点和片元着色器、空 uniforms、空 defines 及默认标志。四段数据写入调用者拥有的 4096 字节空间，每段最多 1024 字节。源码按 UTF-8 解码后保存。

字段 JSON 处理顶点与片元源码、GLSL 版本、lights、clipping、fog、defines 及 uniform。当前接受有限标量和 r160 的三维向量 uniform，拒绝贴图、其他向量、矩阵及其它 uniform 值。GLSL 版本允许 null、100 和 300 es。导入使用 5200 字节对齐工作区，先校验后发布字段，失败保留旧材质与源码。

浏览器验收使用独立 r160 MaterialLoader 核对双向序列化、默认形态、非 ASCII 源码、两种显式 GLSL 版本和原子拒绝。字段投影省略 UUID、名称、元数据、扩展及完整生命周期。范围限于 JSON 交换，GPU 编译和着色器执行尚未接入。
