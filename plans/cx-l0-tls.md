# cx 原生 TLS：卡点、分片与验收

状态：T2.1–T2.4 已落地并过向量（SHA-256/HMAC、base64/hex、HKDF-SHA256、ChaCha20-Poly1305），另加 `cs_ct_eq`/`cs_ct_wipe`；T2.5–T2.10 未实现。本文替代"cx 没有 TLS，所以用 libcurl"这种一句话结论。

## 一句话回答"卡在哪个环节"

**不卡在编译器。** 2026-10-31 用 `unisacc.com` 直接 `-run` 编出 `cx/crypto/sha256.cx`（SHA-256 + HMAC-SHA-256），FIPS 180-4 与 RFC 4231 全部向量通过，并与宿主 `sha256sum` / `openssl dgst` 逐字节对拍一致（`d7a8fbb3…`、`f7bc83f4…`）。这证明 32 位整数运算、常量表、大数组、结构体状态、多单元模块在 `unisacc.com` 上是够用的，`gcc_cx.com` / `clang_cx.com` 走同一套前端与 lowering，没有额外能力门槛。

**也不卡在网络层。** `cx/sys/net.cx` 的 `sy_tcp_dial` 能直连 443 拿到加密字节流；TLS 在应用层，是"拿到字节之后怎么解"的问题。

**卡在密码学原语面。** TLS 1.3 不是"部分可用"的协议：握手要能算密钥、验签名、认证书，缺任何一片都握不上手。仓内在 2026-10-31 前的实际存量：

| 原语 | 2026-10-31 前 | 现在 |
|---|---|---|
| SHA-256 | 有：`include/unisa_sha256.h`（71 行纯计算）、`cx/sys/sha.cx`（Linux AF_ALG，失败回退） | 有 |
| HMAC-SHA-256 | **无** | 有：`cx/crypto/sha256.cx`（本版新增） |
| HKDF-SHA256（RFC 5869） | **无** | 无（HMAC 已就位，可直接叠） |
| ChaCha20-Poly1305（RFC 8439） | **无** | 无 |
| AES-128/256-GCM | **无** | 无 |
| X25519（RFC 7748） | **无** | 无 |
| P-256 ECDSA 验签 | **无** | 无 |
| RSA / modexp | **无** | 无 |
| ASN.1 DER / X.509 | **无** | 无 |
| base64 | **无** | 无 |
| deflate / inflate | 有：`cx/text/deflate.cx`、`cx/runtime/codec.cx`（`cxcodec_inflate`） | 有 |
| 根证书 | 宿主 `/etc/ssl/certs/ca-certificates.crt` 存在 | 同 |

核对方式：`grep -rlniE 'chacha|poly1305|x25519|curve25519|aes_gcm|x509|ecdsa|hmac' cx/ src/ include/ tools/`，2026-10-31 前零命中。

## 为什么不"直接调 OpenSSL / libcurl 就算完"

libcurl + OpenSSL 是可审查的宿主依赖，但那是**宿主 TLS**，不是 cx 内 TLS：交易对手是谁、证书怎么验、密钥怎么派生，全在宿主库里，`cx/` 看不到也测不了。把它当交付物，等于把 0.0.45「模块可外用」的合同架空。

允许的用法只有两种：一是 T1 的可替换传输实现之一；二是 T2 的**对照预言机**（同一个 ClientHello 让两边各解一遍，比字节）。不冒充完成。

## T1：先收敛传输接口（零新密码学，可立即做）

目标是把 HTTPS 从"csih 里散落的 libcurl 调用"收成一个窄接口，让传输可替换：

```
transport_post(url, hdrs, hlen, body, blen, out, outcap) -> 0 | -errno | 具名拒绝
```

- 一个实现是 libcurl（现状，`apps/csih/net.cx` + `sys.ffi`）。
- 另一个至少再有一个实现（本地 TLS 终结的 http 端点），用来证明接口没漏出 libcurl 语义。
- 验收：csih 现有批次仍 PASS；替换实现后调用点零改动；HTTPS 失败原因（证书、超时、非 2xx）仍是具名错误，不退化成空串。

T1 不新增密码学，但它是 T2 能替换进来的前提。

## T2：cx 原生 TLS 1.3，按原语分片

顺序按依赖排，每片独立可测、独立提交：

