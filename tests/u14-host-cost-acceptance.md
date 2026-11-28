# U14-3 验收合同

(1) 测量对象为 shared_rt、m53_wrap、m25_wrap、driveargs_wrap、bind_wrap。五行一一对应，来源与固定提交的 root_target 匹配。每行追加 rev、target、compiler_sha256、flags 四个 TAB 字段。验收方从 Git HEAD、固定目标、独立哈希和参数表取得期望值。

(2) 验收入口接受 UNISACC_U14_COMPILER，指向实际编译器或测试夹具，工具必须使用并记录该编译器的实际字节摘要。缺件夹具仅拦截 bind 的编译并返回 0，不写对象。验收要求工具返回 1，并输出 bind_wrap 的 text_bytes=-1 和汇总 FAIL。其他对象照常编译。注册批次必须传播缺件失败。

(3) 变异逐项验证旧提交、错目标、错摘要、错参数、错路由、重复单位、吞掉缺件。遍历计量工具、现有 check、批次登记及收据消费者。

(4) 第二批用 tests/check-u14-host-cost-elf.cx 独立组装 ELF64，调用计量工具的 --measure-object FILE --target lnx/x86_64。正常输出一行 U14_ELF_TEXT，随后是 TAB 分隔的 text_bytes 与 obj_bytes。拒绝返回 1，标准错误含 REFUSE invalid-elf-object。六个用例验证 17 字节、追加 63 字节后仍为 17、节长改为 18、目标机型错误、节表偏移回绕和节数据越界。检查入口应复用正式计量的同一个解析器。
