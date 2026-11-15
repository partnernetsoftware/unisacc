<!-- 本文件由 tools/surface-doc-gen.cx 自 build/std-surface.tsv 生成,勿手改 -->

# API: ext.textx

本文件由 tools/surface-doc-gen.cx 自 build/std-surface.tsv 生成,勿手改。列义见该表文件头。

| name | signature | analogue | errors | lnx | osx | win | status | note | stab | since | own | test |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| tx_atol | long tx_atol(const char *s); | C atol | none | impl | impl | impl | exists | 无错误通道;新代码用 tx_to_i64 | legacy | 0.1.0 | none | - |
| tx_strtol | long tx_strtol(const char *s, char **end, int base); | C strtol | none | impl | impl | impl | exists | - | legacy | 0.1.0 | none | - |
| tx_strtoul | unsigned long tx_strtoul(const char *s, char **end, int base); | C strtoul | none | impl | impl | impl | exists | - | legacy | 0.1.0 | none | - |
| tx_strtod | double tx_strtod(const char *s, char **end); | C strtod; JS parseFloat | none | impl | impl | impl | exists | - | legacy | 0.1.0 | none | - |
| tx_strtol_r | int tx_strtol_r(const char *s, char **end, int base, long *out); | Rust str::parse | -ENOMEM | impl | impl | impl | exists | - | incubating | 0.1.0 | none | - |
| tx_strtoul_r | int tx_strtoul_r(const char *s, char **end, int base, unsigned long *out); | Rust str::parse | -ENOMEM | impl | impl | impl | exists | - | incubating | 0.1.0 | none | - |
| tx_strtod_r | int tx_strtod_r(const char *s, char **end, double *out); | Rust str::parse::<f64> | -ENOMEM | impl | impl | impl | exists | - | incubating | 0.1.0 | none | - |
| sb_new | sb *sb_new(void); | Rust String::new | null | impl | impl | impl | exists | 0 表分配失败 | incubating | 0.1.0 | none | - |
| sb_free | void sb_free(sb *b); | Rust drop(String) | none | impl | impl | impl | exists | - | incubating | 0.1.0 | none | - |
| sb_len | long sb_len(const sb *b); | Rust String::len | none | impl | impl | impl | exists | - | incubating | 0.1.0 | none | - |
| sb_capacity | long sb_capacity(const sb *b); | Rust String::capacity | none | impl | impl | impl | exists | - | incubating | 0.1.0 | none | - |
| sb_clear | int sb_clear(sb *b); | Rust String::clear | -ENOMEM | impl | impl | impl | exists | - | incubating | 0.1.0 | none | - |
| sb_reserve | int sb_reserve(sb *b, long n); | Rust String::try_reserve | -ENOMEM | impl | impl | impl | exists | - | incubating | 0.1.0 | none | - |
| sb_truncate | int sb_truncate(sb *b, long n); | Rust String::truncate | -ENOMEM | impl | impl | impl | exists | - | incubating | 0.1.0 | none | - |
| sb_putn | int sb_putn(sb *b, const char *s, long n); | Rust String::push_str | -ENOMEM | impl | impl | impl | exists | - | incubating | 0.1.0 | none | - |
| sb_putc | int sb_putc(sb *b, int c); | Rust String::push | -ENOMEM | impl | impl | impl | exists | - | incubating | 0.1.0 | none | - |
| sb_puts | int sb_puts(sb *b, const char *s); | Rust String::push_str | -ENOMEM | impl | impl | impl | exists | - | incubating | 0.1.0 | none | - |
| sb_cstr | const char *sb_cstr(sb *b); | Rust String::as_str | null | impl | impl | impl | exists | 0 表失败 | incubating | 0.1.0 | none | - |
| sb_join | int sb_join(sb *b, const char *const *s, long n, const char *sep); | Rust slice::join; JS Array.join | -ENOMEM | impl | impl | impl | exists | - | incubating | 0.1.0 | none | - |
| sb_insert | int sb_insert(sb *b, long index, const char *s, long n); | Rust String::insert_str | -ENOMEM | impl | impl | impl | exists | - | incubating | 0.1.0 | none | - |
| dz_bound | long dz_bound(long n); | zlib deflateBound | none | impl | impl | impl | exists | - | incubating | 0.1.0 | none | - |
| dz_gzip_bound | long dz_gzip_bound(long n); | zlib deflateBound(gzip) | none | impl | impl | impl | exists | - | incubating | 0.1.0 | none | - |
| dz_deflate | long dz_deflate(const unsigned char *in, long n, unsigned char *out, long cap); | zlib deflate; Node zlib.deflateRawSync | -EINVAL | impl | impl | impl | exists | - | incubating | 0.1.0 | none | - |
| dz_gzip | long dz_gzip(const unsigned char *in, long n, unsigned char *out, long cap); | Node zlib.gzipSync | -EINVAL | impl | impl | impl | exists | - | incubating | 0.1.0 | none | - |
| dz_crc32 | unsigned long dz_crc32(unsigned long crc, const unsigned char *p, long n); | zlib crc32 | none | impl | impl | impl | exists | - | incubating | 0.1.0 | none | - |
