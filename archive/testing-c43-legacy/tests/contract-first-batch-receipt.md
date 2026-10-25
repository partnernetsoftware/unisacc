# 0.0.43 第一批合同本机收据

实际跑过。仅命名suite，无整闸/resume，无publish/push。

产品runner与被检artifact均为根unisacc.com（下表）；产品源码digest c0718934bf51188719f9d1880913eb3253258d612f4bceead807b7aee5c1010b，与候选build.json一致。只动tests，未修改产品闭包。

| 文件 | sha256 |
|---|---|
| unisacc.com | ba11d636b54adcdc519a81af7553d15ab882ecb85622bdc16968d966acdb8f5f |
| tests/abimanifestcheck.cx | 0505f8dcccd551b3f2f294eba45a134281ee69257239c745e9932f765def94ca |
| tests/staticimagecheck.cx | 8a0e8abd6c9f368121f8dc88443a8edff847533c51b24eec31a9d92efbc44288 |
| tests/earlystageseal.cx | 25ddda02bcf86ea8f37d05eba326b8c575d78fe508cfe13933563e405d229b18 |
| tests/abi.manifest | 2990250de92d250129236e7b51868d851700e8ff0e01a4c22e23515fb7422403 |

1. ABI：ee1427ee，abi-manifest rc0（396条）、abi-manifest-selftest rc0（19源码突变+扫描回归）。Python对照selftest rc0；写出清单cmp逐字节同。嵌套调用、复合字面量、注释/字符串brace、nested else、elif及非法分隔符均有夹具。
2. APE：95062fbf，staticimage/staticimage-selftest rc0。历史动态候选 /tmp/c42-x/cand/unisacc-next.com 两架构正式槽位PT_INTERP/PT_DYNAMIC，检查rc1（预期）。现封存artifact两槽静态rc0；槽位/长度/CRC/架构/诱饵负例以及Python结构对照selftest通过。
3. 早段seal：early-stage-seal-selftest rc0，fixture包10文件、1accept、1refusal；修改每个角色、漏role/拒绝、错误rc、错case绑定、路径与symlink均被拒绝。正式--check拒绝fixture。

第三项只是合同数据样板，不是exec-pp-literals实测封存或复用授权。该叶的完整closure trace、全部plain/located逐例输出、oracle工具闭包与环境还未采集。未接gatequeue/gatedeps指纹；不得回填7000 PASS。

Python三个既存文件仍在ungated-checks.tsv登记为reference，只作对照；新合同只用动态C入口。
