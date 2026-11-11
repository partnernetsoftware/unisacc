# Rosetta / 加载路径独立只读审查

结论：**正常绑定源码的干净环境 --version 路径不会调用kn_load，不能把其挂起归因于该函数的RW→RX。** 这是源码早退链的结论；采样本身不能证明应用从未进入。原路径挂起、同Developer ID完整字节的私有新inode副本成功，使“路径/文件对象相关的系统加载或翻译状态”成为优先诊断方向，但还不足断言AOT缓存损坏。Apple公开资料不承诺Rosetta以inode或路径作为缓存键，也不保证cp可清除某种缓存。

本轮实际动作只有读取身份报告/采样/源码及Apple官方网页查询，未启动目标程序、未vmmap、未签名、未删xattr/系统缓存、未改安全设置或主树/git。下面下一实验均未执行。未把其它代理报告的退出码冒充本人持有的进程终态。

## 已有证据复核

实际读取 `identity-review.md`（SHA e6762d553dc8a2de697a25823fdf4fdf2d37062dde98a67e5f43324927e7cfbf）及完整 `original-dev-id-fresh.sample`（SHA f6497f653138d968d9a672862c1c0a8afdf5d508cd663f486b535021f9e296c7）。

- 原run2/osx-x86_64：2462816B，完整SHA `3679a611cbc1eb1204e1ff298c5ba2ba2306565792c5a74f8583a3833f2b4ec6`，Developer ID/runtime、Team L2N7M5M544，allow-unsigned-executable-memory entitlement。原inode94308528；cp -p私有副本inode94312735，完整SHA/755/无xattr一致，双方strict codesign verify0。文件SHA与CDHash不同：原codesign报告CDHash `77ccc8b39d0ea5fba02df20b2230529d8f65e2c6`，SHA256 CodeDirectory fullhash `77ccc8b39d0ea5fba02df20b2230529d8f65e2c645cc9468ac7a7e647f06c8ae`。
- 同cleanenv、同带footer P4 `fffa2311189d97d7dd0868a59a47ff788a6af17259f2664454f9a8351c2ac842`、同arch -x86_64：原路径两次8秒watchdog142，私有Developer ID同字节副本0且输出0.0.48.1。由身份报告保留原会话20898/2840真实终态；本人未再重现。名为dev64的文件实际是原ad-hoc，不能用其名字判断Developer ID。
- 有效采样为PID82661启动后约0.5秒采1秒：769次全部同一Rosetta Runtime Routines地址、200K footprint、无应用栈、Binary images不可用。sample输出Code Type ARM64只描述当前可见Rosetta原生runtime，不能据此说x64程序实际换成ARM切片。第一次旧PID采样255失败不作栈证据。

采样缺应用栈/很小footprint与早期Rosetta路径相容，但也可能受符号/展开能力限制；不能单独排除已执行过应用、回调、代码映射后卡在runtime等情况。142表示watchdog终止，不是macOS返回“签名拒绝”错误码；stdout空也不证明停在main之前。同SHA副本成功反驳“该签名+entitlement在这台机器必然不能执行”的绝对命题，不能证明全部真实编译/运行路径正常。

## Apple官方资料支持什么