| 片 | 内容 | 验收向量 |
|---|---|---|
| T2.1 | SHA-256 + HMAC-SHA-256 | FIPS 180-4 B.1–B.3、RFC 4231 §4；**已过**，与 `sha256sum`/`openssl` 对拍（`cx/crypto/sha256.cx`，批次 `cx-l0-crypto-sha256`） |
| T2.2 | base64 / hex 编解码 | RFC 4648 §10 向量；拒绝非法字符、长度 mod 4 ≠ 0、非规范填充位、中段填充、奇数 hex 长度；容量精确边界。**已过**（`cx/crypto/b64.cx`，批次 `cx-l0-crypto-b64`） |
| T2.3 | HKDF-SHA256 + TLS1.3 Expand-Label | RFC 5869 A.1–A.3（含 82 字节跨三块、空 salt/info）；Expand-Label 值与独立 HKDF-Expand 对拍；`L`=0 与 >255·HashLen、label>249、context>255 拒绝。**已过**（`cx/crypto/hkdf.cx`，批次 `cx-l0-crypto-hkdf`） |
| T2.4 | ChaCha20 + Poly1305 + AEAD | RFC 8439 §2.4.2 块函数、§2.5.2 与 A.3（r=0／s=0／r=2）、§2.8.2 seal/open 与伪造 tag/aad/长度/nonce 拒绝、原地 open、空明文与空 aad。**已过**（`cx/crypto/chacha20poly1305.cx`，批次 `cx-l0-crypto-chacha20poly1305`） |
| T2.5 | AES-128/256-GCM | NIST GCM 测试向量（16/12/8/4 字节 nonce，tag 截断拒绝） |
| T2.6 | X25519 | RFC 7748 §5.2、§6.1；低阶点拒绝 |
| T2.7 | P-256 ECDSA 验签 | NIST CAVP 向量；DER 签名编码与 r/s 越界拒绝 |
| T2.8 | ASN.1 DER + X.509 最小解析 | 真实证书链（宿主 `ca-certificates.crt` 抽样）：签名验证、有效期、basicConstraints、hostname 匹配；畸形 DER 必须拒绝而不是越界 |
| T2.9 | TLS 1.3 握手 | RFC 8446 Appendix A 简化握手轨迹；`Finished` 校验失败必须拒绝；transcript hash 与 reference 一致 |
| T2.10 | 记录层与会话 | 与 libcurl 对同一站点对拍：应用数据字节相等；重协商/会话票据失败路径有测试 |

每片规则：

- 一个 `tools/check-crypto-*.cx`，带 `ok(int, name)` 与 `PASS`/`FAIL`，任一 FAIL 退出 1；测试向量写进检查文件，不写进模块。
- 未过向量记 **UNKNOWN**，不记 PASS。带"看起来对"的实现不进主线。
- 模块按「层就是目录」放 `cx/crypto/`，六项棘轮为 0：无 `#include`、无直接 libc、无平台 `#if`、无 `__syscallN`。
- 平台加速（Linux AF_ALG 的 `skcipher`/`aead`）只放在 L0 静态帮手，失败即回退纯 cx 路径，导出函数不带条件编译；`cx/sys/sha.cx` 已是这个形状。
- 常数时间：验签与 MAC 比较必须走常数时间比较；`mm_same` 是提前返回，不适用于 tag 比较，需在 `cx/crypto/` 内提供 `cs_ct_eq`。

## 体量与顺序判断

- 纯 cx、无 SIMD 的 AES-GCM 与 ChaCha20 大约几十 MB/s 量级，对 harness 的 HTTP 请求量足够；性能不是卡点，正确性才是。
- 建议顺序：T1 → T2.2 → T2.3 → T2.4 → T2.6 → T2.7 → T2.8 → T2.5 → T2.9 → T2.10。先拿下 ChaCha20-Poly1305 + X25519 + ECDSA + X.509 就能与相当一部分站点握手；AES-GCM 排在后面是因为它最容易被硬件加速路径掩盖错误。
- 每片是一个可提交单元，不合并成大提交；`tools/v0.0.46` 的里程碑里 T2 不与 harness 阻塞顺序耦合。

## 与其他计划的关系

- `plans/v0.0.46.md`：T1 是 M2 的前置；T2 与 harness 并行，不是 harness 的验收前提。
- 0.0.45「模块可外用」：T2 是它最硬的一次外部验证——一个全新的、无宿主库的模块族，只靠 `#pragma unisa use` 被上层调用。
- 不因此改写 C 基础实现与归档源码；新增只在 `cx/`。
