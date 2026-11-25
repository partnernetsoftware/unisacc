# v0.1.0 至 v0.1.2 公开资产路径审计

结论：扫描的 24 份字节输入均未命中家目录路径模式；没有发现需要据此替换或下架已公开资产的证据。未改发布状态。中间对象 `.unisa.tape` 的绝对路径问题单列，不外推成公开资产泄露。

| 版本 | 核验 SHA 的发布资产 | 解压的 Unix 切片 | 明文家目录前缀 | 十六进制家目录前缀 | 本机账号路径组件 | 完整对象 tape 记录 | 命中偏移 |
|---|---:|---:|---:|---:|---:|---:|---:|
| v0.1.0 | 4 | 4 | 0 | 0 | 0 | 0 | 无 |
| v0.1.1 | 4 | 4 | 0 | 0 | 0 | 0 | 无 |
| v0.1.2 | 4 | 4 | 0 | 0 | 0 | 0 | 无 |

(1) 从 `partnernetsoftware/unisacc` 三个发布标签下载 `unisacc.com`、`pkg.bin`、`build.json`、`slice-win-arm64` 及 `SHA256SUMS`。APE、包、Win arm64 对照对应 `release/evidence/<版本>/public-readback.txt`；`build.json` 对发布 `SHA256SUMS` 并核与公开读回的 `unisacc.com.build.json` 同摘要。四个 Unix 切片按已核 APE 启动表的 `o-1` 字节偏移和 `n` 长度解 gzip；Win x86_64 原始 PE 已包含在全 APE 扫描中。

(2) `public-asset-path-scan.cx` 同时扫明文和大小写十六进制的 `/home/`、`/Users/`、`\Users\` 前缀，以及以路径分隔符界定的本机账号组件。不输出匹配字节或真实账号，只输出从零起算的输入文件偏移和匿名编号。核 `UNISATAPE1` 声明长度、边界及 `.unit 2` 首记录；若有匹配，标记是否落在载荷内。Linux ELF 六片与 APE 三个 Win x86_64 PE 头另经段表检查，未见 `.unisa.tape` 节。Win arm64 和 macOS 未做独立段表确认，整片字节和可验证的对象 tape 载荷均已扫描。

(3) 正控：合成 tape 中大小写十六进制家目录命中偏移 33，明文 macOS 家目录命中偏移 62，分别标记为载荷内与载荷外，两处归同一匿名账号。正式实跑原行：`PATH_AUDIT files=24 aliases=0`，`PATH_HIT` 行数 0。逐文件计数在 `public-asset-path-audit/public-scan.tsv`，SHA-256 `0b13bf953e494f9f93b73429d0c49c282e77aeb139df65fd6aae1d8cf4249a00`；正控在同目录 `fixture.out`，SHA-256 `11c280da4f62cff6bfe8242a9d45c222d6434d187644d157e03b6feceb93e0d9`。

范围：已扫发布文件全字节及 APE 四个 Unix gzip 成员解压字节。未递归解码模型包内部模型压缩块、其他编码或其他版本；不据此声称任意形式的隐私数据均不存在。
