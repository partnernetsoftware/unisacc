<!-- 本文件由 tools/surface-doc-gen.cx 自 build/std-surface.tsv 生成,勿手改 -->

# API: ext.memx

本文件由 tools/surface-doc-gen.cx 自 build/std-surface.tsv 生成,勿手改。列义见该表文件头。

| name | signature | analogue | errors | lnx | osx | win | status | note | stab | since | own | test |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| ar_new | int ar_new(long limit); | Rust(bumpalo) Bump::new | -EINVAL | impl | impl | impl | exists | - | incubating | 0.1.0 | none | - |
| ar_alloc | void *ar_alloc(int a, long n); | Rust(bumpalo) Bump::alloc | null | impl | impl | impl | exists | 0 表失败或超 limit | incubating | 0.1.0 | none | - |
| ar_dup | char *ar_dup(int a, const char *src, long n); | Rust(bumpalo) alloc_str | null | impl | impl | impl | exists | 0 表失败 | incubating | 0.1.0 | none | - |
| ar_drop | int ar_drop(int a); | Rust drop(Bump) | -EBADF | impl | impl | impl | exists | - | incubating | 0.1.0 | none | - |
