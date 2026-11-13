<!-- 本文件由 tools/surface-doc-gen.cx 自 build/std-surface.tsv 生成,勿手改 -->

# API: ext.tsvx

本文件由 tools/surface-doc-gen.cx 自 build/std-surface.tsv 生成,勿手改。列义见该表文件头。

| name | signature | analogue | errors | lnx | osx | win | status | note | stab | since | own | test |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| tsm_buf | typedef struct {char *p;size_t n,cap;int failed;} tsm_buf; | Rust String | none | impl | impl | impl | exists | - | incubating | 0.1.0 | none | - |
| tsm_put | void tsm_put(tsm_buf *b,const char *s,size_t n); | Rust String::push_str | ret:failed | impl | impl | impl | exists | - | incubating | 0.1.0 | none | - |
| tsm_puts | void tsm_puts(tsm_buf *b,const char *s); | Rust String::push_str | ret:failed | impl | impl | impl | exists | - | incubating | 0.1.0 | none | - |
| tsm_merge | int tsm_merge(const char *header,int columns,int keymode,const char *base,size_t bn,const char *ours,size_t on,const char *theirs,size_t tn,tsm_buf *out,tsm_buf *conflicts); | git merge-file 的按键 TSV 版 | -EINVAL | impl | impl | impl | exists | - | incubating | 0.1.0 | none | - |
| tsm_merge_schema | int tsm_merge_schema(const char *header,int columns,int keymode,const char *base,size_t bn,const char *ours,size_t on,const char *theirs,size_t tn,tsm_buf *out,tsm_buf *conflicts,int inherited_min_columns); | 同 tsm_merge,带列数迁移 | -EINVAL | impl | impl | impl | exists | - | incubating | 0.1.0 | none | - |
| tv_row_next | int tv_row_next(tx_iter_t *it, int *cols); | Rust(csv) Reader::records; Node 无 | -EINVAL;ret:1-item-0-end | impl | impl | impl | new | 跳过空行与 # 注释行;cols 是该行 TAB 字段数;行内有 NUL | incubating | 0.1.0 | none | - |
| tv_field | int tv_field(const tx_iter_t *it, int idx, long *off, long *len); | Rust(csv) StringRecord::get | -ERANGE | impl | impl | impl | new | 不转义、不引号;字段指回原串;列越界 | incubating | 0.1.0 | none | - |
| tv_expect_cols | int tv_expect_cols(const char *s, long n, int cols, long *bad_line); | 无(表 lint) | -EINVAL | impl | impl | impl | new | 整表每行恰好 cols 列;并给出首个坏行号 | incubating | 0.1.0 | none | - |
