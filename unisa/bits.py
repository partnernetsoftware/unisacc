"""Integer helpers shared by the seed's encoders, image writers and VM.

Four image writers each carried their own `_round`, and five modules spelt
the 64-bit mask inline; one definition keeps them from drifting.
"""

MASK64 = (1 << 64) - 1


def round_up(v, a):
    """v rounded up to a multiple of a."""
    return (v + a - 1) // a * a
