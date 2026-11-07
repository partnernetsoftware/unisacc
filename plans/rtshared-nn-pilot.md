# 试点：rtsharedname（替代 asmtext）

不改产品。asmtext 暂缓。

## 为何是它

- 单元 `m14.o` .text 8420，`default_path=YES`，`FINITE`。
- 真源已是表：`exec/facts/rt-shared.tsv` 2 行 group。
- 生成器已绿：`sh ./unisacc.com tools/rt-shared-gen.cx -- --check` → `RT_SHARED_GEN SAME groups=2 bytes=3191 unlisted=0`。

## gold 文本等价（非往返）

对每个 group 下标 i∈{0,1}：

- `rts_at` 写出的 `group,probe,provider,unit_macro,extern_macro` 与 tsv 列 **逐字段字节相等**（含 NUL 语义：C 字符串与表单元格）。
- `rts_covers(i,name)` / `rts_used(i,name)` 对表内每个符号为 1，对表外抽样名为 0。

**不是** `as(dis(x))==x`。

## 拒绝合同

| 输入 | 现实现 | 迁网后必须相同 |
|---|---|---|
| 空指针 `rts_at` | 4 | 4 |
| i∉{0,1} `rts_at` | 2 | 2 |
| `rts_covers` name=NULL | 4 | 4 |
| i 越界 `rts_covers` | 2 | 2 |
| `rts_symbol_count` 坏下标 | -1 | -1 |

夹具：生成器 `--check` + 现有 rt-shared 批次（若有）保持 rc0。

## 内存 / 时间

- 若做成流：输入约 4B 下标；执行器属性 **8B/输入字节** → 约 32B 属性（CORE-INPUT-MEMORY-OOM 仍在）。
- **每字节步数：UNMEASURED**（本块未跑 `sim --cov` / core_run 计数）。小样计划：i=0 与 i=1 各 1 次 ask，记录步数/ns 后再批。
- 体积：2 键网 vs 现 84 行 C，**可能不降切片**；目标是证明默认路径 FINITE 单元的 gold 文本门，不是省 223KB。

## 不做

不把 inflate/`codec.o`、cxmod、tapelink、asmtext 当本试点。不碰 loader。
