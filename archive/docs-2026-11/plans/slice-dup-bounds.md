# 单片 runtime / libc 副本区间（符号跨度，非 map）

**产品 `entry.elf` 无节表、无符号、无 map。** 本页不改产品。

## 输入身份

- 目录：`/tmp/cdx-bom-k2-precheck-04/sl-lnx`（81 个 ET_REL `.o` + `entry.elf`）
- `entry.elf` 1860142 B；LOAD RX `FileSiz=0x1bcb16`=1821462 B（`readelf -l`）
- **不是** 已装 APE `ae0b12d8…`：该 APE 内切片无配套 `.o`，无法做本口径
- 构造器 sha `016fb959…`；p3 `29e13aeb…`（收据 `entry-build-v3`）

## 方法

1. 每对象 ELF `.symtab`：`.text` 内符号按地址排序；跨度 = 下一符号地址 − 本地址（含 `__unisacc_L*` 作边界，不计名）。
2. ELF `st_size` 全 0，`nm -S` 无大小列 → **禁止当函数真实大小**。
3. 同名跨对象：`extra = Σsize − max(size)`（去重留一份的对象.text 上界）。
4. 下界：仅 **尺寸完全相同** 的重复（identical-size extra）。
5. 分类：`libc` = malloc/free/…/ferror/exit/write/fprintf… 名表；`runtime` = `_u_st*` `_unisa*` `__main_ret` `__init_u` `sy_`/`mm_`/`tx_`/`cx_`。其余不计本行。

镜像仍无来源边：区间是 **对象 .text 重复量**，上界同时 ≤ RX；不得写成已回收镜像 KB。

## 结果（本切片）

| 类 | 下界 identical-size extra | 上界 span extra | 81 份都有的 extra（参考） |
|---|---:|---:|---:|
| runtime 副本 | 28471 | 90238 | 80548 |
| libc 副本 | 1942 | 55947 | 31560 |

单片交一行：

`runtime [28471,90238] B; libc [1942,55947] B; image_attr=UNATTRIBUTED; RX=1821462; nobj=81; 口径=ET_REL .text 相邻符号跨度`

旧 269058 / 477728 **未** 在本切片复现为单值。
