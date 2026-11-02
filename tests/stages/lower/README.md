# lower 阶段独立测试台

阶段：`lower`（`tape.text` → `target.text`），七流第 6 位（e2→e1→e3→e4→prune→lower→elf）。

## 权重归属（真源 prd.md:128-130）

| 网 | 文件 |
|---|---|
| abi | `weights/gold/abi.tsv`（522 行，87 系统调用） |
| regmap | `weights/gold/regmap.tsv`（16 行寄存器映射） |
| reloc | `weights/gold/reloc.tsv`（6 行重定位） |

改这三个 gold 表（或对应 `.unisa` 构造产物）→ lower 阶段受影响 → `stream-lower-*` 批次重跑；
改其他网（如 `weights/gold/pp.tsv` = e2）→ lower 不受影响。

## 依赖判定

`tools/check-stage-lower.cx` 经 `tools/affected.cx --files` 实际执行：
- 改 `weights/gold/abi.tsv` → `stream-lower-domain-*` 受影响（重跑 lower）
- 改 `weights/gold/pp.tsv` → `stream-lower-domain-*` 不受影响（负例）

## golden 输入/输出字节（样本，与 tests/stream-lower.cx 一致）

输入 `literal.tape`：
```
_start:
  imm r0, 7
  ret
```

输出 `literal.target`（stream6 产出）：
```
@target lnx/x86_64
@data -
@src_os lnx
@data_len 168
@bss 0
@relocs -
_start:
spinit rsp
argsave 304, 312, true
imm rax, 7
ret form=x86
```

阶段独立测试台不在此构造权重、不跑整包；golden 字节复现由 `tests/stream-lower.cx`
（pinned stream6 材料）覆盖，本目录只存档阶段归属与依赖映射。
