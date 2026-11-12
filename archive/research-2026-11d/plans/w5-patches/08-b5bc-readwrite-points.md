# 08 补丁(B5b + rb_record 分流)读写点清单 —— 缩范围版

omp-cc 裁定(2026-11-03):B5c(退役 build.json artifact 字段)本纪元不做,seed/ident.c 不动,继续写 artifact_sha256/bytes。08 只含 B5b + 一处读取侧分流。

## 补丁内容(2 文件)
1. `tools/cxlib/productpkg.cx`(B5b):`pp_product_digest` 改为内部重构 6 字段 manifest 文本(name\tkind\tarch空\tsha\tsize\torder\n)再 sha256,替代原 name\tsha\n 排序。新增 `pp_member_kind`(base/stage/resource)、`pp_put_long`(整数转文本);删死代码 `pp_canon_add`。
2. `tools/cxlib/rebuildcontext.cx`(读取侧分流):`rb_record` 加 `product_root` 参数——非空时身份 = `pp_product_digest(product_root)`(product 角色,对应 root/unisacc.com 与 gen/unisacc-next.com),为空时保持单文件 sha(bootstrap/epoch/package 角色)。

## 分流调用点
| 调用点 | product_root | 角色 |
|---|---|---|
| rb_verify_mode 250行 root/unisacc.com | c.root | root 产品(split2 成员集) |
| rb_optional_product_metadata 243行 gen/unisacc-next.com | c->gen | next 产品 |
| 其余(bootstrap/epoch/controller/mixed builder) | 0(NULL) | 单文件 sha,不变 |

## B5c 相关(本纪元不做,列明不遗漏)
- seed/ident.c:189/234 的 build.json artifact_sha256+bytes 写/读:保留不动。
- 其余 9 个 artifact_sha256 解析点(rebuildcontext rb_record 的 canonical 重写、sourcepackage/source-package*/identity-material/libneed-material/candidate-identity 等):保留不动,读历史 build.json 记录,与 B5c 一起延后。

## 验证(私有副本 B5bc-v3,git archive HEAD + patch -p1)
- patch -p1 应用成功(2 文件)。
- product-build.cx 编译通过(打印 usage)→ rb_record 分流语法+use productpkg 正确。
- candidate-install.cx 编译通过(打印 usage)→ 第二个 rb_verify 入口正确。
- check-productpkg assertions=20 failed=0(副本改后 productpkg)。
- pp_product_digest(epoch root) = aebc27fc... == manifest-gen sha256(逐字节相同)。
