# 分组摘要的聚合算法(可复算)

适用于 INDEX.tsv 中 5 条分组摘要:`strict-W-original`、`strict-W-original-raw`、`chains` 三条(每个链目录一条)。

## 算法

- 选取集合:该组文件的全路径清单(见下)。
- 排序:按相对路径的字节序升序(`strcmp`,即 C locale 字节比较)。
- 序列化:对每个文件产生一行 `<sha256>  <relpath>\n`,两个空格,行尾 LF,无 BOM。
- 摘要:对整段序列化文本的字节做 SHA-256,输出 64 位小写十六进制。
- 逐文件 sha256 本身是该文件全部字节的 SHA-256;`strict-W-original-raw` 用脱敏前的源文件字节,`strict-W-original` 用归档副本字节。

## 各组输入

- `strict-W-original`:归档目录下 `strict-W0*.tsv`(292 个,不含 6 个 W99 窗),`relpath` 即文件名。
- `strict-W-original-raw`:同一 292 个文件在 `~/.cache/unisacc/prerun/v0.1.3-f485ec9a2b/` 的原始字节,`relpath` 同文件名。
- `chains/<rev>`:以各 REV 目录自身为根(`chains/<rev>/` 即该组的根),选取其下全部常规文件(含 `bg.log`、`chain-run-*.result`、`owner.tsv`、`exit.tsv`);`relpath` 相对 REV 根,**不含** `chains/<rev>/` 前缀;三条分别为 `fd57ec1f…`、`a0c4fc8285…`、`f485ec9a2b…`。INDEX 中三条链摘要均按此基准重算,不复用任何以证据包根为基准的旧值。
- 逐文件原始与归档 sha 的对照另见 `prerun-inputs/strict-W-sha-map.tsv`(292 行,列为 file/raw_bytes/raw_sha256/archived_bytes/archived_sha256;188 个文件仅 `layout=` 行家路径被替换为 `~/`)。

## 复算命令

- 单文件:`sha256sum FILE`。
- 组摘要:按上述排序与序列化生成清单后,再对清单字节做一次 SHA-256;归档侧同一工具生成 `evidence-rows.tsv` 与 `INDEX.tsv` 的本组行。
