# 下次慢车道纪元合并清单(2026-11-02)

只计划。一次纪元合并全部已修/已定项,按「不改产品身份→改产品身份」排序,每步可回滚。产品身份(entry+P3)只在标注项变。

## 合并顺序

A→C→B(作废)→D→E→F→G。A/C 已提交 HEAD,纪元只跑 D/E/F/G。

| 项 | 谁 | 路径 | 验收命令 | 回滚 | 产品身份 |
|---|---|---|---|---|---|
| A kernel收窄(已提交) | bdy | kernelinputs+8工具 | kernel-build 全链路字节==旧blob(32cecc8a/4d6e45a1) | revert cbf41774d7,098801f74a | 不变 |
| B W5补丁04作废 | cc | source-package-pack basis_pin | 不应用 | 无 | 不变 |
| C host.cx cwd头(已提交) | cc | src/host.cx+check-include-shadow.cx+srcshape.tsv | check-include-shadow 8条绿 | revert 9dd65bae25 | 变entry |
| D W5补丁01 lower-codeend-txt | bdy | exec/lower/codeend-manifest.tsv 一行 | source-job-wave 全绿+final VERIFIED | 还原该行 | 变P3 |
| E W5补丁02 unit-cap128 | bdy | cx/cx0/bundle.cx(64→128)+exec/c/PACKAGE.md+exec/parse2/units-result.tsv+**tests/preprocess/cx0-bundle.cx(65→129)** | 65单元rc1→rc0,129拒绝 | 还原四处 | 变entry+P3 |
| F W5补丁03 pp-negative | bdy | exec/pp/include-location-result.tsv+macro-byte.tsv | pp-negative 批次绿 | 还原两表 | 变P3 |
| G FFI字节表入include/ | go | tools/gen-ffi-bridge.cx+check-ffi-bridge-bytes.cx+include/ffi_bridge_bytes.h+exec/facts一行 | gen-ffi-bridge --check绿;树内include字节表可用 | 删头+生成器/检查 | 变entry+P3 |

## 要点

- D/E/F 是模型表/对象小改,一次纪元内连跑;G 依赖现建 P3+entry,放最后作本纪元身份收尾(entry+P3 一起变)。
- W5 补丁 04(basis_pin 锁 ddeb0021)已无意义:HEAD 的 d1153cc8db 已改为运行时 self_sha。
- E 的 65 单元放行是门禁值对照,非耗时对照(60s 时限是另一层,靠 rb_phase/entry-build 对象路线,与 W5 无关)。

## 未核实项(已实测,预演结论)

1. **E 的 129 拒绝覆盖 = 已存在但补丁 02 漏同步(实测 FAIL)**:`tests/preprocess/cx0-bundle.cx:25` 硬编码 `put(bad+12,65);reject(...,"unit-count-bound")`。补丁把上限 64→128 后 65 变成合法值,实测 `CX0_BUNDLE assertions=38 failed=1`(`unit-count-bound actual=bundle-digest`,即 65 不再触发 unit-count 拒绝而是走到 digest)。**需同步改为 `put(bad+12,129)`**(已写入 E 行路径)。纪元时必须连同 cx0-bundle.cx 一起改,否则 `check-cx0-bundle` 红。
2. **G 的 include 枚举结论**:`release-build.cx` `rb_walk`(160-179 行)用 `sy_dir_names` 枚举 include 根,只收 `kind=='f'` 普通文件 + `kind=='d'` 子目录,`kind=='l'` 符号链接**不收**。所以 ffi_bridge_bytes.h 必须用生成器产出**普通文件**(非符号链接),才能进 P3 资源——印证 ffi-bridge §4「用生成不用链接」。
