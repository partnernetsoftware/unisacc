<!-- 本文件由 tools/surface-doc-gen.cx 自 build/std-surface.tsv 生成,勿手改 -->

# API: ext.crypto

本文件由 tools/surface-doc-gen.cx 自 build/std-surface.tsv 生成,勿手改。列义见该表文件头。

| name | signature | analogue | errors | lnx | osx | win | status | note | stab | since | own | test |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| cs_sha256_t | typedef struct { unsigned h[8]; unsigned char buf[CS_BLOCK]; long n; long total; } cs_sha256_t; | Node crypto.Hash; Rust(sha2) Sha256 | none | impl | impl | impl | exists | - | incubating | 0.1.0 | none | - |
| cs_sha256_init | int cs_sha256_init(cs_sha256_t *s); | Node crypto.createHash | -ENOMEM | impl | impl | impl | exists | - | incubating | 0.1.0 | none | - |
| cs_sha256_update | int cs_sha256_update(cs_sha256_t *s, const void *p, long n); | Node hash.update | -ENOMEM | impl | impl | impl | exists | - | incubating | 0.1.0 | none | - |
| cs_sha256_final | int cs_sha256_final(cs_sha256_t *s, unsigned char out[32]); | Node hash.digest | -ENOMEM | impl | impl | impl | exists | - | incubating | 0.1.0 | none | - |
| cs_sha256 | int cs_sha256(const void *data, long n, unsigned char out[32]); | Rust(sha2) Sha256::digest | -ENOMEM | impl | impl | impl | exists | - | incubating | 0.1.0 | none | - |
| cs_sha256_hex | int cs_sha256_hex(const void *data, long n, char hex[65]); | Node hash.digest(hex) | -ENOMEM | impl | impl | impl | exists | - | incubating | 0.1.0 | none | - |
| sy_sha256_hex | int sy_sha256_hex(const void *data, long n, char hex[65]); | 同 cs_sha256_hex | -ENOMEM | impl | impl | impl | alias-of:cs_sha256_hex | L0 sys.sha 的名字,留在 cx.base 供核心自用 | incubating | 0.1.0 | none | - |
| cs_hmac_sha256 | int cs_hmac_sha256(const void *key, long klen, const void *data, long dlen, unsigned char out[32]); | Node crypto.createHmac | -ENOMEM | impl | impl | impl | exists | - | incubating | 0.1.0 | none | - |
| cs_hmac_sha256_hex | int cs_hmac_sha256_hex(const void *key, long klen, const void *data, long dlen, char hex[65]); | Node hmac.digest(hex) | -ENOMEM | impl | impl | impl | exists | - | incubating | 0.1.0 | none | - |
| cs_ct_eq | int cs_ct_eq(const void *a, const void *b, long n); | Node crypto.timingSafeEqual | 0/1 | impl | impl | impl | exists | - | incubating | 0.1.0 | none | - |
| cs_ct_wipe | void cs_ct_wipe(void *p, long n); | Rust(zeroize) Zeroize | none | impl | impl | impl | exists | - | incubating | 0.1.0 | none | - |
| cs_hex_len | long cs_hex_len(long n); | 无 | none | impl | impl | impl | exists | - | incubating | 0.1.0 | none | - |
| cs_hex_encode | int cs_hex_encode(const void *in, long n, char *out, long outcap); | Node Buffer.toString hex | -EINVAL | impl | impl | impl | exists | 下沉后成为 tx_hex_encode 的别名 | incubating | 0.1.0 | none | - |
| cs_hex_decode | int cs_hex_decode(const char *in, long n, void *out, long outcap); | Node Buffer.from hex | -EINVAL | impl | impl | impl | exists | 下沉后成为 tx_hex_decode 的别名 | incubating | 0.1.0 | none | - |
| cs_b64_len | long cs_b64_len(long n); | 无 | none | impl | impl | impl | exists | - | incubating | 0.1.0 | none | - |
| cs_b64_encode | int cs_b64_encode(const void *in, long n, char *out, long outcap); | Node Buffer.toString base64 | -EINVAL | impl | impl | impl | exists | 下沉后成为 tx_b64_encode 的别名 | incubating | 0.1.0 | none | - |
| cs_b64_decode | int cs_b64_decode(const char *in, long n, void *out, long outcap); | Node Buffer.from base64 | -EINVAL | impl | impl | impl | exists | 下沉后成为 tx_b64_decode 的别名 | incubating | 0.1.0 | none | - |
