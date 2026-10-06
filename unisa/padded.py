"""Data whose zero tail is a length, not bytes.

`bytearray(n)` memsets (CPython 3.14 on macOS: 1 GiB -> 1.09 GB resident), so
a zero tail held as real bytes costs its full size.  `Padded` holds only the
stored prefix; `len()` is the full length.  Buffer users (memoryview, rstrip,
slicing) see the prefix, which is exactly what an image stores."""


class Padded(bytearray):
    def __init__(self, prefix=b"", full=0):
        super().__init__(prefix)
        self.full = max(full, super().__len__())

    def __len__(self):
        return self.full

    def stored(self):
        return super().__len__()
