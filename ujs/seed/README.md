# seed/ — 第 0 代（造出权重与 iterate）

- [`construct/`](construct/) — Python 构造臂（gold/catalog/oracle/front/emit）；`import ujs.construct` 仍可用（顶层 symlink）
- [`stage0/`](stage0/) — C 子集编译器源与 zig 构建脚本 → `../iterate/compiler.wasm`
- [`vm/`](vm/) — path-A `ujs_vm.c` 等

判别：这里是**程序**；写出的表网进 [`../weights/`](../weights/)。