Apple说明JIT和AOT都会在dyld前由Rosetta接管；AOT由系统服务向进程提供翻译产物只读FD，补充签名绑定原x64的CodeDirectory hash，产物位于服务管理的Data Vault；原hash与产物签名不匹配按无效代码签名处理。[Apple Platform Security：Rosetta 2](https://support.apple.com/en-sa/guide/security/secebb113be1/web)。因此“Rosetta Runtime Routines出现在最早加载阶段”有机制依据；本例是否走AOT、是否等待该IPC、是否命中错误缓存仍未测到。

CDHash是CodeDirectory的hash，不是整文件SHA；架构和多个CodeDirectory会影响选择值，核身份应保留完整codesign输出，而不只比较一个命名字符串。[Apple TN3126：Inside Code Signing—Hashes](https://developer.apple.com/documentation/technotes/tn3126-inside-code-signing-hashes)、[Apple CodeDirectoryHash](https://developer.apple.com/documentation/lightweightcoderequirements/codedirectoryhash)。

本轮查到的Apple官方说明**没有公开缓存以inode/path/CDHash哪种组合索引的完整实现合同**。不能从它推导“同SHA必命中同AOT”或“新inode必绕AOT”。cp同时改变路径、文件对象及系统此前见过它的历史；即使权限/字节/xattr相同，仍不是只改变一个变量。也不能把静态codesign verify0等同动态加载/翻译/公证信任全部通过。系统cache的删除、重装Rosetta、重启或放宽entitlements都不应是本轮默认诊断步骤。

## --version的源码因果边界

已实读当前源：

1. `src/unisacc.cx:5` main调用dv_main，之后rescache_release。
2. `src/drive.cx:398–464`：干净env的ex_cap_gate只处理argv；cxc_default_run、dv_parse进入直接CLI扫描。
3. `src/driveargs.cx:28` dv_parse_args先cxc_step；--version经dv_word到`src/entrybind.cx:99` bx_cli_word=1，返回kind11。
4. `src/drive.cx:247–250` kind11直接ex_ri_say；`cx/runtime/runexec.cx:990–1008` 非strict ex_ri_finish只返回0，之后sy_printf。drive:464后i>=0立即return，不经过后续包打开、模型执行/编译路线。
5. `cx/sys/kernel.cx:54/117/126` kn_load是静态私有，只有kn_call在kn_code未初始化时调用；公开core_run才进kn_call。所查CLI早退链无core_run/kn调用。此链未见constructor/init_array自动触发kn_load。

因此在“签后文件确实对应已绑定源码、机器码执行正确、启动器未额外调用产品模型”的常规前提下，该次**--version挂起可排除显式kn_load执行作为原因**。这比从采样猜卡点强，但不是对CRT/系统dyld/Rosetta/静态映像映射或编译器错误的动态证明；本轮未做完整机器码可达证明。即使OS映射包含kernel模块的静态代码页，也不代表应用执行了kn_load。`-run`、hello、真实编译会调用core_run，不能借version早退替这些路径排除kn_load；mre-review中私有ad-hoc最小RW→RX成功也不能迁成Developer ID完整内核资格。

## 最小下一实验（设计，不重复原挂起）

先保留原签后路径/文件完全不动，以已成功的私有同字节副本建立只在out内的新对照。固定cleanenv/P4/argv/父进程和cwd，每项一次≤8秒，出现未知就停止该项，不自动重试原路径。

- 私有副本A原路径运行一次，然后仅在私有目录rename A→B（inode相同、SHA/权限/xattr/CDHash核相同），B运行一次；另造私有同目录新inode C并运行一次。记录实际文件身份及时间顺序，区分路径变化与文件对象/先前执行历史；仍有系统状态随时间变化混杂，不能仅三点就宣布缓存根因。不在原路径替换字节、不对原物建hardlink改变nlink。
- 若新增私有样本首次出现同型挂起，在**已有该次进程**上最多一次sample1秒和必要时vmmap -summary（各独立外bound3），核实际PID/PGID后终止/reap；无挂起则不为了抓图反复制造。vmmap里原映像/dyld/AOT映射存在与否可缩小阶段，但存在映射仍不证明main已执行。
- 可补只读、限定该PID和短时间窗的系统加载/Rosetta错误日志，寻找实际签名补充或服务失败信息；权限拒绝如实UNKNOWN，不提权读Data Vault，不用第三方缓存删除建议。没有AOT命中或IPC证据前，措辞只能是“加载/翻译状态相关假设”。

若私有同inode换路径仍成功、新inode也成功，而仅原对象历史挂起，优先形成最小sysdiagnose/Feedback所需证据；不先改kn_load或加宽签名权限。若未来真实-run卡在core_run/kn入口，则另开与本version现象独立的阶段标记诊断，不能沿用本报告替代动态边界证据。
