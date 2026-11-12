# Paper B 相对 ujs.com 进化：该改什么（2026-10-25）

**裁定指针**：`ujs/prd.md` §0.1b（产品 · 论文 · unisacc）。  
**正文**：`research/ujs-paper.md` 已打 Status／谱系／闸 补丁；全篇改口另裁，不挡产品版号。

## 需要改（合理、非可选）

| 点 | 旧稿风险 | 应对 |
|---|---|---|
| **谱系** | 「companion to A」易读成 A 父／B 子产品 | 明写 **B→A 派生**；A 只作方法核引用 + C 面佐证 |
| **主产物** | 强调 M3／`compiler_core`／Pages 像「出货答案」 | 升格 **`ujs.com`／host1_package**；M3 保持脚手架脚注 |
| **门禁** | `tests/ujs.sh`／`ujs2wasm_compiler.sh` | 改指 `ujs/tests/run.cx`；旧脚本 archive_only |
| **树与语言债** | seed/construct Python、weights `.c` | 注明已迁 `archive/ujs-legacy-py-c/`；方法弧未卸完 |
| **三标签** | 易混 host1／M3／construct | 正文与附录坚持 `host1_package` ≠ `m3_scaffold` ≠ `method_construct` |

## 不必改成「产品绿 = 理论完」

- host1 验面／28 语料 **不**卸 m4／IntNet。  
- 不把 `--verify` 写成语义完备证明。  
- Paper B 主张中心仍是表／流→`\0asm` 的方法弧；产品只提供可辩护场景与诚实边界。

## 建议下一裁（写作）

1. Abstract／§1 重写「问题来自闭合 JS＋wasm 生产力工具；A 为派生佐证」。  
2. §2 贡献表加一行：**host image 合同**（编＋预言＋验面）作为 *engineering surface*，非 claim center。  
3. Appendix D 增一行：`ujs.com` vs `unisacc.com`（同理论两语言面，非父子产品）。
