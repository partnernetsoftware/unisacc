<!-- 本文件由 tools/surface-doc-gen.cx 自 build/std-surface.tsv 生成,勿手改 -->

# API: cx.text

本文件由 tools/surface-doc-gen.cx 自 build/std-surface.tsv 生成,勿手改。列义见该表文件头。

| name | signature | analogue | errors | lnx | osx | win | status | note | stab | since | own | test |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| tx_len | long tx_len(const char *s, long cap); | C strnlen; Rust str::len | none | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| tx_same | int tx_same(const char *a, long an, const char *b, long bn); | Rust slice == | none | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| tx_eq | int tx_eq(const char *a, const char *b, long cap); | C strncmp == 0 | none | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| tx_eqz | int tx_eqz(const char *a, const char *b); | C strcmp == 0 | none | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| tx_prefixz | int tx_prefixz(const char *a, const char *lit, long n); | Rust str::starts_with | none | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| tx_prefix | int tx_prefix(const char *s, long n, const char *p, long pn); | Rust str::starts_with; JS startsWith | none | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| tx_suffix | int tx_suffix(const char *s, long n, const char *p, long pn); | Rust str::ends_with; JS endsWith | none | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| tx_contains | int tx_contains(const char *s, long n, const char *p, long pn); | Rust str::contains; JS includes | none | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| tx_count | long tx_count(const char *s, long n, const char *p, long pn); | Rust str::matches().count() | none | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| tx_chr | long tx_chr(const char *s, long n, int c); | C memchr; Rust str::find(char) | miss:-1 | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| tx_rchr | long tx_rchr(const char *s, long n, int c); | C memrchr; Rust str::rfind(char) | miss:-1 | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| tx_cmp | int tx_cmp(const char *a, long an, const char *b, long bn); | Rust Ord::cmp; JS localeCompare(字节序) | none | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| tx_find | long tx_find(const char *s, long n, const char *p, long pn); | Rust str::find; JS indexOf | miss:-1 | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| tx_rfind | long tx_rfind(const char *s, long n, const char *p, long pn); | Rust str::rfind; JS lastIndexOf | miss:-1 | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| tx_copy | int tx_copy(char *dst, long cap, const char *src, long n); | C strlcpy(带界) | -ERANGE | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| tx_fields | long tx_fields(const char *s, long n, int c); | Rust split(c).count() | none | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| tx_field | int tx_field(const char *s, long n, int c, long idx, long *off, long *len); | Rust split(c).nth(i) | -ERANGE | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| tx_trimc | int tx_trimc(const char *s, long n, int c, long *off, long *len); | Rust trim_matches(c) | none | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| tx_trim | int tx_trim(const char *s, long n, long *off, long *len); | Rust str::trim; JS trim | none | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| tx_iter_t | typedef struct { const char *s; long n; long pos; long off; long len; int done; } tx_iter_t; | Rust str::Split / str::Lines | none | impl | impl | impl | new | 不分配;off/len 指回原串 | pre | 0.1.0 | none | - |
| tx_iter_init | void tx_iter_init(tx_iter_t *it, const char *s, long n); | Rust str::split 起点 | none | impl | impl | impl | new | - | pre | 0.1.0 | none | - |
| tx_split_next | int tx_split_next(tx_iter_t *it, const char *sep, long sepn); | Rust Split::next; JS split | -EINVAL;ret:1-item-0-end | impl | impl | impl | new | 保留空段;末尾分隔符后有一个空段;空分隔符 | pre | 0.1.0 | none | - |
| tx_line_next | int tx_line_next(tx_iter_t *it); | Rust Lines::next | ret:1-item-0-end | impl | impl | impl | new | 行尾认 LF 与 CRLF;末行无换行也给出;末尾换行后不多给空行 | pre | 0.1.0 | none | - |
| tx_replace | long tx_replace(char *dst, long cap, const char *s, long n, const char *from, long fn, const char *to, long tn); | Rust str::replace; JS replaceAll | -ERANGE;-EINVAL;ret:needlen | impl | impl | impl | new | dst 不得与 s 重叠;装不下(dst 不动);空 from | pre | 0.1.0 | none | - |
| tx_lower | int tx_lower(char *s, long n); | Rust make_ascii_lowercase | none | impl | impl | impl | new | 只动 ASCII;locale 在 OUT | pre | 0.1.0 | none | - |
| tx_upper | int tx_upper(char *s, long n); | Rust make_ascii_uppercase | none | impl | impl | impl | new | 只动 ASCII | pre | 0.1.0 | none | - |
| tx_ieq | int tx_ieq(const char *a, long an, const char *b, long bn); | Rust eq_ignore_ascii_case | none | impl | impl | impl | new | - | pre | 0.1.0 | none | - |
| tx_utf8_valid | int tx_utf8_valid(const char *s, long n, long *bad); | Rust str::from_utf8 | 0/1;-EINVAL | impl | impl | impl | new | 拒绝过长编码、代理区、大于 U+10FFFF;*bad 是首个坏字节偏移 | pre | 0.1.0 | none | - |
| tx_utf8_next | int tx_utf8_next(const char *s, long n, long *pos, unsigned *cp); | Rust Chars::next; JS codePointAt | -EILSEQ;ret:1-item-0-end | impl | impl | impl | new | - | pre | 0.1.0 | none | - |
| tx_utf8_put | int tx_utf8_put(char *dst, long cap, unsigned cp); | Rust char::encode_utf8; JS String.fromCodePoint | -ERANGE;-EILSEQ;ret:needlen | impl | impl | impl | new | - | pre | 0.1.0 | none | - |
| tx_utf8_count | long tx_utf8_count(const char *s, long n); | Rust chars().count() | -EILSEQ | impl | impl | impl | new | - | pre | 0.1.0 | none | - |
| tx_hex_encode | int tx_hex_encode(const void *in, long n, char *out, long outcap); | Node Buffer.toString hex | -ERANGE | impl | impl | impl | new | 实现自 ext/crypto/b64.cx 下沉;小写 | pre | 0.1.0 | none | - |
| tx_hex_decode | int tx_hex_decode(const char *in, long n, void *out, long outcap); | Node Buffer.from hex | -ERANGE;-EINVAL | impl | impl | impl | new | 大小写都收;奇数长或非十六进制 | pre | 0.1.0 | none | - |
| tx_b64_encode | int tx_b64_encode(const void *in, long n, char *out, long outcap); | Node Buffer.toString base64; JS btoa | -ERANGE | impl | impl | impl | new | 标准字母表带填充 | pre | 0.1.0 | none | - |
| tx_b64_decode | int tx_b64_decode(const char *in, long n, void *out, long outcap); | Node Buffer.from base64; JS atob | -ERANGE;-EINVAL | impl | impl | impl | new | 严格:不收空白与缺填充 | pre | 0.1.0 | none | - |
| tx_from_i64 | int tx_from_i64(char *dst, long cap, int64_t v, int base); | Rust i64::to_string; JS Number.toString(radix) | -ERANGE;-EINVAL;ret:needlen | impl | impl | impl | new | base 不在 2..36 | pre | 0.1.0 | none | - |
| tx_from_u64 | int tx_from_u64(char *dst, long cap, uint64_t v, int base); | Rust u64::to_string | -ERANGE;-EINVAL;ret:needlen | impl | impl | impl | new | - | pre | 0.1.0 | none | - |
| tx_to_i64 | int tx_to_i64(const char *s, long n, int base, int64_t *out); | Rust i64::from_str_radix; JS parseInt | -EINVAL;-ERANGE | impl | impl | impl | new | 整段必须全是数字,带界 span,不认前后空白;空或有非数字;溢出 | pre | 0.1.0 | none | - |
| tx_to_u64 | int tx_to_u64(const char *s, long n, int base, uint64_t *out); | Rust u64::from_str_radix | -EINVAL;-ERANGE | impl | impl | impl | new | - | pre | 0.1.0 | none | - |
| tx_vsnprintf | int tx_vsnprintf(char *dst, size_t cap, const char *fmt, va_list ap); | C vsnprintf | ret:needlen | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| tx_snprintf | int tx_snprintf(char *dst, size_t cap, const char *fmt, ...); | C snprintf; QuickJS std.sprintf | ret:needlen | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
