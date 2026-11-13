<!-- 本文件由 tools/surface-doc-gen.cx 自 build/std-surface.tsv 生成,勿手改 -->

# API: cx.path

本文件由 tools/surface-doc-gen.cx 自 build/std-surface.tsv 生成,勿手改。列义见该表文件头。

| name | signature | analogue | errors | lnx | osx | win | status | note | stab | since | own | test |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| pt_is_sep | int pt_is_sep(int c); | Rust path::is_separator | none | impl:只认 / | impl:只认 / | impl:认 / 与 \ | exists | 宿主风格;跨风格用 *_as | pre | 0.1.0 | none | - |
| pt_sep | int pt_sep(void); | Node path.sep; Rust path::MAIN_SEPARATOR | none | impl:/ | impl:/ | impl:\ | exists | - | pre | 0.1.0 | none | - |
| pt_is_abs | int pt_is_abs(const char *p, long n); | Node path.isAbsolute; Rust Path::is_absolute | none | impl | impl | impl:盘符加分隔符、UNC、\\?\ 记绝对;单个 \ 开头不算 | exists | - | pre | 0.1.0 | none | - |
| pt_is_abs_as | int pt_is_abs_as(int style, const char *p, long n); | Node path.posix/win32.isAbsolute | -EINVAL | impl | impl | impl | exists | style 取 PT_POSIX/PT_WIN/PT_HOST;纯文本,任何宿主结果相同;坏 style | pre | 0.1.0 | none | - |
| pt_root_len | long pt_root_len(int style, const char *p, long n); | Node path.parse().root; Rust Component::Prefix + RootDir | none | impl | impl | impl | exists | 根前缀长度:/ 为 1,C:\ 为 3,\\srv\share\ 整段,相对路径 0 | pre | 0.1.0 | none | - |
| pt_join | long pt_join(char *dst, long cap, const char *a, const char *b); | Node path.join; Rust Path::join; QuickJS 无 | -ERANGE;ret:needlen | impl | impl | impl:两种分隔符都收,输出用 \ | exists | b 是绝对路径时结果就是 b(Rust 语义);不规整 | pre | 0.1.0 | none | - |
| pt_join_n | long pt_join_n(char *dst, long cap, const char *const *parts, int n); | Node path.join(...parts) | -ERANGE;-EINVAL | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| pt_join_as | long pt_join_as(int style, char *dst, long cap, const char *a, const char *b); | Node path.posix/win32.join | -ERANGE;-EINVAL | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| pt_dirname | int pt_dirname(const char *p, long n, long *off, long *len); | Node path.dirname; Rust Path::parent | none;-EINVAL | impl | impl | impl | exists | 不分配,指回原串;尾部分隔符先去掉;无父目录时 len 为 0 | pre | 0.1.0 | none | - |
| pt_basename | int pt_basename(const char *p, long n, long *off, long *len); | Node path.basename; Rust Path::file_name | none;-EINVAL | impl | impl | impl | exists | 根路径时 len 为 0 | pre | 0.1.0 | none | - |
| pt_ext | int pt_ext(const char *p, long n, long *off, long *len); | Node path.extname; Rust Path::extension | none;-EINVAL | impl | impl | impl | exists | 含点;.bashrc 这类点开头名无扩展名;无扩展名 len 为 0 | pre | 0.1.0 | none | - |
| pt_stem | int pt_stem(const char *p, long n, long *off, long *len); | Rust Path::file_stem; Node path.parse().name | none | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| pt_next | int pt_next(const char *p, long n, long *pos, long *off, long *len); | Rust Path::components | ret:1-item-0-end | impl | impl | impl | exists | 跳过空分量;根前缀作为首个分量给出 | pre | 0.1.0 | none | - |
| pt_normalize | long pt_normalize(char *dst, long cap, const char *p, long n); | Node path.normalize; Go filepath.Clean | -ERANGE;ret:needlen | impl | impl | impl:输出统一为 \,保留盘符大小写 | exists | 折叠重复分隔符、. 与 ..;纯文本不查盘;空串得 .;绝对路径的 .. 不越根 | pre | 0.1.0 | none | - |
| pt_normalize_as | long pt_normalize_as(int style, char *dst, long cap, const char *p, long n); | Node path.posix/win32.normalize | -ERANGE;-EINVAL | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| pt_relative | long pt_relative(char *dst, long cap, const char *from, const char *to); | Node path.relative; Rust(pathdiff) diff_paths | -ERANGE;-EINVAL;-EXDEV | impl | impl | impl:盘符比较不分大小写 | exists | 两边先规整;相同路径得 .;一绝对一相对;盘符不同 | pre | 0.1.0 | none | - |
| pt_has_prefix | int pt_has_prefix(const char *p, long n, const char *dir, long dn); | Rust Path::starts_with | 0/1 | impl | impl | impl | exists | 按分量比较,/ab 不是 /a 的下级;两边先规整;用于越界防护 | pre | 0.1.0 | none | - |
| pt_to_slash | long pt_to_slash(char *dst, long cap, const char *p, long n); | Go filepath.ToSlash | -ERANGE | impl:原样拷贝 | impl:原样拷贝 | impl:\ 换成 / | exists | - | pre | 0.1.0 | none | - |
| pt_to_native | long pt_to_native(char *dst, long cap, const char *p, long n); | Go filepath.FromSlash; Node path.toNamespacedPath 近似 | -ERANGE | impl:原样拷贝 | impl:原样拷贝 | impl:/ 换成 \ | exists | - | pre | 0.1.0 | none | - |
| pt_dirname_as | int pt_dirname_as(int style, const char *p, long n, long *off, long *len); | Node path.posix/win32.dirname | -EINVAL | impl | impl | impl | exists | len 0 表示当前目录 . | pre | 0.1.0 | none | - |
| pt_basename_as | int pt_basename_as(int style, const char *p, long n, long *off, long *len); | Node path.posix/win32.basename | -EINVAL | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| pt_ext_as | int pt_ext_as(int style, const char *p, long n, long *off, long *len); | Node path.posix/win32.extname | -EINVAL | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| pt_stem_as | int pt_stem_as(int style, const char *p, long n, long *off, long *len); | Rust Path::file_stem | -EINVAL | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| pt_next_as | int pt_next_as(int style, const char *p, long n, long *pos, long *off, long *len); | Rust Path::components | -EINVAL | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| pt_relative_as | long pt_relative_as(int style, char *dst, long cap, const char *from, const char *to); | Node path.posix/win32.relative | -EINVAL;-ERANGE;-ENAMETOOLONG | impl | impl | impl | exists | 绝对与相对混用或 from 含无法回退的 ..;超 1024 | pre | 0.1.0 | none | - |
| pt_has_prefix_as | int pt_has_prefix_as(int style, const char *p, long n, const char *dir, long dn); | Rust Path::starts_with | -EINVAL;-ENAMETOOLONG | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
