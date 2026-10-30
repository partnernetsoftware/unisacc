"""mulberry32 + Box-Muller.  [D-3] [N-5]

Bit-exact 32-bit arithmetic at every step so weights are byte-reproducible
across runs and machines.  Training may use libm; deploy may not.
"""
import math

M32 = 0xFFFFFFFF
TWO32 = 4294967296.0


class Rng:
    __slots__ = ("s",)

    def __init__(self, seed):
        self.s = seed & M32

    def u32(self):
        self.s = (self.s + 0x6D2B79F5) & M32
        t = self.s
        t = ((t ^ (t >> 15)) * (t | 1)) & M32
        t = (t ^ ((t + (((t ^ (t >> 7)) * (t | 61)) & M32)) & M32)) & M32
        return (t ^ (t >> 14)) & M32

    def uniform(self):
        return self.u32() / TWO32

    def gauss(self, sigma=1.0):
        """Box-Muller on two consecutive uniforms; sine half discarded. [N-5]"""
        u1 = self.uniform()
        u2 = self.uniform()
        if u1 < 1e-300:
            u1 = 1e-300
        return sigma * math.sqrt(-2.0 * math.log(u1)) * math.cos(2.0 * math.pi * u2)

    def perm(self, n):
        """Deterministic Fisher-Yates. Used for per-epoch batch order. [D-3]"""
        a = list(range(n))
        for i in range(n - 1, 0, -1):
            j = self.u32() % (i + 1)
            a[i], a[j] = a[j], a[i]
        return a
