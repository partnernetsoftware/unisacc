# 门禁重叠清单（待政委/董秘确认）

cc 2026-10-09 由 `tests/gate.sh --list --com | unisacc.com -run tests/stagemap.c -- --overlaps` 生成，共 138 项。规则：只标注、不合并、不删测试、不改验收措辞或归属；每组确认后才在 0.0.38 K2c 合并。

| 主阶段 → 也属于 | 数量 | 套件 |
|---|---|---|
| E3 → build | 48 | `exec-e3self`, `rowcov-parse2-build`, `rowcov-parse2-union`, `seedparse2-1`, `seedparse2-2`, `seed-construct-parse2`, `seed-construct-parse2-2`, `seed-construct-parse2-3`, `seed-construct-parse2-4`, `seed-construct-parse2-5`, `seed-construct-parse2-6`, `seed-construct-parse2-7`, `seed-construct-parse2-8`, `seed-construct-parse2-10`, `seed-construct-parse2-11`, `seed-construct-parse2-12`, `seed-construct-parse2-13`, `seed-construct-parse2-14`, `seed-construct-parse2-15`, `seed-construct-parse2-16`, `seed-construct-parse2-17`, `seed-construct-parse2-18`, `seed-construct-parse2-19`, `seed-construct-parse2-20`, `seed-construct-parse2-21`, `seed-construct-parse2-22`, `seed-construct-parse2-23`, `seed-construct-parse2-24`, `seed-construct-parse2-25`, `seed-construct-parse2-26`, `seed-construct-parse2-27`, `seed-construct-parse2-28`, `seed-construct-parse2-29`, `seed-construct-parse2-30`, `seed-construct-parse2-31`, `seed-construct-parse2-32`, `seed-construct-parse2-33`, `seed-construct-parse2-34`, `seed-construct-parse2-35`, `seed-construct-parse2-36`, `seed-construct-parse2-37`, `seed-construct-parse2-38`, `seed-construct-parse2-39`, `seed-construct-parse2-40`, `seed-construct-parse2-41`, `seed-construct-parse2-42`, `seed-construct-parse2-43`, `seed-construct-parse2-44` |
| chain → E3 | 27 | `exec-multi-cc-m-forward`, `exec-multi-cc-m-reverse`, `exec-multi-cc-n-forward`, `exec-multi-cc-n-reverse`, `exec-multi-cc-pool-forward`, `exec-multi-cc-pool-reverse`, `exec-multi-cc-isolation`, `exec-multi-cc-static`, `exec-multi-ua-m-forward`, `exec-multi-ua-m-reverse`, `exec-multi-ua-n-forward`, `exec-multi-ua-n-reverse`, `exec-multi-ua-pool-forward`, `exec-multi-ua-pool-reverse`, `exec-multi-ua-isolation`, `exec-multi-ua-static`, `exec-multi-asm-m-forward`, `exec-multi-asm-m-reverse`, `exec-multi-asm-n-forward`, `exec-multi-asm-n-reverse`, `exec-multi-asm-pool-forward`, `exec-multi-asm-pool-reverse`, `exec-multi-asm-isolation`, `exec-multi-asm-static`, `exec-multi-cc-frame`, `multi`, `com-multi` |
| E6 → chain | 14 | `exec-memory-cc-1`, `exec-memory-cc-2`, `exec-memory-cc-3`, `exec-memory-ua-1`, `exec-memory-ua-2`, `exec-memory-ua-3`, `exec-memx86-cc-1`, `exec-memx86-cc-2`, `exec-memx86-cc-3`, `exec-memx86-ua-1`, `exec-memx86-ua-2`, `exec-memx86-ua-3`, `exec-memwinarm`, `exec-memwinx86` |
| chain → build | 11 | `bigclosure-lnx-x86_64`, `bigclosure-lnx-arm64`, `bigclosure-osx-x86_64`, `bigclosure-osx-arm64`, `bigclosure-win-x86_64`, `bigclosure-win-arm64`, `closure-ex`, `closure-c1`, `closure-c2`, `closure-c3`, `closure-c4` |
| lower → E6 | 8 | `exec-bindprep-first-arm64`, `exec-bindprep-second-arm64`, `exec-bindprep-arm64`, `exec-bindverify-arm64`, `exec-bindprep-first-x86_64`, `exec-bindprep-second-x86_64`, `exec-bindprep-x86_64`, `exec-bindverify-x86_64` |
| lower → lib | 7 | `ffi-bridge`, `ffi-product`, `forward`, `forward-multi`, `ccinterop`, `com-forward`, `com-forward-multi` |
| E3 → meta | 7 | `tapebin-roundtrip-structure-1`, `tapebin-roundtrip-structure-2`, `tapebin-roundtrip-codec`, `tapebin-shape`, `com-tapebin-1`, `com-tapebin-2`, `com-tapebin-3` |
| E5 → chain | 6 | `exec-memory-asm-1`, `exec-memory-asm-2`, `exec-memory-asm-3`, `exec-memx86-asm-1`, `exec-memx86-asm-2`, `exec-memx86-asm-3` |
| lower → build | 2 | `rowcov-lower-build`, `rowcov-lower-union` |
| E6 → E3 | 2 | `linkunits`, `com-linkunits` |
| E5 → build | 2 | `rowcov-enc-build`, `rowcov-enc-union` |
| E5 → E6 | 2 | `asmtext`, `combo` |
| E4 → build | 1 | `exec-e4self` |
| E1 → build | 1 | `strconvert-seed` |

## 建议（待确认，不执行）

- E3 → build（48）：多为 seed/自举的 parse2 构造，主体是 E3 表，build 只是消费者；建议维持 E3。
- chain → E3（27）：多单元 multi 套件，按整链判定；建议维持 chain，E3 改动时额外入选。
- E6 → chain（14）：exec-memory/memx86/memwin 内存运行；建议维持 E6。
- 其余小组逐项确认。
