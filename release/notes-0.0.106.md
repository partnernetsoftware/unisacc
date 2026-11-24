# unisacc 0.0.106 发布说明 (草稿)

整体裁决 NOT_QUALIFIED (草稿,待整链实跑)

## 真实身份

| 对象 | 身份 |
|---|---|
| 冻结提交 | <待冻结起链后回填> |
| 发布 APE SHA256(unisacc.com) | <待冻结起链后回填> |
| 源快照 SID | <待冻结起链后回填> |
| pkg.bin | <待冻结起链后回填> |
| slice-win-arm64 | <待冻结起链后回填> |
| unisacc.com.build.json / build.json | <待冻结起链后回填> |
| SHA256SUMS | <待冻结起链后回填> |

## 严格证据

<待整链严格证据产出后回填>

## 集成车入车清单

- (1) `0.0.124` 运行缓存漏钉修复(并入提交 `4258ae1f83844f8ee530595474376b54c1805f6a`):共享 `cx/text/pinscan.cx` 词法、续行与宏片段钉住判据,`rcident.cx` 与 `runcache.cx` 统一走 `pinscan`,新增 `tests/check-pinscan.cx` 与 `tests/check-runcache-pin.cx`。
- (2) `0.0.122` 转发桩修复(并入提交 `848039da19`):修复转发桩回放与声明扫描,新增 `cx/driver/fwdreplay.cx`、`include/sys/_ffi_forward.h` 与 `tests/check-fwdstub-replay.cx`,`cx.driver` 补入 `cx.text@0.1.0` 依赖。
- (3) `0.0.108` 的 S0 表改与表级用例(提交 `578c617de6`、`8311639d17`、`f8c0de2157`):`exec/parse2/forward-result.tsv` 观察值 `1-2 -> FW.full`、`0 -> FW.capture`(相等即拒),新增表级检查 `tests/check-forward-limit-table.cx`(3 断言 PASS);产品级三档自红检查未入车。
- (4) `0.0.103`(占位):<待五夹具实测数字验收;达标后入车,否则本车不含 0.0.103>。
- (5) 移出说明:原 `0.0.106` 的 `CXM_MAXU 256` 改动已改入第二车 `0.0.107`,不在本版。

## 已知缺口

- (1) `forward` 调用上限 8192 在产品内不可达:仅表级证据(`check-forward-limit-table`)证明 `exec/parse2/forward-result.tsv` 相等即拒;当前产品约 354 个不同被调转发符号处即出现异常(355 处生成 `forward-entry.c` 畸形、356-412 报 `model driver: cli`、>=413 段错误),未修,后续跟踪在 `0.0.126`。
- (2) 空目标返回边:在普通 CLI 无入口,但保留为显式 typed 宿主的潜在影响,需注意。
- (3) 升级后首轮运行缓存全冷与材料身份一次性失效:因闭包摘要与 `pinscan` 变更,升级至 `0.0.106` 后首轮运行缓存为全冷态,既有材料身份一次性失效,需首轮重建。
- (4) 未验证项数量:<待整链严格证据统计 UNKNOWN/未验证项数量后回填>;全套 `s9` `daily`/`release` 长跑未执行,不宣称全批发布资格。

## 回滚方案

公开后如需撤回,按 `release/rollback.txt` 执行:(1) 将 `v0.0.106` release 转回 draft 并把 Latest 移回 `v0.0.105` 后读回;(2) 在 `main` 按逆序前向 `git revert` 相关提交并推送,绝不删 tag 或 release;(3) 使用 `tools/install-ape.cx` 从既有 `0.0.105` 备份回装本机 Root 并核对版本。
