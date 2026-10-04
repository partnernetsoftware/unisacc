# N2：POSIX 兼容主线的缺口清单（0.0.25，2026-10-04）

验收对象（政委 2026-10-04 定）：lua、sqlite、minicon、agenterm。下面是参考编译器（/tmp/ua_ref，与当前 main 同源）实际编译这些 C 源码时报出的第一处缺口；产品路线的结果在 0.0.26 补测。分类依据 prd §5.1：能转发给系统库的优先转发，Windows 等不能转发的目标再自研。

| 程序 | 文件 | 第一处缺口 | 类别 | 处理建议 |
|---|---|---|---|---|
| lua | corpus/lua | 无（realprog 基线已通过） | — | 保持 |
| sqlite | corpus/sqlite/sqlite3.c（9.2 MB 合并版） | `source too large`：src/main.c 的 MAXSRC 上限 | 编译器容量 | 调大上限或改为动态增长；是 v0.1.2“容量”的一部分，可单独提前 |
| sqlite | corpus/sqlite/shell.c | 没有 `<pwd.h>` | 头文件 | 加 pwd.h：getpwuid/getpwnam 在 POSIX 目标上只写原型（转发），Windows 返回 0 |
| minicon | loader/loader.c | `extern char **environ` 没有定义 | libc 全局变量 | 转发只接函数，不接数据符号：要么在 unistd.h 里由启动代码提供 environ，要么扩展转发去取数据符号 |
| agenterm | research/agenterm-com-loader/loader.c | 同上，environ | libc 全局变量 | 同上 |
| agenterm | examples/c/*.c、crates/agenterm-platform/native/process_inspection.c | 没有 `<windows.h>`、`<dlfcn.h>`、`<libproc.h>`、`<sys/proc_info.h>` | 头文件 | dlfcn.h（dlopen/dlsym）在 POSIX 目标上转发，同时也是 TLS“按库名 dlopen”的前提；libproc/proc_info 是 macOS 专用，只写原型加结构体；windows.h 范围太大，只按 agenterm 实际用到的函数补 |

建议的顺序（0.0.26 起，每补一类配一个与系统 cc 对拍的探针）：
1. `<dlfcn.h>` 转发：TLS 和 agenterm 都要用。
2. environ：minicon 和 agenterm 的 loader 都卡在这里。
3. `<pwd.h>` 转发与 MAXSRC 容量：这两项做完，sqlite 的 shell 和合并版都能进入编译。
4. libproc/proc_info（macOS）和 windows.h 的子集：按 agenterm 的实际调用补。
