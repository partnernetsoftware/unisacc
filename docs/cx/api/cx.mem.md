<!-- 本文件由 tools/surface-doc-gen.cx 自 build/std-surface.tsv 生成,勿手改 -->

# API: cx.mem

本文件由 tools/surface-doc-gen.cx 自 build/std-surface.tsv 生成,勿手改。列义见该表文件头。

| name | signature | analogue | errors | lnx | osx | win | status | note | stab | since | own | test |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| mm_alloc | void *mm_alloc(long n); | Rust alloc; C malloc | null | impl | impl | impl | exists | 0 表失败 | pre | 0.1.0 | open:heap close:mm_free | - |
| mm_zalloc | void *mm_zalloc(long n); | C calloc | null | impl | impl | impl | exists | 0 表失败 | pre | 0.1.0 | open:heap close:mm_free | - |
| mm_grow | void *mm_grow(void *p, long n); | C realloc | null | impl | impl | impl | exists | 0 表失败且原块仍有效 | pre | 0.1.0 | none | - |
| mm_free | void mm_free(void *p); | C free | none | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| mm_strdup | char *mm_strdup(const char *s); | C strdup | null | impl | impl | impl | exists | 0 表失败 | pre | 0.1.0 | none | - |
| mm_alloc_n | void *mm_alloc_n(long count, long size); | Rust Vec::with_capacity; C calloc 的溢出检查 | null | impl | impl | impl | exists | 清零;0 表失败、count/size<=0 或 count*size 溢出;调用者 mm_free | pre | 0.1.0 | open:heap close:mm_free | cx-l0-buffer |
| mm_copy | int mm_copy(void *dst, long dcap, const void *src, long n); | Rust copy_from_slice | -ERANGE | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| mm_move | int mm_move(void *dst, long dcap, const void *src, long n); | Rust copy_within; C memmove | -ERANGE | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| mm_fill | int mm_fill(void *dst, long dcap, int byte, long n); | Rust slice::fill; Node Buffer.fill | -ERANGE | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| mm_same | int mm_same(const void *a, const void *b, long n); | Rust slice ==; Node Buffer.equals | none | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| mm_cmp | int mm_cmp(const void *a, const void *b, long n); | C memcmp; Node Buffer.compare | none | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| mm_find | long mm_find(const void *h, long hn, const void *needle, long nn); | C memmem; Node Buffer.indexOf | miss:-1 | impl | impl | impl | exists | 空针返回 0;坏参数(负长、非零长配 null)也返回 -1;允许内嵌 NUL | pre | 0.1.0 | none | cx-l0-buffer |
| mm_vec | typedef struct { void *data; long len; long cap; long elem; } mm_vec; | Rust Vec<T> | none | impl | impl | impl | exists | 唯一的带元素宽度 vec;元素按值拷入;调用者拥有结构,数据块只由 mm_vec_free 释放 | pre | 0.1.0 | none | cx-l0-buffer |
| mm_vec_init | void mm_vec_init(mm_vec *v, long elem); | Rust Vec::new | none | impl | impl | impl | exists | 不分配;elem<1 存 0,其后增长类操作 -EINVAL | pre | 0.1.0 | open:vec close:mm_vec_free | cx-l0-buffer |
| mm_vec_reserve | int mm_vec_reserve(mm_vec *v, long n); | Rust Vec::try_reserve | -EINVAL;-ENOMEM;-EOVERFLOW | impl | impl | impl | exists | 倍增(起 4);n*elem 溢出 -EOVERFLOW;失败不改对象 | pre | 0.1.0 | none | cx-l0-buffer |
| mm_vec_push | int mm_vec_push(mm_vec *v, const void *item); | Rust Vec::push | -EINVAL;-ENOMEM | impl | impl | impl | exists | 按值拷入;null 或 elem 0 -EINVAL;溢出也归 -ENOMEM | pre | 0.1.0 | none | cx-l0-buffer |
| mm_vec_pop | int mm_vec_pop(mm_vec *v, void *item); | Rust Vec::pop | -EINVAL;ret:1-got-0-empty | impl | impl | impl | exists | item 可为 0(丢弃) | pre | 0.1.0 | none | cx-l0-buffer |
| mm_vec_at | void *mm_vec_at(const mm_vec *v, long i); | Rust slice::get | null | impl | impl | impl | exists | 指针在下一次 push/reserve 后失效;0 表越界或负索引 | pre | 0.1.0 | borrow | cx-l0-buffer |
| mm_vec_truncate | int mm_vec_truncate(mm_vec *v, long n); | Rust Vec::truncate | -EINVAL | impl | impl | impl | exists | n 不在 [0,len] -EINVAL;不缩容 | pre | 0.1.0 | none | cx-l0-buffer |
| mm_vec_free | void mm_vec_free(mm_vec *v); | Rust drop(Vec) | none | impl | impl | impl | exists | 置零 data/len/cap(保留 elem);重复释放无害 | pre | 0.1.0 | none | cx-l0-buffer |
| mm_map_new | int mm_map_new(int *m); | Rust HashMap<String, V>::new; JS Map | -EINVAL;-ENOMEM | impl | impl | impl | exists | 唯一的字符串键 map;句柄式(1..64,0 永不有效,同时最多 64 张,槽满 -ENOMEM);键拷入,值是调用者的指针;查找是哈希预筛的线性扫描 O(n),不是哈希表 | pre | 0.1.0 | open:map close:mm_map_free | cx-l0-buffer |
| mm_map_set | int mm_map_set(int m, const char *key, long kn, void *val, void **old); | Rust HashMap::insert; JS Map.set | -ENOMEM;-EBADF;-EINVAL | impl | impl | impl | exists | *old 得旧值或 0;键拷入(可含 NUL),值不拷贝不释放;负键长或 null 键配正长 -EINVAL;线性扫描 O(n) | pre | 0.1.0 | none | cx-l0-buffer |
| mm_map_get | int mm_map_get(int m, const char *key, long kn, void **val); | Rust HashMap::get; JS Map.get | -EBADF;-EINVAL;ret:1-yes-0-no | impl | impl | impl | exists | 未命中 *val=0;线性扫描 O(n) | pre | 0.1.0 | none | cx-l0-buffer |
| mm_map_del | int mm_map_del(int m, const char *key, long kn, void **old); | Rust HashMap::remove; JS Map.delete | -EBADF;-EINVAL;ret:1-yes-0-no | impl | impl | impl | exists | 后项前移,遍历游标作废;不释放值 | pre | 0.1.0 | none | cx-l0-buffer |
| mm_map_len | long mm_map_len(int m); | Rust HashMap::len; JS Map.size | -EBADF;ret:count | impl | impl | impl | exists | - | pre | 0.1.0 | none | cx-l0-buffer |
| mm_map_next | int mm_map_next(int m, long *cursor, const char **key, long *kn, void **val); | Rust HashMap::iter; JS Map.entries | -EBADF;-EINVAL;ret:1-item-0-end | impl | impl | impl | exists | 按插入顺序;遍历中增删后 cursor 作废;key 指向 map 自有内存,至下次 set 新键/del/free 有效;游标 null 或负 -EINVAL | pre | 0.1.0 | borrow | cx-l0-buffer |
| mm_map_free | int mm_map_free(int m); | Rust drop(HashMap) | -EBADF | impl | impl | impl | exists | 不释放值;重复释放 -EBADF | pre | 0.1.0 | none | cx-l0-buffer |
| mg_buf | typedef struct { unsigned char *data; long len, cap; } mg_buf; | Rust Vec<u8>; Node Buffer | none | impl | impl | impl | exists | - | pre | 0.1.0 | open:mg close:mg_free | - |
| mg_init | void mg_init(mg_buf *b); | Rust Vec::new | none | impl | impl | impl | exists | - | pre | 0.1.0 | open:mg close:mg_free | - |
| mg_reserve | int mg_reserve(mg_buf *b, long n); | Rust Vec::try_reserve | -ENOMEM | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| mg_push | int mg_push(mg_buf *b, int c); | Rust Vec::push | -ENOMEM | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| mg_append | int mg_append(mg_buf *b, const void *p, long n); | Rust Vec::extend_from_slice; Node Buffer.concat | -ENOMEM | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| mg_free | void mg_free(mg_buf *b); | Rust drop(Vec) | none | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| mg_move | int mg_move(mg_buf *dst, mg_buf *src); | Rust mem::take | -ENOMEM | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| mg_clear | int mg_clear(mg_buf *b); | Rust Vec::clear | -ENOMEM | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| mg_pop | int mg_pop(mg_buf *b, int *c); | Rust Vec::pop | -ENOENT | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| mg_truncate | int mg_truncate(mg_buf *b, long n); | Rust Vec::truncate | -ENOMEM | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| mg_insert | int mg_insert(mg_buf *b, long index, int c); | Rust Vec::insert | -ENOMEM | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| mg_remove | int mg_remove(mg_buf *b, long index, int *c); | Rust Vec::remove | -ENOMEM | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| mg_take | int mg_take(mg_buf *b, unsigned char **data, long *len, long *cap); | Rust Vec::into_raw_parts | -ENOMEM | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| ms_slice | typedef struct { const unsigned char *p; long n; } ms_slice; | Rust &[u8] | none | impl | impl | impl | exists | - | pre | 0.1.0 | borrow | - |
| ms_slice_mut | typedef struct { unsigned char *p; long n; } ms_slice_mut; | Rust &mut [u8] | none | impl | impl | impl | exists | - | pre | 0.1.0 | borrow | - |
| ms_none | ms_slice ms_none(void); | Rust Option::None | none | impl | impl | impl | exists | - | pre | 0.1.0 | borrow | - |
| ms_is_none | int ms_is_none(const ms_slice *s); | Rust Option::is_none | none | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| ms_span | ms_slice ms_span(const unsigned char *p, long n); | Rust slice::from_raw_parts | none | impl | impl | impl | exists | - | pre | 0.1.0 | borrow | - |
| ms_of | ms_slice ms_of(const mg_buf *b); | Rust Vec::as_slice | none | impl | impl | impl | exists | - | pre | 0.1.0 | borrow | - |
| ms_len | long ms_len(const ms_slice *s); | Rust slice::len | none | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| ms_at | int ms_at(const ms_slice *s, long off, long n, ms_slice *out); | Rust slice::get(range); Node Buffer.subarray | -ERANGE | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| ms_eq | int ms_eq(const ms_slice *a, const ms_slice *b); | Rust slice == | none | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| ms_find | long ms_find(const ms_slice *s, const ms_slice *needle); | Node Buffer.indexOf | miss:-1 | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| ms_rfind | long ms_rfind(const ms_slice *s, const ms_slice *needle); | Node Buffer.lastIndexOf | miss:-1 | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| ms_contains | int ms_contains(const ms_slice *s, const ms_slice *needle); | Node Buffer.includes | none | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| ms_starts_with | int ms_starts_with(const ms_slice *s, const ms_slice *p); | Rust slice::starts_with | none | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| ms_ends_with | int ms_ends_with(const ms_slice *s, const ms_slice *p); | Rust slice::ends_with | none | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| ms_mut_of | ms_slice_mut ms_mut_of(mg_buf *b); | Rust Vec::as_mut_slice | none | impl | impl | impl | exists | - | pre | 0.1.0 | borrow | - |
| ms_mut_len | long ms_mut_len(const ms_slice_mut *s); | Rust slice::len | none | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| ms_mut_at | int ms_mut_at(const ms_slice_mut *s, long off, long n, ms_slice_mut *out); | Rust slice::get_mut(range) | -ERANGE | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| ms_mut_fill | int ms_mut_fill(ms_slice_mut *s, int byte); | Rust slice::fill | -ENOMEM | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| ms_mut_copy | int ms_mut_copy(ms_slice_mut *dst, const ms_slice *src); | Rust copy_from_slice | -ERANGE | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
