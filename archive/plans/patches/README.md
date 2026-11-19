# cc 补丁提案（未应用；exec/src 所有者 cdx/grk 合入）

- `driver-diagnostics.diff`：exec/c/compiler.c — link_objects 末尾加"无单元对象"与"无 main"具名拒绝，product_ar 入口校验操作字母。关 triage #15–#18；验收 = 批 driver-diagnostics 11/11、link-edge-cases.no-main 文本具名。`git apply --check` 已在冻结导出（f3705dfa 源）通过。
- tapelink 补丁：已由 src 所有者在 896888f424 合入（"Reject unresolved function externs in unit linker"），cc 的提案撤回；验收 = 新产品上批 link-undefined-function 转绿。
