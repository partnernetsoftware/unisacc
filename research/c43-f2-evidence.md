# F2 验证记录

F1 新根 SHA256 `4f0b493c7a83d73a4ae1b348c8400ea3b7eda0ea190051b54a84c712a6730eda`。

两个 P4a 探针移至 `tests/c/a_array_pointer_member_store.c`、`tests/c/a_array_pointer_arith.c`，与原文件逐字节相同。隔离 scratch 实际运行：新路径 ccrun（NOBASE=1）2 通过、0 wrong、0 refused；exec/c/chain.sh（CHAINSHARD=1/1，默认网络）2 equal、0 bad。据此 ccrun required-name 基线增加两个名字，计数由 90 增至 92。冻结 glossary 路径表仅移除旧路径，未添加新行。

TP-9 与 Tape 检查器 README 对齐现有整数下划线拒绝语义；符号名下划线仍允许。docs、tapecheck-model-1..3 与语料分片检查结果随本提交的 commitgate 日志保存；com-tapedomain 使用单独的 --com 闸验收。F3/F4 将在最终探针集合上全扫，旧分片结果不复用。
