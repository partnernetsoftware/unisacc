# CX0BND1：首版不可变编译单元束

实现为 `bundle.cx`；独立手写字节 gold 在 `tests/preprocess/cx0-bundle.cx`。本版封存源字节、CXIF v0 接口、明确供给的头资源、单元顺序和有序强制头引用。读取器不访问文件系统，先复制调用者输入，再校验并交付只读视图。

所有整数是无符号 32 位小端，所有长度都是字节数；字符串不带 NUL。束最大 64 MiB，1–64 个单元，最多 4096 项资源，每单元最多 128 个强制头；名字为 1–4095 个无控制字符字节。目标接受六个既有 `OS/ARCH` 名称；ABI 固定 `unisa-c99-v1`。独立 E2 适配器目前仅实现 `lnx/x86_64`，其他目标明确拒绝。

| 偏移 | 字段 |
|---|---|
| 0 | 8 字节 `CX0BND1\n` |
| 8 | 总长度，含末尾摘要 |
| 12 / 16 | 单元数 / 资源数 |
| 20 / 24 | 目标名长度 / ABI 名长度 |
| 28 | 保留字段，必须为零 |
| 32 | 64 字节小写十六进制：完整 E2 模型容器的 SHA256 |
| 96 | 目标名，随后 ABI 名 |

随后顺序存放所有单元：六个整数 `id, language, name_len, source_len, include_count, reserved`，再存名字、源字节和 `include_count` 个资源 ID。ID 必须从零连续；language 为 0=C、1=CX；reserved 必须为零。同一单元不得重复引用资源 ID。

随后存放所有资源：五个整数 `id, kind, name_len, data_len, reserved`，再存名字和资源字节。kind 为 1=随带头逻辑名、2=精确文件逻辑路径；reserved 必须为零。名字在两类资源之间也必须唯一，避免强制头名称查询产生歧义。空字节资源表示存在且为空，缺失表示没有该记录。

最后 64 字节是此前全部字节的 SHA256，小写十六进制文本。摘要用于核内容，不是签名或来源认证。读取器拒绝错误版次、长度、数量、保留位、枚举、名字、重复、越界引用、截断、尾随和摘要错配；失败返回清零视图及具名原因。

## API 与资源取得

`cxb_seal` 借用参数并返回自有字节，使用后 `cxb_release`。源码来自规划器实际读取的观察记录；同一个路径在本次规划中重复读取但字节不同会拒绝。生成的接口同样封存，调用者不能用冲突的额外资源覆盖它。普通 include 的资源由调用者显式供给，不对文件名再次隐式搜索。完整跨文件原子快照尚未实现。

`cxb_read(input, bytes, &view, why, capacity)` 的输出须为新对象或已经 `cxb_clear`。成功后的单元/资源视图借用它拥有的复制字节，调用者释放原输入或改写现场文件不会改变视图；用完 `cxb_clear`。C/CX 的公开结构仍可被调用者误改，这不是编译期所有权或防篡改内存。

`tools/cx0-bundle.cx` 提供直接源码入口：

```sh
sh ./unisacc.com tools/cx0-bundle.cx -- seal lnx/x86_64 E2_MODEL INTERFACE_DIR RESOURCE_MANIFEST NEW_BUNDLE SOURCE.cx
sh ./unisacc.com tools/cx0-bundle.cx -- inspect BUNDLE
```

资源清单每行是 `H<TAB>头逻辑名<TAB>物理文件` 或 `F<TAB>精确逻辑路径<TAB>物理文件`。所有需要的传递头也须明确供给；CX 源通常至少需要 H 类 `cx.h`。输出独占新建，不覆盖旧束。CLI 对输入文件先核普通文件类型和 64 MiB 上限，再完整读取、核尾部和关闭结果。

## E2 消费与已知边界

`tools/cx0-e2.c` 是通用 C 执行器的资源适配壳，参数为 `E2_MODEL_CONTAINER BUNDLE UNIT_INDEX`。它核对实际容器字节的 SHA，将同一份已核字节通过私有匿名文件交给现有包读取器，运行真正的 E2 网络。源码、强制接口、普通头仅取束内字节；未声明头即使在现场存在，也不能回到文件系统取得。只有网络接受后才向 stdout 交付 UNIPP1。

构建所需 `bundle.h` 是 CXIF v0 生成头：

```sh
sh ./unisacc.com -cx-interface cx/cx0/bundle.cx > cx/cx0/bundle.h
sh ./unisacc.com tools/cx0-e2.c cx/cx0/bundle.cx -o out/cx0-e2
```

本版保留调用者提供的源路径和显式接口路径，尚未实现跨目录搬迁的规范逻辑 ID。直接依赖图、一般 `-D/-U/-I` 选项序列、CX0 原位置映射和类型化 CXIF 尚未进入束格式；本版仅支持明确的默认配置和既有接口投影。当前适配器每单元进程隔离，尚未证明单进程内任意实例恢复。已安装产品驱动仍未切换到此路径，完整 U1/U2 和外国宿主合同仍开。
