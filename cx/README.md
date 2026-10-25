Bounded runtime-owned i64 buffer prototype, not compile-time Rust ownership.
Import owned with #pragma unisa use owned. No source #include or pointer out-parameter.
All public operations return owned_result {status,value}; errors set value=0.
new accepts lengths 0..8, zero initializes, and returns a handle; four slots maximum.
length/get return a value; set/drop return value 0. get/set validate handle before index.
move returns a new generation and invalidates all old handle copies; drop invalidates them.
Each slot admits generations 1..255. No wrap: move refuses at 255, retaining the valid owner;
drop retires that slot permanently. new distinguishes full capacity from free retired slots.
Failure changes no live buffer or valid handle. Inputs are checked before any array access.
All storage and implementation helper functions are module-static.
Single-threaded only; no reset, resizing, general heap allocation, or borrow checker.
Handles are ordinary copyable integers: copies alias a live object and guessed live handles can
access it. This is runtime stale-handle/bounds isolation, not linearity, secrecy, or authority.
Arbitrary C, malicious ABI declarations, and data races are outside this module contract.
Run `sh ./unisacc.com cx/check_owned.cx` under a caller-owned <=30 second watchdog.
This module is a reusable bounded runtime facility. It is not yet used by compiler
internals or the construction coordinator. `make owned` is its development entry;
the complete new suite still reports NOT_READY independently of this local batch.
