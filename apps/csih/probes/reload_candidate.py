{
  "passed": true,
  "first_hash": "b05d49a4e1b27041bb493068245a93b0ea801c1885843398bcfa5b3d30482a8e",
  "second_hash": "4d79ec9c914a8d64a9a071f1499ecf24d44eefe81f3d611e727af2a995e7c445",
  "checks": [
    "bad source refused; no ready receipt",
    "first real build and CLI verify, both gates green",
    "tamper refused: app/csih.c / source manifest/hash mismatch",
    "tamper refused: compiler.com / compiler hash mismatch",
    "tamper refused: candidate / artifact hash mismatch",
    "tamper refused: candidate / artifact permissions not0700",
    "second comment-only version: distinct identity, both real gates green"
  ],
  "driver_hash": "771590ca6a3252ab146ecf0eeaab84226ed239c21d0ad6895d1396d89c7fa594",
  "probe_hash": "461637b2d4b7c27fec1dd79da47d997d57c6beb244d7273c23562e37c6cb5576",
  "compiler_hash": "b7128a0f6c4539f232d9765b654df722d25cfbb4f042a4989820d9968035a310",
  "source_before": [
    {
      "path": "agent.c",
      "sha256": "e73d54bda58ccfc767aa3d80086755b0757ce65f266c7c86497c0503bb6d586a"
    },
    {
      "path": "agent_cli.c",
      "sha256": "73dcc70fee7e4ae342b6779d129dd3c8a95f47987da4cfa5e6eed200f912de88"
    },
    {
      "path": "agent_result.h",
      "sha256": "a44926692aeca38ae042c3cc4c7461a51ae0fd2a3acfa4412a9a992e5ceb3d7c"
    },
    {
      "path": "chat.c",
      "sha256": "20c77cb3d68a53db1865c13b597debd364501ec80307b622d81aa0c88f420c0c"
    },
    {
      "path": "chat_cli.c",
      "sha256": "8af314d6e7afff857d6dda32fcca8a04a6f687f8716a921c09dbf9e599bde3ea"
    },
    {
      "path": "check.c",
      "sha256": "96ae7d952c6f24c18191c8769832c433a206856c2ecad8ae4ea399f4014ee529"
    },
    {
      "path": "clock.c",
      "sha256": "860d620fa9709132fbed379b51560e59d951d2e71303f8f4b333cbc10ca537c0"
    },
    {
      "path": "clock_cli.c",
      "sha256": "b7db6a718f303c77950a6d3e80c6c6ff8ff9110aef7ba98fce30296b2190eab1"
    },
    {
      "path": "context_index.inc",
      "sha256": "bdf920c6fc5df88bf7ab27689784f86d02300070b1321bcfcec3dd2441de5b28"
    },
    {
      "path": "csih.c",
      "sha256": "c76e97da72dc910384f54e091a2f6b3e7c4a595641afd113ad88d26b944a4c38"
    },
    {
      "path": "csih_cols.h",
      "sha256": "b7d9337964e7805f698125723d11c44b7ecb01bc766f8245c54802bffcd57919"
    },
    {
      "path": "csih_home.h",
      "sha256": "4ce4308ad1042f8565e75655c7297965cef9e12467aede7fc66fdd52f037d351"
    },
    {
      "path": "csih_message.c",
      "sha256": "71caa134a5a5be7e107e36eef09c7fea69fcd4d3a195ad1d8a97f59d45c30a75"
    },
    {
      "path": "csih_message.h",
      "sha256": "de849b47393aa0d959c457e65c55d79b8b10fb012a925e9bced5ac8920704ed1"
    },
    {
      "path": "csih_message_io.c",
      "sha256": "1896c57678f6f5878f5d79aca46f6b9a783afdabb609a2a26b1de9fdc8f05e4e"
    },
    {
      "path": "csih_message_io.h",
      "sha256": "7bfa419eb11ebccf178c8b5e9f2ca358fde295607366c0069655b48cec511eea"
    },
    {
      "path": "edit.c",
      "sha256": "68f6f2299ee298d0a52886d85638b00b2a1b57afac8f4b3042a5861ca87cb0b0"
    },
    {
      "path": "edit_cli.c",
      "sha256": "a0b2ad025cd56bcfdd2fab12a405e15eb701cd60970a5369ce2db36dac909103"
    },
    {
      "path": "file.c",
      "sha256": "1d78a9c0d46891662f040642cdbcfe41d865e88c4501e1b645b87701b4d13b8c"
    },
    {
      "path": "file_cli.c",
      "sha256": "cc6e488d900999d2194f1d5ffcaeb2d2df9f4917c5b925863e7d01f7f983144d"
    },
    {
      "path": "gate.c",
      "sha256": "73dc78ceafcdef0f8c47d1ccdc078fd20fd93e24231e4f8a44af410fa39cdc97"
    },
    {
      "path": "gate_cli.c",
      "sha256": "28fede5ebc73dc4647d7e969c19440aa98f8103ada8190c1bec86e2836b8708f"
    },
    {
      "path": "https_post.c",
      "sha256": "a378b7e3466a1ff9c0ad2a8fbfddb4923ae4746fd71925213e665dfb76f9158a"
    },
    {
      "path": "journal_checkpoint.c",
      "sha256": "a806362a56e525b18c88f6341e8af7e71e136de3ae1a2e67bba4874861a25e25"
    },
    {
      "path": "journal_checkpoint_cli.c",
      "sha256": "9eec90f6a918cc23f1bd161f0dd00d9e6cf6fad20b13b4b4d5d65164315f71ce"
    },
    {
      "path": "json.c",
      "sha256": "486b7bab7cdc616e1221d50812cee16344a26a8cd9adba88efc4234cfeef266d"
    },
    {
      "path": "json_cli.c",
      "sha256": "3397d100c2a63f4886b78a99300d16176b37d418d4d2665d2a6a6ebc884a0087"
    },
    {
      "path": "llm.c",
      "sha256": "e8be1e63bc7ed3399a3ce105c71c912aa5b8d0d4aa23b36d69859758ac0055a1"
    },
    {
      "path": "llm_cli.c",
      "sha256": "23d710c6ee91fd361451e573b2ea3c8b8be3aa78542cd111de6afb78a21d0a0f"
    },
    {
      "path": "loop.c",
      "sha256": "e1bcffb255e033092070fa364440f7400b0cfef6f8bf12772407aba91dace311"
    },
    {
      "path": "net.c",
      "sha256": "c6bbe060dd14c152ea652aa163384da8b2f5e9c4b09856ec0f70868d5943730a"
    },
    {
      "path": "net_cli.c",
      "sha256": "573d1e5b2f4d98118fef7628227eeeb363636e029bcc3aa0a4a6dc26edb53a79"
    },
    {
      "path": "plugin.c",
      "sha256": "d02e82dcc20589fa9a540a8fbe60a0d8f1859a9a0535878291c2c87df393a4d4"
    },
    {
      "path": "plugin_api.h",
      "sha256": "3a0cd6b9eba1a518c5769cbe4711e626e51c0bf97bc3f28ca638f22f11c4b75e"
    },
    {
      "path": "plugin_call.c",
      "sha256": "ecd4bbc0b0d1f9237c8d53a664cbd4bc13b1a39b8f46c23b0c34198795700f8b"
    },
    {
      "path": "plugin_cli.c",
      "sha256": "5f1ab60e9d2af85ba93334600bf90e35d863d74951e5980e6f1478c4e438f73f"
    },
    {
      "path": "plugin_ping.c",
      "sha256": "c4abc0a499a4d6c4ba2e02b841bd443ebc5ff2e83c3d7fee8dcd6d5e16a0a1d6"
    },
    {
      "path": "probes/agent_roundtrip.c",
      "sha256": "0e241a7892c786441ae3753632857c611230351981305edc924ddbb77f16e8b1"
    },
    {
      "path": "probes/dns.c",
      "sha256": "67f61633720e5e1d52402faca170a79ff0d311c3332b72be4db8ae7d81950003"
    },
    {
      "path": "probes/dup_symbol.c",
      "sha256": "e0973f7bd8854e49ad21548a2f10a3e0c263706150e459206f493bfef0b1bd18"
    },
    {
      "path": "probes/errno_cover.c",
      "sha256": "b6f092026801bfe52414e9d720fc830206177bf6a51762ee7da9430898e52b79"
    },
    {
      "path": "probes/f22.c",
      "sha256": "cc07eb96b8ea9073ccb455b07f208605bd252fb7cf9db38afea82228d2374c4e"
    },
    {
      "path": "probes/fs-open.c",
      "sha256": "cbe170d0d0d78c15314082ed868d6ee06d0235100b03cc378343f11c554c0eca"
    },
    {
      "path": "probes/fs-stat.c",
      "sha256": "a2f2d487cfeb2600f709a807351935f9145a9b349cec1c9c607d0d49f90dd9a8"
    },
    {
      "path": "probes/http_roundtrip.c",
      "sha256": "a65037ea49e67e57bdc61fb719efc12cb87edf5fc9fff9777d1e3a08bae6c4a2"
    },
    {
      "path": "probes/libc_cover.c",
      "sha256": "617cd11be9c6db1840f5da66562b5143e63f33b6fad188c98dadbe5f47bc2854"
    },
    {
      "path": "probes/llm_roundtrip.c",
      "sha256": "3d05c05a033adbe0e10c2e624b6f7e863826327c614974eafdd8e3f5ad87094d"
    },
    {
      "path": "probes/net-tcp.c",
      "sha256": "7c4997a848b2ea122a366cef855c4b7591879a5018e0f122babe3a5d84fdaf91"
    },
    {
      "path": "probes/net/alone.c",
      "sha256": "e8d119d06bcd09f82a85e7d96bbebc3def1cd7ec4eeeaedccc109a96805bb1b6"
    },
    {
      "path": "probes/net/dns.c",
      "sha256": "068bf319137bdb7ca7f8a5abc39422380ad4f3391287113ec120d6942af1718a"
    },
    {
      "path": "probes/net/roundtrip.c",
      "sha256": "e865b6dc1caa80335f60fbd0fbc2cc34b522dd520af5eb87ac4380873338e18c"
    },
    {
      "path": "probes/net/withwait.c",
      "sha256": "ef0abc4fff55d7429d867ae70dfcee000932e4b7d2a4793089a204531187d058"
    },
    {
      "path": "probes/net_cover.c",
      "sha256": "b2b6399c8823897bc33b198b486380a423d895d96867b4f844ab8cfe00bbaa22"
    },
    {
      "path": "probes/poll-mux.c",
      "sha256": "fa7ea2b9699531c8d7f8ceded3911dd6032cdcc3df94613897a535171787a9aa"
    },
    {
      "path": "probes/run.c",
      "sha256": "89b6dadebfafdedd9555aa4ac000969b832d12f6caa815a8def799bec1b759e2"
    },
    {
      "path": "probes/tty-raw.c",
      "sha256": "79050e973afb0d23bd218c6e3c099cd9f721cc09b91880c78bd9fcdabbcba178"
    },
    {
      "path": "probes/tty-size.c",
      "sha256": "86b0552a40046b80160212d90aeba37bdce8e2e78945a087b34af2103b61a837"
    },
    {
      "path": "probes/tui-q5-sigaction.c",
      "sha256": "96b78baf92de0cca6c5bc24b7c5c03b46d65cef66c50db6c659b0409e53f4f3f"
    },
    {
      "path": "probes/tui-q5-signal.c",
      "sha256": "8b379c6c865f2839e815d3b029bc96db099379ac36492e79f0d1396f2be37b05"
    },
    {
      "path": "probes/tui-terms.c",
      "sha256": "2b5fe43fb11e34f50a4aed847a98941e31ac3fdb01ea423452dcd44443dfbb99"
    },
    {
      "path": "probes/tui_agent_e2e.c",
      "sha256": "ae4b356d4f9914349a8657d1ba0473d4d54b777c0b5eb6a4f5878748d4fa8c14"
    },
    {
      "path": "probes/tui_e2e.c",
      "sha256": "6556473a9570e32eb61cbc7e83dcf787b78de7e14f1410d287edc12dbc3cdd79"
    },
    {
      "path": "probes/tui_run.c",
      "sha256": "60686599402bcc6688fa59d612e1da887bf7dcc210e0e063fcb28b4a03be661f"
    },
    {
      "path": "probes/u_run.c",
      "sha256": "6e8f40555e9cdff423528a7ec2ac4e7960f79285136d878796441b014324569f"
    },
    {
      "path": "reload_consume.c",
      "sha256": "c709a58eac14728b45e68a77da51a9222ef687ea3d5a7badc9aeb95416646be3"
    },
    {
      "path": "reload_consume_cli.c",
      "sha256": "fd674d3721d6465b0a802d40a6643f553e562fe93be7e9d94cc51bd251c07835"
    },
    {
      "path": "reload_io.c",
      "sha256": "0faf638123d8edffd0e4711bc28149ff8fb890185a59cdc4db4cf5e3140d0c09"
    },
    {
      "path": "reload_io.h",
      "sha256": "30116b0aaf2b6e2ed613a32c85f0a59bf050c87a38b392cd0878b5461606d7ff"
    },
    {
      "path": "reload_io_cli.c",
      "sha256": "ad43d4670a9c332db41c86dbcb38cbc8dbe651044985c7d1047f3f5a6a8ce6a2"
    },
    {
      "path": "reload_load.c",
      "sha256": "cf6cfab12bdd5fc8202e0603846bd7f60aaaacaf020ad1d93621f6d5bdbfc178"
    },
    {
      "path": "reload_load_cli.c",
      "sha256": "f5f32cef78866b547e47fc1de6509ab819c750c0843a7c9620484703c4f1d944"
    },
    {
      "path": "reload_owner.c",
      "sha256": "bbfc29f894b57dc88529a56532fbba1dc6e7c7264fa69217b7bc8351643a2600"
    },
    {
      "path": "reload_owner.h",
      "sha256": "4a08df0dcdbae3385c0bd9030c6534666c7ba0cbcb6e8fc06f054e520655fa8a"
    },
    {
      "path": "reload_owner_cli.c",
      "sha256": "f8fb4d09ebf11b37bd9dda883b000eb7f00ce9e94742ca60f05046c78faeecda"
    },
    {
      "path": "reload_session.h",
      "sha256": "36fc17bc999a2d8dff9b587af3f5966d1c43ff599854babc1da602e71ecce673"
    },
    {
      "path": "reload_session_cli.c",
      "sha256": "c38cf14d6ec0bacd7fe8581455d2d950c0ed272ec76c498103ec75b3dbc83c06"
    },
    {
      "path": "reload_session_decode.c",
      "sha256": "001ba8d8002c8c595910408e2141ddcd1a56604834515924a81c666c2594fff2"
    },
    {
      "path": "reload_session_encode.c",
      "sha256": "3d48c48825eb9ccaadb05792f4d738907d357b97133f1a4edf03182db24b00d8"
    },
    {
      "path": "reload_state.c",
      "sha256": "acccc1a41b481af393a49a7d4596e2da6eece7e373e61b871457828b5ed05ebb"
    },
    {
      "path": "reload_state_cli.c",
      "sha256": "d4271a27ba35a731ac86c4a6b20fae5aa471e1f0ff4e8f49ba635de0f864f351"
    },
    {
      "path": "reload_support.inc",
      "sha256": "b3d1fa302b760f422bda4d9ca24e115b1391e0d38fa37a8d10646ff2a624f862"
    },
    {
      "path": "render.c",
      "sha256": "f7cca7e0016bf5425ec60c1a32a8dfd13359172f87a51eb09933e5b6060c8ae8"
    },
    {
      "path": "render_api.h",
      "sha256": "ba12ea866c5b21b503f4c842b9fd86462ebd945d9d60794b4e28b65b0ef49214"
    },
    {
      "path": "render_cli.c",
      "sha256": "c27fea5e2e358f14a32550dddcc71cc49f1eac4c6db795cb7d230ccd9d6992db"
    },
    {
      "path": "session.c",
      "sha256": "b051bd59687e37b92eacc8ad1fc59a2954cc55bcc756ee99bd35fdc173d9ea0d"
    },
    {
      "path": "session_cli.c",
      "sha256": "8e5d7774da35297669f42f419a8102586646efd0f31eda37b3530071acedd097"
    },
    {
      "path": "shell.c",
      "sha256": "b5478fccaec0f2cefccf24207484a250770262dadb913b5d90edb3a65aa59c83"
    },
    {
      "path": "shell_api.h",
      "sha256": "e23fb5c50b4e98928fbaa868bdd3b8392f50a91d34234db960e95b4c7b4ee2e8"
    },
    {
      "path": "shell_cli.c",
      "sha256": "d345bdfa55653d7ad0e136fff32fc418608f1b709d2e752c31db3956d4adb835"
    },
    {
      "path": "suite.c",
      "sha256": "ad36879113b5dcb0d69a03627b213190f55c42e17eaf8525e1a53b14ab898bc3"
    },
    {
      "path": "suite_cli.c",
      "sha256": "a8e51a95546347a00beda4b2db1bbd15ed33b48193f665582c91306b0362fd48"
    },
    {
      "path": "term.c",
      "sha256": "9bf1d01094b3ea0cc8a5df5f1d4cbe6465ff8ac3072f484757f5d2e2db3e6e70"
    },
    {
      "path": "term_api.h",
      "sha256": "dfe2a761b09249228fafb80b46c9d0b59cb05fe7b1f6ae924329e7bef31fd2cd"
    },
    {
      "path": "term_cli.c",
      "sha256": "c6ac53fd8a085f262523efd517162b613052976bed88238693902d51768fec29"
    },
    {
      "path": "tools.c",
      "sha256": "d12ed6c01d78672710cd6d6ff9315e45ed2353ebb70b8b49beeb5c877460ad25"
    },
    {
      "path": "tools/redact.c",
      "sha256": "6141d814d3d8496ea4d5d7f2ba9f148c45c6144474bf52bd4fd6a56dc95853c0"
    },
    {
      "path": "tools_cli.c",
      "sha256": "32581a9cfab60e4f6b6995c7ea26df492149263b9cf7be4fb4cce7742ad942f8"
    },
    {
      "path": "tui.c",
      "sha256": "cf201f4c39302c61ae443ad4e3f822a5d7a7bb24c52274d5c9038d3e2c292c3a"
    }
  ],
  "source_after": [
    {
      "path": "agent.c",
      "sha256": "e73d54bda58ccfc767aa3d80086755b0757ce65f266c7c86497c0503bb6d586a"
    },
    {
      "path": "agent_cli.c",
      "sha256": "73dcc70fee7e4ae342b6779d129dd3c8a95f47987da4cfa5e6eed200f912de88"
    },
    {
      "path": "agent_result.h",
      "sha256": "a44926692aeca38ae042c3cc4c7461a51ae0fd2a3acfa4412a9a992e5ceb3d7c"
    },
    {
      "path": "chat.c",
      "sha256": "20c77cb3d68a53db1865c13b597debd364501ec80307b622d81aa0c88f420c0c"
    },
    {
      "path": "chat_cli.c",
      "sha256": "8af314d6e7afff857d6dda32fcca8a04a6f687f8716a921c09dbf9e599bde3ea"
    },
    {
      "path": "check.c",
      "sha256": "96ae7d952c6f24c18191c8769832c433a206856c2ecad8ae4ea399f4014ee529"
    },
    {
      "path": "clock.c",
      "sha256": "860d620fa9709132fbed379b51560e59d951d2e71303f8f4b333cbc10ca537c0"
    },
    {
      "path": "clock_cli.c",
      "sha256": "b7db6a718f303c77950a6d3e80c6c6ff8ff9110aef7ba98fce30296b2190eab1"
    },
    {
      "path": "context_index.inc",
      "sha256": "bdf920c6fc5df88bf7ab27689784f86d02300070b1321bcfcec3dd2441de5b28"
    },
    {
      "path": "csih.c",
      "sha256": "c76e97da72dc910384f54e091a2f6b3e7c4a595641afd113ad88d26b944a4c38"
    },
    {
      "path": "csih_cols.h",
      "sha256": "b7d9337964e7805f698125723d11c44b7ecb01bc766f8245c54802bffcd57919"
    },
    {
      "path": "csih_home.h",
      "sha256": "4ce4308ad1042f8565e75655c7297965cef9e12467aede7fc66fdd52f037d351"
    },
    {
      "path": "csih_message.c",
      "sha256": "71caa134a5a5be7e107e36eef09c7fea69fcd4d3a195ad1d8a97f59d45c30a75"
    },
    {
      "path": "csih_message.h",
      "sha256": "de849b47393aa0d959c457e65c55d79b8b10fb012a925e9bced5ac8920704ed1"
    },
    {
      "path": "csih_message_io.c",
      "sha256": "1896c57678f6f5878f5d79aca46f6b9a783afdabb609a2a26b1de9fdc8f05e4e"
    },
    {
      "path": "csih_message_io.h",
      "sha256": "7bfa419eb11ebccf178c8b5e9f2ca358fde295607366c0069655b48cec511eea"
    },
    {
      "path": "edit.c",
      "sha256": "68f6f2299ee298d0a52886d85638b00b2a1b57afac8f4b3042a5861ca87cb0b0"
    },
    {
      "path": "edit_cli.c",
      "sha256": "a0b2ad025cd56bcfdd2fab12a405e15eb701cd60970a5369ce2db36dac909103"
    },
    {
      "path": "file.c",
      "sha256": "1d78a9c0d46891662f040642cdbcfe41d865e88c4501e1b645b87701b4d13b8c"
    },
    {
      "path": "file_cli.c",
      "sha256": "cc6e488d900999d2194f1d5ffcaeb2d2df9f4917c5b925863e7d01f7f983144d"
    },
    {
      "path": "gate.c",
      "sha256": "73dc78ceafcdef0f8c47d1ccdc078fd20fd93e24231e4f8a44af410fa39cdc97"
    },
    {
      "path": "gate_cli.c",
      "sha256": "28fede5ebc73dc4647d7e969c19440aa98f8103ada8190c1bec86e2836b8708f"
    },
    {
      "path": "https_post.c",
      "sha256": "a378b7e3466a1ff9c0ad2a8fbfddb4923ae4746fd71925213e665dfb76f9158a"
    },
    {
      "path": "journal_checkpoint.c",
      "sha256": "a806362a56e525b18c88f6341e8af7e71e136de3ae1a2e67bba4874861a25e25"
    },
    {
      "path": "journal_checkpoint_cli.c",
      "sha256": "9eec90f6a918cc23f1bd161f0dd00d9e6cf6fad20b13b4b4d5d65164315f71ce"
    },
    {
      "path": "json.c",
      "sha256": "486b7bab7cdc616e1221d50812cee16344a26a8cd9adba88efc4234cfeef266d"
    },
    {
      "path": "json_cli.c",
      "sha256": "3397d100c2a63f4886b78a99300d16176b37d418d4d2665d2a6a6ebc884a0087"
    },
    {
      "path": "llm.c",
      "sha256": "e8be1e63bc7ed3399a3ce105c71c912aa5b8d0d4aa23b36d69859758ac0055a1"
    },
    {
      "path": "llm_cli.c",
      "sha256": "23d710c6ee91fd361451e573b2ea3c8b8be3aa78542cd111de6afb78a21d0a0f"
    },
    {
      "path": "loop.c",
      "sha256": "e1bcffb255e033092070fa364440f7400b0cfef6f8bf12772407aba91dace311"
    },
    {
      "path": "net.c",
      "sha256": "c6bbe060dd14c152ea652aa163384da8b2f5e9c4b09856ec0f70868d5943730a"
    },
    {
      "path": "net_cli.c",
      "sha256": "573d1e5b2f4d98118fef7628227eeeb363636e029bcc3aa0a4a6dc26edb53a79"
    },
    {
      "path": "plugin.c",
      "sha256": "d02e82dcc20589fa9a540a8fbe60a0d8f1859a9a0535878291c2c87df393a4d4"
    },
    {
      "path": "plugin_api.h",
      "sha256": "3a0cd6b9eba1a518c5769cbe4711e626e51c0bf97bc3f28ca638f22f11c4b75e"
    },
    {
      "path": "plugin_call.c",
      "sha256": "ecd4bbc0b0d1f9237c8d53a664cbd4bc13b1a39b8f46c23b0c34198795700f8b"
    },
    {
      "path": "plugin_cli.c",
      "sha256": "5f1ab60e9d2af85ba93334600bf90e35d863d74951e5980e6f1478c4e438f73f"
    },
    {
      "path": "plugin_ping.c",
      "sha256": "c4abc0a499a4d6c4ba2e02b841bd443ebc5ff2e83c3d7fee8dcd6d5e16a0a1d6"
    },
    {
      "path": "probes/agent_roundtrip.c",
      "sha256": "0e241a7892c786441ae3753632857c611230351981305edc924ddbb77f16e8b1"
    },
    {
      "path": "probes/dns.c",
      "sha256": "67f61633720e5e1d52402faca170a79ff0d311c3332b72be4db8ae7d81950003"
    },
    {
      "path": "probes/dup_symbol.c",
      "sha256": "e0973f7bd8854e49ad21548a2f10a3e0c263706150e459206f493bfef0b1bd18"
    },
    {
      "path": "probes/errno_cover.c",
      "sha256": "b6f092026801bfe52414e9d720fc830206177bf6a51762ee7da9430898e52b79"
    },
    {
      "path": "probes/f22.c",
      "sha256": "cc07eb96b8ea9073ccb455b07f208605bd252fb7cf9db38afea82228d2374c4e"
    },
    {
      "path": "probes/fs-open.c",
      "sha256": "cbe170d0d0d78c15314082ed868d6ee06d0235100b03cc378343f11c554c0eca"
    },
    {
      "path": "probes/fs-stat.c",
      "sha256": "a2f2d487cfeb2600f709a807351935f9145a9b349cec1c9c607d0d49f90dd9a8"
    },
    {
      "path": "probes/http_roundtrip.c",
      "sha256": "a65037ea49e67e57bdc61fb719efc12cb87edf5fc9fff9777d1e3a08bae6c4a2"
    },
    {
      "path": "probes/libc_cover.c",
      "sha256": "617cd11be9c6db1840f5da66562b5143e63f33b6fad188c98dadbe5f47bc2854"
    },
    {
      "path": "probes/llm_roundtrip.c",
      "sha256": "3d05c05a033adbe0e10c2e624b6f7e863826327c614974eafdd8e3f5ad87094d"
    },
    {
      "path": "probes/net-tcp.c",
      "sha256": "7c4997a848b2ea122a366cef855c4b7591879a5018e0f122babe3a5d84fdaf91"
    },
    {
      "path": "probes/net/alone.c",
      "sha256": "e8d119d06bcd09f82a85e7d96bbebc3def1cd7ec4eeeaedccc109a96805bb1b6"
    },
    {
      "path": "probes/net/dns.c",
      "sha256": "068bf319137bdb7ca7f8a5abc39422380ad4f3391287113ec120d6942af1718a"
    },
    {
      "path": "probes/net/roundtrip.c",
      "sha256": "e865b6dc1caa80335f60fbd0fbc2cc34b522dd520af5eb87ac4380873338e18c"
    },
    {
      "path": "probes/net/withwait.c",
      "sha256": "ef0abc4fff55d7429d867ae70dfcee000932e4b7d2a4793089a204531187d058"
    },
    {
      "path": "probes/net_cover.c",
      "sha256": "b2b6399c8823897bc33b198b486380a423d895d96867b4f844ab8cfe00bbaa22"
    },
    {
      "path": "probes/poll-mux.c",
      "sha256": "fa7ea2b9699531c8d7f8ceded3911dd6032cdcc3df94613897a535171787a9aa"
    },
    {
      "path": "probes/run.c",
      "sha256": "89b6dadebfafdedd9555aa4ac000969b832d12f6caa815a8def799bec1b759e2"
    },
    {
      "path": "probes/tty-raw.c",
      "sha256": "79050e973afb0d23bd218c6e3c099cd9f721cc09b91880c78bd9fcdabbcba178"
    },
    {
      "path": "probes/tty-size.c",
      "sha256": "86b0552a40046b80160212d90aeba37bdce8e2e78945a087b34af2103b61a837"
    },
    {
      "path": "probes/tui-q5-sigaction.c",
      "sha256": "96b78baf92de0cca6c5bc24b7c5c03b46d65cef66c50db6c659b0409e53f4f3f"
    },
    {
      "path": "probes/tui-q5-signal.c",
      "sha256": "8b379c6c865f2839e815d3b029bc96db099379ac36492e79f0d1396f2be37b05"
    },
    {
      "path": "probes/tui-terms.c",
      "sha256": "2b5fe43fb11e34f50a4aed847a98941e31ac3fdb01ea423452dcd44443dfbb99"
    },
    {
      "path": "probes/tui_agent_e2e.c",
      "sha256": "ae4b356d4f9914349a8657d1ba0473d4d54b777c0b5eb6a4f5878748d4fa8c14"
    },
    {
      "path": "probes/tui_e2e.c",
      "sha256": "6556473a9570e32eb61cbc7e83dcf787b78de7e14f1410d287edc12dbc3cdd79"
    },
    {
      "path": "probes/tui_run.c",
      "sha256": "60686599402bcc6688fa59d612e1da887bf7dcc210e0e063fcb28b4a03be661f"
    },
    {
      "path": "probes/u_run.c",
      "sha256": "6e8f40555e9cdff423528a7ec2ac4e7960f79285136d878796441b014324569f"
    },
    {
      "path": "reload_consume.c",
      "sha256": "c709a58eac14728b45e68a77da51a9222ef687ea3d5a7badc9aeb95416646be3"
    },
    {
      "path": "reload_consume_cli.c",
      "sha256": "fd674d3721d6465b0a802d40a6643f553e562fe93be7e9d94cc51bd251c07835"
    },
    {
      "path": "reload_io.c",
      "sha256": "0faf638123d8edffd0e4711bc28149ff8fb890185a59cdc4db4cf5e3140d0c09"
    },
    {
      "path": "reload_io.h",
      "sha256": "30116b0aaf2b6e2ed613a32c85f0a59bf050c87a38b392cd0878b5461606d7ff"
    },
    {
      "path": "reload_io_cli.c",
      "sha256": "ad43d4670a9c332db41c86dbcb38cbc8dbe651044985c7d1047f3f5a6a8ce6a2"
    },
    {
      "path": "reload_load.c",
      "sha256": "cf6cfab12bdd5fc8202e0603846bd7f60aaaacaf020ad1d93621f6d5bdbfc178"
    },
    {
      "path": "reload_load_cli.c",
      "sha256": "f5f32cef78866b547e47fc1de6509ab819c750c0843a7c9620484703c4f1d944"
    },
    {
      "path": "reload_owner.c",
      "sha256": "bbfc29f894b57dc88529a56532fbba1dc6e7c7264fa69217b7bc8351643a2600"
    },
    {
      "path": "reload_owner.h",
      "sha256": "4a08df0dcdbae3385c0bd9030c6534666c7ba0cbcb6e8fc06f054e520655fa8a"
    },
    {
      "path": "reload_owner_cli.c",
      "sha256": "f8fb4d09ebf11b37bd9dda883b000eb7f00ce9e94742ca60f05046c78faeecda"
    },
    {
      "path": "reload_session.h",
      "sha256": "36fc17bc999a2d8dff9b587af3f5966d1c43ff599854babc1da602e71ecce673"
    },
    {
      "path": "reload_session_cli.c",
      "sha256": "c38cf14d6ec0bacd7fe8581455d2d950c0ed272ec76c498103ec75b3dbc83c06"
    },
    {
      "path": "reload_session_decode.c",
      "sha256": "001ba8d8002c8c595910408e2141ddcd1a56604834515924a81c666c2594fff2"
    },
    {
      "path": "reload_session_encode.c",
      "sha256": "3d48c48825eb9ccaadb05792f4d738907d357b97133f1a4edf03182db24b00d8"
    },
    {
      "path": "reload_state.c",
      "sha256": "acccc1a41b481af393a49a7d4596e2da6eece7e373e61b871457828b5ed05ebb"
    },
    {
      "path": "reload_state_cli.c",
      "sha256": "d4271a27ba35a731ac86c4a6b20fae5aa471e1f0ff4e8f49ba635de0f864f351"
    },
    {
      "path": "reload_support.inc",
      "sha256": "b3d1fa302b760f422bda4d9ca24e115b1391e0d38fa37a8d10646ff2a624f862"
    },
    {
      "path": "render.c",
      "sha256": "f7cca7e0016bf5425ec60c1a32a8dfd13359172f87a51eb09933e5b6060c8ae8"
    },
    {
      "path": "render_api.h",
      "sha256": "ba12ea866c5b21b503f4c842b9fd86462ebd945d9d60794b4e28b65b0ef49214"
    },
    {
      "path": "render_cli.c",
      "sha256": "c27fea5e2e358f14a32550dddcc71cc49f1eac4c6db795cb7d230ccd9d6992db"
    },
    {
      "path": "session.c",
      "sha256": "b051bd59687e37b92eacc8ad1fc59a2954cc55bcc756ee99bd35fdc173d9ea0d"
    },
    {
      "path": "session_cli.c",
      "sha256": "8e5d7774da35297669f42f419a8102586646efd0f31eda37b3530071acedd097"
    },
    {
      "path": "shell.c",
      "sha256": "b5478fccaec0f2cefccf24207484a250770262dadb913b5d90edb3a65aa59c83"
    },
    {
      "path": "shell_api.h",
      "sha256": "e23fb5c50b4e98928fbaa868bdd3b8392f50a91d34234db960e95b4c7b4ee2e8"
    },
    {
      "path": "shell_cli.c",
      "sha256": "d345bdfa55653d7ad0e136fff32fc418608f1b709d2e752c31db3956d4adb835"
    },
    {
      "path": "suite.c",
      "sha256": "ad36879113b5dcb0d69a03627b213190f55c42e17eaf8525e1a53b14ab898bc3"
    },
    {
      "path": "suite_cli.c",
      "sha256": "a8e51a95546347a00beda4b2db1bbd15ed33b48193f665582c91306b0362fd48"
    },
    {
      "path": "term.c",
      "sha256": "9bf1d01094b3ea0cc8a5df5f1d4cbe6465ff8ac3072f484757f5d2e2db3e6e70"
    },
    {
      "path": "term_api.h",
      "sha256": "dfe2a761b09249228fafb80b46c9d0b59cb05fe7b1f6ae924329e7bef31fd2cd"
    },
    {
      "path": "term_cli.c",
      "sha256": "c6ac53fd8a085f262523efd517162b613052976bed88238693902d51768fec29"
    },
    {
      "path": "tools.c",
      "sha256": "d12ed6c01d78672710cd6d6ff9315e45ed2353ebb70b8b49beeb5c877460ad25"
    },
    {
      "path": "tools/redact.c",
      "sha256": "6141d814d3d8496ea4d5d7f2ba9f148c45c6144474bf52bd4fd6a56dc95853c0"
    },
    {
      "path": "tools_cli.c",
      "sha256": "32581a9cfab60e4f6b6995c7ea26df492149263b9cf7be4fb4cce7742ad942f8"
    },
    {
      "path": "tui.c",
      "sha256": "cf201f4c39302c61ae443ad4e3f822a5d7a7bb24c52274d5c9038d3e2c292c3a"
    }
  ]
}