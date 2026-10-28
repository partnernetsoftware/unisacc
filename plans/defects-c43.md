# c43 已知缺陷登记（cc，2026-10-28 起）

## link.undef-function-refusal — 未定义外部函数链接成崩溃镜像（开放）

- 现象：`int used(void); int main(void){return used()+2;}` 经 `-c -b lnx/x86_64 -funit` 得 main.o 后，`unisacc -o img main.o`（或加只定义 `unused` 的归档、或直接 `-o img main.c`）rc 0 并写出镜像；运行 SIGSEGV（rc 139）。预期：`link: undefined reference to used`，rc≠0，无镜像。
- 定位：src/tapelink.c:351 `tl_check_undef`（R20-3）只检查名字以 `g_` 开头的未定义外部（全局数据对象），函数外部不检查。
- 同现：经典参考 REF 0ed46192 行为相同（共享缺陷，非回归）；产品==REF 字节差分仍有效，不阻塞候选 f3705dfa。
- 复现入口即批次：`tests/batches.tsv` 行 `link-undefined-function`（tools/link-undefined-function.cx），当前红 0/3，预期不放宽；日志 out/c43-cc-reg-02/link-undefined-function.log；另有 out/c43-cc-link-undef-repro-01/REPRO.md。
- 拟议修复（由 src 域所有者 cdx/grk 在本纪元合入；cc 不动 src/）：`tl_check_undef` 去掉 `s[0]==103 && s[1]==95` 前缀条件，对所有 `tl_ext` 中未在 `tl_def` 出现的名字（除链接器自身生成的 `__init`）发 `link: undefined reference to NAME`。需同步 REF（tools/build_ref.sh 源）以保持差分；下一慢车道纪元重建后批次转绿。
- 裁定：fable 2026-10-28（选项 B+C 修复部分）：拒绝条款不属 archive.product-ar 格合同，归独立批次。
