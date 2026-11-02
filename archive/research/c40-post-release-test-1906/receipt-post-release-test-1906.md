# 回执：v0.0.40 已装 unisacc.com 代裁编排续测（1906）

墙钟：2026-10-11 19:09 → 19:12 +0800（m4pro）。

## 授源 / 边界
消息入口：m4pro 已装 GitHub v0.0.40 Latest unsigned（target `1db30835`，SHA `50ab4c21…`，2243662 B）；`--version` 0.0.40；冒烟 -o/run rc0。`build.json` 旧记录以新二进制为准。
**禁**：扩大发布 / 补签 / 切资产。与 0.0.41 K6 **并行不抢窗**（续测可读窗；三 paste 落盘，未强占忙窗）。

## 身份钉
| 项 | 值 |
|---|---|
| 宿主 | m4pro.local · Darwin 25.5.0 · arm64 · macOS 26.5.1 (25F80) |
| tip | `cfabc81bde2d16b10382a2ad29e7baefe014f3a3`（product src 自 `v0.0.40`/`1db30835` 未变） |
| `v0.0.40` | `1db3083506e7af178ed604052532d4226511ad01` |
| `rc/v0.0.40`（本机 tip） | `a82b9bda598557c1c854a7fcb4baec154aa726da`（**与公开回执 rc=1db30835 漂移；只并列，本刀不改签/不 bump tag**） |
| unisacc.com sha256 | `50ab4c213feedb2f1a4f1f39c8bfca7d8d63c74a1d6b50915db8ceb233f5cf58` |
| bytes / version | 2243662 / `unisacc 0.0.40` |
| build.json | **STALE**（artifact `b7128a0f…` / 2225534 B）；**以二进制为准**，本刀不改 sidecar |

## 测了什么 → 绿/红

### 1) bootstrap B=C=U
| 路径 | 结果 |
|---|---|
| **A. UA=已装 unisacc.com** | **红** B=1 U=0（约 18s）。B：`model driver: object output (-c -b) for this target is not on the product route yet`。U tape=`9266b52f…` 421304 lines（=1532 历史 tape）。**结论**：Darwin 上产品 .com **不能**作 bootstrap UA；固定点仍走参考 UA。 |
| **B. 参考 UA（`ua_ready`→/tmp/ua_ref）** | **绿** `ok B=C=U 257c4ef6944103ef (421304 lines)` / rc0 / fixed point reached。三 tape sha256 全 `9266b52f953646af278b9fb28862e873da51728336280e37dac10fcd175424a1`（=1532）。 |

### 2) acceptance 抽样
| 项 | 结果 |
|---|---|
| `ACCEPT_CASE=6-fold` | **绿** rc0（7× 6/6 match） |
| `ACCEPT_CASE=6-fault` | **绿** rc0（fault under built 4/6） |
| `10-*` 无 ACCEPT_STATE | rc2 缺态（预期；非产品红） |
| `10-Btape→Bimage→Ctape→Uimage→compare` + ACCEPT_STATE | **绿** 全 rc0；`B = C = U same`；tape 同 `9266b52f…` |

### 3) 常见 -o/run（已装 .com）
8 例 ×（`-run` + `-o`+exec，run==native）：**绿** pass=16 fail=0。  
hello/fact/fib/ptr/switch/struct/do/host 全 rc0。

## 卡点 / 另记（不扩本刀）
1. **产品作 UA bootstrap**：Darwin 产品路由无 -c/-b object；固定点证书用参考 UA（与 1532 同构）。非发布阻塞，属产品/参考路径分账。
2. **`rc/v0.0.40` tip 漂移** a82b9bda ≠ 公开回执 1db30835；本刀不改 tag、不扩发布。
3. **`unisacc.com.build.json` stale**：只记账，不改资产。
4. **签 / publish.sh / 全量 `tests/release.sh --com`**：仍 OPEN（与 r40 pending 同）；本刀禁扩。
5. **窗**：未抢 cc-unisacc/K6；paste 三份已落，窗闲可投。

## 禁项自检
- [x] 未扩大发布 / 未补签 / 未切资产
- [x] 未动 Latest / Draft / seal
- [x] 未与 K6 抢窗优先
- [x] build.json 以二进制为准只记 stale
- [x] 大 tape/镜像已哈希后剔除出 research 目录

## 附件
见同目录 `*-SUMMARY.txt` / `*-log` / `paste-*.txt` / `HASHES.sha256`。
