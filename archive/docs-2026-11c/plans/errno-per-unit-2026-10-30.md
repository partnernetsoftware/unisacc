# errno 每个编译单元各一份（2026-10-30）

## 事实

- **最小复现**：out/errnotest/，csih 报告，cc 用已装产品 79fb84e6 复测过。
  - seterr.cx 的 `set_err(17)` 执行 `errno = 17`。
  - main.cx 先置 `errno = 0`，再调用 `set_err(17)`，之后读到的仍是 0。
- **原因**：include/errno.h 第 9 行（cx-flat.h 第 76 行）是 `static int errno = 0;`。
  - 前导会进入每个单元，所以每个单元各有一份 errno。
  - 前导的 libc 包装也是 static 的，所以 errno 和它的设置者总在同一个单元内，单元内部的语义不受影响。

## 合同（本版）

- **跨模块传错误码一律走返回值，不得走 errno。**
  - sy_* 一律返回 -errno，这一点已经是现状。
  - 上层模块的导出函数也一样：要么返回 -errno，要么返回自己的错误码。
- **允许**：同一个单元里调用前导 libc 后紧接着读 errno。
- **不允许**：读另一个模块的函数留下的 errno，读到的值是未定义的。
- **csih 的 fio.cx**：单独导出 fio_errno()，是在这个合同下绕开问题的做法。新代码优先改成返回 -errno。

## 修复方向（编译器车道，排在 win -o 和单元缓存之后）

- 把 errno 改成整个映像只有一份：前导里写 `extern int errno;`，由链接器只出一个定义（common 或 weak 符号，或由驱动程序注入一个单元来定义）。前提是 tape 链接器支持这种只留一份的符号。
- 修好之前不改合同；修好之后合同仍然保留，返回值本来就是更好的接口。
- 验收：
  - out/errnotest 打印 17；
  - 同一单元内的 errno 用例不回退（check-cx-sys 108/0）；
  - runcache 12/0。
