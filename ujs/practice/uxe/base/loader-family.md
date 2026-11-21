# Loader 家族配置与请求

范围为 three.js r160 的 Loader 及十二个核心派生类。类清单见 loader-family-layout.tsv。GLTFLoader 属于插件。r160 核心类的共同接口是构造器、五个配置方法、load、loadAsync 和 parse。这里覆盖构造、管理器身份、五个链式配置方法、实例隔离及 FileLoader 的一次请求生命周期。r160 夹具还通过受控回调验证继承的 loadAsync 转发、进度、完成和异常，并对 ObjectLoader 覆写的 loadAsync 单独验证请求配置、场景解析和拒绝。各类已有的解码或解析入口继续承载其各自的数据合同。

copy、clone、toJSON 和 dispose 不是这些 r160 Loader 实例的方法。构造和五个配置方法对应这组加载器的对象合同。FileLoader 的 release 回收基座持有的响应视图，属于基座的缓冲生命周期。r160 侧对照请求完成、失败及后续请求，基座的过期代号、重叠内存、容量拒绝和响应回收由独立 cx 断言。

调用各类的构造入口（例如 FileLoader_family）时预留并清零 LoaderFamily_size() 字节。manager 为 -1 时选择同基座默认管理器。路径、资源路径和跨域配置借用调用者的字节区。跨域长度的全一值表示默认 anonymous，零长表示显式空字符串。请求头是宿主拥有的字典句柄，零值表示该实例的空字典。宿主需在相关配置和请求生命周期内保留借用数据。

FileLoader 请求把 path 和 URL 字节拼接到调用者的缓冲区，并保存凭据和请求头句柄。宿主据此解析管理器 URL 策略并投递响应。成功后由当前代号读取响应地址和长度，使用完毕再释放。资源路径和跨域字段保留为配置，但不参与 r160 FileLoader 的 Fetch 请求。其余派生类的具体请求转发、宿主句柄解析、缓存以及解码接线属于各自的加载入口。

验证入口为 `sh ./unisacc.com ujs/tests/loader-family-check.cx --`。
