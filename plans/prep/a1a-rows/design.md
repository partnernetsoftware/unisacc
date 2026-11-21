# A1a 设计决定稿(简报乙,2026-11-21,供 0.0.89 开工用;只含设计与参考实现,未动 exec/ src/ cx/)

## 实跑证据
- 命令:`sh ./unisacc.com plans/prep/a1a-rows/ref-check.cx`,末行 `REF_BITS pass=10 fail=0`(b01 到 b10 共 10 例,输出 sha256 与 expect.tsv 的 gcc_out_sha256 列一致,返回码与 note 列 gcc_run_rc 一致)。
- 范围限定:参考函数在已装产品下默认 -run 执行,证明的是展开式算法只用 and or xor shl shr mul sub add 与比较即可得 gcc 同值;不证明六目标 lowering。b07 的 sizeof 三值与 b08 的常量折叠值(popcount(0xF0)=4、ctz(16)=4)在检查里是字面核对,类型与折叠要在实现期用样例本身核。b10 的 clz(0) 走守卫返回 32。
- 参考式写在 ref-check.cx 内(简报写的 ref-bits.c 并入此文件,避免两份漂移)。

## D1 位运算:用现有运算通用展开,不新增 IR 运算
展开式要点(u32 为例,u64 同构换常量与宽度):
- popcount:SWAR 四步,`x-((x>>1)&0x55..)`,`(x&0x33..)+((x>>2)&0x33..)`,`(x+(x>>4))&0x0f..`,`(x*0x0101..)>>24`(64 位 >>56)。
- clz:先涂抹 `x|=x>>1,2,4,8,16[,32]`,再 `W - popcount(x)`;clz(0)=W(gcc 未定义,守卫用法 b10 不受影响)。
- ctz:`popcount((x&(0-x))-1)`;ctz(0)=W。ffs:`x?ctz+1:0`。
- parity:`x^=x>>32,16,8,4,2,1` 后 `&1`(或 popcount&1)。
- bswap16/32/64:移位加掩码加或;64 位由两个 32 位拼。
- 用到的 IR 风味均在词表内(exec/facts/kernel-emit.tsv:57 的 IRFLAV:add sub mul and or xor shl shr lshr ult 等)。注意无符号右移须用 lshr,64 位常量走 imm。
备选:(A) 新 IR 运算 + 两 ISA 编码,原生 popcnt lzcnt bsr,要改 lower-code lower-abi enc-x86 enc-arm kernel-emit tape-shape opt-gen 等约 8 个,破 20 封顶;(B) 外部库调用,违反无 libc 方向。结论 D1 取通用展开,原生指令留作以后优化。
常量折叠(b08 数组长度、静态初始化):constexpr-operators 或 constexpr-manifest 的折叠器直接对字面量算同一公式,不走 IR。
未验证:栈机上展开要复用操作数,有无 dup 或局部槽可用未核(风味表有 frame load store lea,预期可用局部槽暂存);e3 步数增量未测。

## D2 trap:不新开运算
trap 与 unreachable 都不新增 IR 运算。trap 在 lnx 上用现有运算构造必然终止的序列,候选:(1) 对地址 0 的 store(SIGSEGV,rc 139),(2) 调现有 exit 风味不合要求(gcc 是被信号杀,u02 按 rc>=128 判)。取 (1) 作首选,代价是信号为 SEGV 而非 SIGILL,按比对规则 rc>=128 通过,须在发布说明写明。备选:ud2 经 asmtext(只在 cx/compiler/asmtext.cx 出现,不在 enc 表,不选)。unreachable 作为无后继控制流终点,与缺 return 诊断的交互未核。osx win 沿用具名拒绝。

## D3 U4 范围
只做 lnx ELF:函数定义与外部声明、外部变量的改名,加 `__asm` 拼写登记(a04)。osx win 保持具名拒绝并写入发布说明。a05 a06 a07 边界例本稿未裁(P5 未做)。

## 砍线顺序
先砍 cxm_iscxpath 随行(已移出 0.0.89);再砍 U4 变量改名(a03);U0 U1 U2 U3 不砍。

## 启动驻留名
现 exec/parse2/startup-names.tsv 数据行 18(三个文件头注释行不计;builtins 组 13)。U1 U2 U3 新增 20 名:clz clzl clzll ctz ctzl ctzll popcount popcountl popcountll parity parityl parityll ffs ffsl ffsll(15),bswap16 bswap32 bswap64(3),unreachable trap(2),均带 `__builtin_` 前缀。不带前缀别名不登记。降低数量的备选:前缀识别放词法侧(U0 的设计点)后由单一规则覆盖,不逐名驻留;未验证。
28 示例步数现值:UNMEASURED(0.0.88 首链运行中,全机禁重负载,Q4 待首链结束后测)。

## 行草稿
见 rows-draft.tsv(startup-names 20 行定稿格式;k2-call 与 callcontrol 行因展开文本要在 IR 文本格式下写,仅列键与待填项,不冒充已验证)。
