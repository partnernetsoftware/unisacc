# 只输出链接映射方案（待 cc 评审，未实施）

## 裁决与现状

c46-1 先行；本页不授权改产品或重构候选。tapegate 不整体迁移。旧镜像归属目前为 **UNATTRIBUTED**，不能用对象字节、对象 text 比例或旧 412KB 估算替代当前候选贡献。

已实际运行 `readelf -WS -l --wide`、`nm -S --defined-only`、`size -A`：`/tmp/c2m/out/rc2-5ac7784dcb/sl-lnx/entry.elf` 无节表、无符号、无 map。文件 1855638B：RX 1817215B（含176B头）、RW文件37014B、段间填充1409B；RW内存14578216B，零尾14541202B不占文件。`size` 的 Total0 只是缺节元数据，不是零字节镜像。

## 为什么事后重放不足

- 对象 `.unisa.tape` 经 `tl_unit` 改名及全局BSS去重，`.unit/.global/.extern` 不进入合并 tape；单纯拼对象大小不能归因（`cx/compiler/tapelink.cx:120-167`）。工具能通过 `tl_bind` 的真实emit计数得到合并tape区间，但不是镜像区间。
- 真正镜像走 prune→lower→elf，不走 asmtext 的 ET_REL 写器（`src/drivestream.cx:1-4`）。lower有融合及窗口优化，来源可能跨单元（`exec/lower/README.md:13-23`）。
- `target.text` 的 `@sym` 是数据偏移，不是代码标签地址（`exec/pipeline/formats.md:91`）。仅补“目标行→镜像偏移”仍缺“目标行→原始单元”的来源边；**整文件cmp一致是必要条件，不足以证明内部单元分界**。不采用相邻零尺寸符号差作为函数真实大小，它还包含填充/无名区间。

## 提案：可审计的旁路范围，不改变生成物

建议新增显式 opt-in 的独立sidecar输出，默认关闭；接口名与落点待评审，不先加CLI。关闭时不得增加可避免分配/计算；开启也不把元数据写入镜像stdout，不加节、不改已有输出格式、指令布局、拒绝及错误/所有权合同。

1. **输入身份与单元来源**：记录target、源提交、构造器/P3/对象顺序与SHA。链接器在已有tl_unit emit边界记录来源区间；去重内容记SHARED及来源集合，合成初始化记SYNTHETIC，不归给首个单元。
2. **保留优化来源**：prune/lower保留块或记录的来源关联；删除记录不贡献输出，融合记录合并来源集合。不能可靠跟踪时明确UNATTRIBUTED。不复制整个lower到工具里，也不为此往每个输入字节再加属性；若需修改执行器资源合同，先处理CORE-INPUT-MEMORY-OOM，不能塞进本A类。
3. **最终布局输出范围**：布局收敛后输出真实`file_offset/file_length`及`virtual_address/memory_length`，分类RX、RODATA、DATA、ZEROFILL、HEADER、ALIGN。ELF层的目标行范围只解决这一步，不冒称单元归属已完成。函数/单元归属必须能沿第2步追到第1步；否则保持未知。
4. **运行库与共享贡献**：仅据真实来源或已登记生成区块识别RUNTIME/共享libc，不凭名字猜；共享范围单列，不重复记入多个单元。输出未知覆盖率，不按比例摊余量。

建议sidecar最小字段：`image_sha256,target,pkg_sha256,source_commit,range_kind,file_offset,file_length,vaddr,mem_length,owner_kind,owners,source_record_range`。先写临时文件，全部成功且镜像哈希确定后原子发布；失败保留诊断，不能留下看似完整的map。

## 验收、成本与边界

- 同一冻结输入、同一候选分别关闭/开启map，镜像逐字节cmp一致；真实Linux执行及既有拒绝合同不变。不能用旧候选map授权新镜像。
- 所有文件范围不越界、不重叠，已归属+共享+头/对齐+未知逐字节守恒等于文件大小；NOBITS/零尾仅计内存，不充文件贡献。融合跨两个单元必须SHARED或UNATTRIBUTED；空单元、重复BSS、零尺寸符号、重排和截断均保留负控。
- 可独立构造的小输入在已知边界下核map；不只看总cmp，不缓存PASS。每命令/内层最多60秒，未覆盖或未跑不得记PASS。
- 运行开销、元数据大小、构造脆弱性均 **UNMEASURED**。优先区间/块级流式记录，不建8倍输入属性表；资源失败必须明确失败，不panic冒充损坏输入拒绝。
- 此方案新增产品输出接口，须cc批准具体落点和fable复核来源证明后实施；不并入已交窗的c46-1三项产品改动。若只批准ELF行偏移map，交付必须明确它不能生成可信单元top表。
