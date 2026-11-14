<!-- 本文件由 tools/surface-doc-gen.cx 自 build/std-surface.tsv 生成,勿手改 -->

# API: cx.stream

本文件由 tools/surface-doc-gen.cx 自 build/std-surface.tsv 生成,勿手改。列义见该表文件头。

| name | signature | analogue | errors | lnx | osx | win | status | note | stab | since | own | test |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| xs_item | typedef struct { mg_buf buf; int shape; } xs_item; | Rust Vec<u8> 带形状标签 | none | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| xs_item_init | void xs_item_init(xs_item *it); | Rust Default::default | none | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| xs_item_drop | void xs_item_drop(xs_item *it); | Rust Drop | none | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| xs_item_set | int xs_item_set(xs_item *it, int shape, unsigned char *data, long len, long cap); | Rust Vec::from_raw_parts | -EINVAL | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| xs_item_bytes | ms_slice xs_item_bytes(const xs_item *it); | Rust Vec::as_slice | none | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| xs_open | int xs_open(int *h, long bound); | Node stream.pipeline; Rust Iterator 链 | -ENOMEM;-EMFILE | impl | impl | impl | exists | - | pre | 0.1.0 | open:pipe close:xs_close | - |
| xs_add | int xs_add(int h, int id, int in_shape, int out_shape, int (*fn)(void *, void *, void *, char *, int), void *ctx); | Node stream.Transform | -EBADF;-EMFILE;-EINVAL | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| xs_tap | int xs_tap(int h, void (*fn)(void *, int, int, int, const unsigned char *, long), void *ctx); | Rust Iterator::inspect | -EBADF | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| xs_run | int xs_run(int h, xs_item *in, xs_item *out, char *why, int cap); | Node stream.pipeline 回调 | -errno;ret:why | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| xs_receipt_count | int xs_receipt_count(int h); | 无 | -EBADF | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| xs_receipt | int xs_receipt(int h, int i, int *id, int *rc, long *in_n, long *out_n); | 无 | -EBADF | impl | impl | impl:计数是 32 位 long | exists | 新代码用 xs_receipt64 | legacy | 0.1.0 | none | - |
| xs_receipt64 | int xs_receipt64(int h, int i, int *id, int *rc, int64_t *in_n, int64_t *out_n); | 无 | -EBADF;-EINVAL | impl | impl | impl | exists | 64 位计数 | pre | 0.1.0 | none | cx-streamapi |
| xs_receipt_id | int xs_receipt_id(int h, char *hex, long cap); | 无 | -EBADF | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| xs_close | int xs_close(int h); | Rust drop | -EBADF | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| xs_reader_fd | int xs_reader_fd(int *r, int fd, long bufcap); | Rust BufReader::with_capacity; Node fs.createReadStream | -ENOMEM;-EMFILE;-EINVAL | impl | impl | impl | exists | 句柄式;不拥有 fd,关 reader 不关 fd;管道、文件、套接字同一入口;句柄用尽 | pre | 0.1.0 | none | cx-streamapi |
| xs_reader_mem | int xs_reader_mem(int *r, const unsigned char *p, int64_t n); | Rust io::Cursor; Node Readable.from(buffer) | -EMFILE;-EINVAL | impl | impl | impl | exists | 借用,不拷贝 | pre | 0.1.0 | none | cx-streamapi |
| xs_read | int64_t xs_read(int r, void *buf, int64_t n); | Rust Read::read; QuickJS FILE.read | -errno;-EBADF;ret:eof-0 | impl | impl | impl | exists | - | pre | 0.1.0 | none | cx-streamapi |
| xs_read_full | int xs_read_full(int r, void *buf, int64_t n); | Rust Read::read_exact | -errno;-EPIPE | impl | impl | impl | exists | 提前结束记 -EPIPE | pre | 0.1.0 | none | cx-streamapi |
| xs_read_line | int xs_read_line(int r, mg_buf *line, long max); | Rust BufRead::read_line; QuickJS FILE.getline; Node readline | -errno;-EMSGSIZE;ret:1-item-0-end | impl | impl | impl | exists | 去掉行尾 LF 与 CRLF;末行无换行也给出;不静默截断;行超过 max 记 -EMSGSIZE | pre | 0.1.0 | none | cx-streamapi |
| xs_reader_count | int64_t xs_reader_count(int r); | Rust Seek::stream_position | -EBADF | impl | impl | impl | exists | 已交给调用者的字节数,64 位 | pre | 0.1.0 | none | cx-streamapi |
| xs_reader_close | int xs_reader_close(int r); | Rust drop(BufReader) | -EBADF | impl | impl | impl | exists | - | pre | 0.1.0 | none | cx-streamapi |
| xs_writer_fd | int xs_writer_fd(int *w, int fd, long bufcap); | Rust BufWriter::with_capacity; Node fs.createWriteStream | -ENOMEM;-EMFILE;-EINVAL | impl | impl | impl | exists | 不拥有 fd | pre | 0.1.0 | none | cx-streamapi |
| xs_writer_buf | int xs_writer_buf(int *w, mg_buf *dst, int64_t limit); | Rust Vec<u8> as Write; Node Writable 收集 | -EMFILE;-EINVAL | impl | impl | impl | exists | 写入超过 limit 记 -EMSGSIZE;limit 小于 0 表不限 | pre | 0.1.0 | none | cx-streamapi |
| xs_write | int xs_write(int w, const void *p, int64_t n); | Rust Write::write_all; QuickJS FILE.write | -errno;-EBADF;-EMSGSIZE | impl | impl | impl | exists | 全写或报错,没有部分写入的返回 | pre | 0.1.0 | none | cx-streamapi |
| xs_flush | int xs_flush(int w); | Rust Write::flush; QuickJS FILE.flush | -errno | impl | impl | impl | exists | - | pre | 0.1.0 | none | cx-streamapi |
| xs_writer_count | int64_t xs_writer_count(int w); | Rust Seek::stream_position | -EBADF | impl | impl | impl | exists | - | pre | 0.1.0 | none | cx-streamapi |
| xs_writer_close | int xs_writer_close(int w); | Rust drop(BufWriter) 但报告 flush 错误 | -errno;-EBADF | impl | impl | impl | exists | 先 flush;flush 失败也释放句柄并回报错误 | pre | 0.1.0 | none | cx-streamapi |
| xs_copy | int xs_copy(int r, int w, int64_t limit, int64_t *copied); | Rust io::copy; Node stream.pipeline | -errno;-EMSGSIZE | impl | impl | impl | exists | limit 小于 0 表不限;超过 limit 记 -EMSGSIZE | pre | 0.1.0 | none | cx-streamapi |
| xs_pipe_pair | int xs_pipe_pair(int *r, int *w, long bufcap); | Node stream.PassThrough; Rust io::pipe | -errno;-EMFILE | impl | impl | impl:CreatePipe | exists | 经 sy_pipe 的一对读写端;关句柄时关 fd | pre | 0.1.0 | none | cx-streamapi |
| xs_reader_deadline | int xs_reader_deadline(int r, sy_deadline_t d); | Rust Read 带超时 | -EBADF | refuse:ENOSYS 本版未实现 | refuse:ENOSYS 本版未实现 | refuse:ENOSYS 本版未实现 | exists | 之后的读到期返回 -ETIMEDOUT | pre | 0.1.0 | none | cx-streamapi |
| xs_writer_deadline | int xs_writer_deadline(int w, sy_deadline_t d); | Rust Write 带超时 | -EBADF | refuse:ENOSYS 本版未实现 | refuse:ENOSYS 本版未实现 | refuse:ENOSYS 本版未实现 | exists | 之后的写到期返回 -ETIMEDOUT | pre | 0.1.0 | none | cx-streamapi |
