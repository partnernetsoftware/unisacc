# win argv 修法草案（不实现、不提交；cc 要求先拿探针一行）

探针会落在三种情形之一，每条对应不同改法。先把 A（`argv[1]==NULL`、argv[0] 正常）的两种改法
列清楚，含大小/风险/要动的表。

## 事实（已实测，见 plans/win-argv-2026-11-06.md）

- `.data`：va 0xb000、VirtualSize 0x20598、PointerToRawData 0x9400、**SizeOfRawData 0x200**。
- argc 槽 rva 0x1b2b8、argv 数组 rva 0x1b390..0x1b590，**都远超 raw 的 0x200**，
  而整个 PE 只有 0x9800 字节 —— 这两处在文件里不存在，全靠 loader 按 VirtualSize 零填。
- `.data` raw 区里只放了 73 个非零字节（字符串池开头：`__u_word is the 8-byte machine word`、
  `NAN`、`nan`、`INF`、`inf`、`(null)`……），其余是 0。

## 改法一：把 argv 区搬进 `.data` 的 raw 区（改布局，不动段头）

思路：`winargs` 的目标从 `pe_ds + 137`（当前落在 0x102b8 偏移处）改到 raw 区里一处
未使用的 8×64+8 字节空洞（raw 0x200 里目前用了 73 字节，理论上放得下，但会撞上字符串池）。

- **要动的表**：`exec/enc/x86win-result.tsv` 第 49/50 行把 `LDI ad_o,137` 改成新的偏移；
  若 raw 区放不下还要同时改 `exec/facts/image-pe.tsv` 的 `.data` 尺寸（那就成了改布局）。
- **大小**：两行常量（约 2 字节差异），**但必然连带改布局**：raw 0x200 已被字符串池开头占据，
  64 项 argv 需要 512 字节 + argc 8 字节，**0x200 放不下**。实际要扩 raw 到 0x400 以上，
  于是文件变大、所有节的 file offset 平移。
- **风险**：高。段头偏移、重定位页、字符串池偏移三者耦合；与"先拿观测"的精神相反。
- **是否触及网**：触及。`.data` 尺寸是 `PE.full` 的常量项，改它会改 PE 网里
  `pe_ds/pe_dvs/pe_df/pe_rf` 的一整条派生链。

## 改法二：让 PE 写出器把 `.data` 的 SizeOfRawData 补齐到 VirtualSize

思路：不动任何动作，只改"段头里那个数"的来源 —— 把 `.data` 的 `SizeOfRawData` 从
`pe_datafile`（按内容对齐）改成 `pe_dvs`（= VirtualSize）。这样 argv 区在文件里真实存在
（一片零），任何按 raw 处理的路径都能看到它。

- **要动的表**：`exec/facts/enc-pe.tsv` **第 110 行**（`.data` 段头的第 89 槽，现在写
  `pe_datafile`）改成 `pe_dvs`；`exec/facts/enc-pe-forward.tsv` 同类行同步（第 110/218 附近）。
- **大小**：改 2 处寄存器名，`0` 行净变化。段头数值从 0x200 变成 0x20598，
  **文件增大到约 0x2c000+**（VirtualSize 对齐后），比改法一更肥 —— 但只肥在一个节里，
  且不需要平移任何已有内容（新内容就是在 .data 尾部补零）。
- **风险**：中。段头与文件长度的一致性由同一个 `pe_rr`（reloc rva）派生，补零不影响；
  loader 看到 raw 覆盖 vsize 是合法 PE。风险点是别的消费者（`-run` 的装载器、dumpbin 类工具）
  可能假设 `.data` raw 很小。
- **是否触及网**：**不触及动作网语义**，只改一个操作数在段头写入时的取值来源（表里的寄存器名）。
  但它仍然改 `exec/facts/`，属产品输入，要进候选就得重造。

## 我的建议

等探针那一行。若探针显示 `argv[1]==0` 且 `argv[0]` 正常 → 优先改法二（改 2 个寄存器名、
不碰网语义、不需平移），并在同一提交里补一条检查：`.data` 的 SizeOfRawData >= VirtualSize，
防止再退化。若探针显示 argv 数组地址为 0 或一读就 AV → 那是"未提交页"，
改法二正好解决（raw 覆盖后该页必然被提交），仍是同一个 patch。

## 追加：改法二的确切落点（已核）

`enc-pe.tsv` 里 `.data` 段头的 SizeOfRawData 是**第 89 槽**，出现在两处
（第 110 行与第 218 行，各属 PE.store 与 PE.storezero 分支），值都是 `reg "pe_datafile"`；
`enc-pe-forward.tsv` 的第 110/218 行是同一个槽，同样 `pe_datafile`。
所以改法二 = 这 **4 个位置**把 `pe_datafile` 换成 `pe_dvs`（`.data` 的 VirtualSize，第 87 槽同源）。
两表都由 `tools/facts-gen.cx` 从 `exec/facts/image-pe.tsv` 生成 —— 直接改生成物会被覆盖，
必须改源表/生成规则并跑 `--check`。这是它真正的成本：不是 4 行，而是"要先查清
image-pe.tsv 里 .data 的 raw/vsize 两值分别怎么来，再把 raw 改成取 vsize"。

## 追加 2：改法二真正的落点是生成规则，不是表

`exec/facts/enc-pe.tsv` 与 `enc-pe-forward.tsv` 都由 `tools/facts-gen.cx` 生成，
.data 段头的四个字段在第 953 行一处写死：

```
if (q == 2) { fc_field(... "pe_dvs"); fc_field(... "pe_dt"); fc_field(... "pe_datafile"); fc_field(... "pe_df"); }
```

所以要改的是**这一行里的第三个字段**：`pe_datafile` → `pe_dvs`（SizeOfRawData 取 VirtualSize）。
改完必须 `facts-gen --check` 全绿（它会重算 enc-pe/enc-pe-forward/enc-arm 等全部下游表），
并重跑 `cxinclude`/`cxxapis` 相关棘轮（facts-gen 的输入清单会变）。

### 精确代价

| 项 | 改法一（argv 搬进 raw） | 改法二（raw 补到 vsize） |
|---|---|---|
| 改动位置 | x86win-result.tsv 两行 + .data 尺寸（跨表） | facts-gen.cx 一处字段名 |
| 生成链 | 要重算 x86win 结果表与 PE 布局 | 只要 facts-gen --check + 下游表重算 |
| PE 文件 | raw 0x200 放不下 512B argv，必须扩 raw 并平移所有后续节 | 只在 .data 尾部补零，不平移 |
| 触及网语义 | 是（pe_ds/pe_dvs/pe_df/pe_rf 派生链） | 否（只改段头一个数） |
| 风险 | 高（偏移耦合） | 中（若有消费者假设 .data raw 很小） |

结论：**如果探针指向 A，改法二是明显更小的那一个**，但仍属产品输入、要重造候选。
它的验收必须同时是：探针打印出 `argv[1]="A"`、`.data` 的 SizeOfRawData >= VirtualSize、
且 `.reloc` 仍只覆盖真实需要的项（补零不产生新的绝对地址，所以 reloc 不该变）。
