# Loader 家族配置与请求

范围为 three.js r160 的 Loader 及十二个核心派生类。类清单见 loader-family-layout.tsv。GLTFLoader 属于插件。r160 核心类的共同接口是构造器、五个配置方法、load、loadAsync 和 parse。这里覆盖构造、管理器身份、五个链式配置方法、实例隔离及 FileLoader 的一次请求生命周期。r160 夹具还通过受控回调验证继承的 loadAsync 转发、进度、完成和异常，并对 ObjectLoader 覆写的 loadAsync 单独验证请求配置、场景解析和拒绝。各类已有的解码或解析入口继续承载其各自的数据合同。

copy、clone、toJSON 和 dispose 不是这些 r160 Loader 实例的方法。构造和五个配置方法对应这组加载器的对象合同。FileLoader 的 release 回收基座持有的响应视图，属于基座的缓冲生命周期。r160 侧对照请求完成、失败及后续请求，基座的过期代号、重叠内存、容量拒绝和响应回收由独立 cx 断言。

调用各类的构造入口（例如 FileLoader_family）时预留并清零 LoaderFamily_size() 字节。manager 为 -1 时选择同基座默认管理器。路径、资源路径和跨域配置借用调用者的字节区。跨域长度的全一值表示默认 anonymous，零长表示显式空字符串。请求头是宿主拥有的字典句柄，零值表示该实例的空字典。宿主需在相关配置和请求生命周期内保留借用数据。

FileLoader 请求把 path 和 URL 字节拼接到调用者的缓冲区，并保存凭据和请求头句柄。宿主据此解析管理器 URL 策略并投递响应。成功后由当前代号读取响应地址和长度，使用完毕再释放。资源路径和跨域字段保留为配置，但不参与 r160 FileLoader 的 Fetch 请求。

具体解析器共用 LoaderFamily_beginAsync 和 LoaderFamily_receiveAsync。宿主用请求描述发起异步操作，把响应的状态和借用字节按同一代号交回。管理器事件记录传输的完成及错误。解析成功或失败由对应解码入口的返回值交给调用者。只有完成传输的当前代号可进入解析，解码入口在实际调用解析器后即释放响应。解析前的地址拒绝保留响应，调用者可换用正确缓冲区重试，或用 LoaderFamily_releaseAsync 明确释放。

当前解析接线覆盖 AudioLoader 的 WAV、MaterialLoader 的基础材质 JSON 和 ImageLoader、ImageBitmapLoader 与 TextureLoader 的 PNG。ImageBitmapLoader 将解码结果复制到调用方独立缓冲，可翻转行和预乘 Alpha。TextureLoader 输出纹理描述，像素由调用方提供的 RGBA 缓冲持有，上传由调用方调度。PNG 单独准备基座，运行 `cx -I ujs/tests ujs/tests/loader-family-png-linked.cx -- --prepare`，再运行同一门，附 `--mutate` 会修改像素读回。准备缓存可用 `UXE_PNG_LOADER_WASM_CACHE` 隔离。类专用入口复用既有解析范围，调用者负责提供输出和暂存区。其他派生类继续使用各自既有解码或解析入口。DataTextureLoader 与 CompressedTextureLoader 的 r160 载入依赖派生类提供数据解析器，CubeTextureLoader 需要六个图像请求的聚合。

验证命令如下。

(1) `sh ./unisacc.com ujs/tests/loader-family-check.cx --` 核对 13 类共同配置和 FileLoader 生命周期。

(2) `sh ./unisacc.com -I ujs/tests ujs/tests/loader-family-async.cx --` 对拍 r160 成功、错误、并发和重试，并校验独立金值。加 `--mutate` 将材质透明度读回替换成粗糙度，预期第 7 项失败。
