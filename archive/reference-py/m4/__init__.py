"""m4：流×网表驱动的 \\0asm 最小路径（合同见 research/m4-flow-net-contract-draft.md）。

主张弧：gold → IntNet → 四流 → 字节。ujs 的旧路径（ujs/、tests/ujs2wasm）保留为参考，
check 子命令逐字节对比它。M3 不在本轨主张里。
"""
from .contract import FLOWS, NETS, wired  # noqa: F401
