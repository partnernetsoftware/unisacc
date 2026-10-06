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


class ZData:
    """Parse-time data area: byte segments and zero segments (a zero segment
    is only a length).  A large zero global (unisacc's own 32 MB buffers sit
    between initialised tables) costs nothing until a byte in it is written.
    Supports what the front ends and lower use: len, extend, slice read and
    write, count(0, s, e), and copy_into for loaders."""
    BIG = 4096

    def __init__(self):
        self.segs = []          # [start, bytearray | int]
        self.n = 0

    def __len__(self):
        return self.n

    def extend(self, raw):
        k = len(raw)
        if not k:
            return
        if k >= self.BIG and raw.count(0) == k:
            if self.segs and isinstance(self.segs[-1][1], int):
                self.segs[-1][1] += k
            else:
                self.segs.append([self.n, k])
        elif self.segs and not isinstance(self.segs[-1][1], int):
            self.segs[-1][1].extend(raw)
        else:
            self.segs.append([self.n, bytearray(raw)])
        self.n += k

    def _span(self, s, e):
        import bisect
        i = max(bisect.bisect_right([g[0] for g in self.segs], s) - 1, 0)
        while i < len(self.segs) and self.segs[i][0] < e:
            a, obj = self.segs[i]
            b = a + (obj if isinstance(obj, int) else len(obj))
            if b > s:
                yield i, a, obj, max(s, a), min(e, b)
            i += 1

    def _range(self, key):
        s, e, step = key.indices(self.n)
        assert step == 1, 'ZData: stepped slice'
        return s, max(s, e)

    def __getitem__(self, key):
        if not isinstance(key, slice):
            return self[key:key + 1][0]
        s, e = self._range(key)
        out = bytearray(e - s)
        for _, a, obj, lo, hi in self._span(s, e):
            if not isinstance(obj, int):
                out[lo - s:hi - s] = obj[lo - a:hi - a]
        return bytes(out)

    def __setitem__(self, key, raw):
        s, e = self._range(key)
        raw = bytes(raw)
        assert len(raw) == e - s, 'ZData: slice write changes the length'
        split = []
        for i, a, obj, lo, hi in self._span(s, e):
            part = raw[lo - s:hi - s]
            if not isinstance(obj, int):
                obj[lo - a:hi - a] = part
            elif part.count(0) != len(part):
                split.append((i, a, obj, lo, hi, part))
        for i, a, obj, lo, hi, part in reversed(split):
            new = []
            if lo > a: new.append([a, lo - a])
            new.append([lo, bytearray(part)])
            if a + obj > hi: new.append([hi, a + obj - hi])
            self.segs[i:i + 1] = new

    def count(self, x, s=0, e=None):
        assert x == 0, 'ZData.count counts zeros only'
        e = self.n if e is None else min(e, self.n)
        total = 0
        for _, a, obj, lo, hi in self._span(s, e):
            total += hi - lo if isinstance(obj, int) else obj.count(0, lo - a, hi - a)
        return total

    def copy_into(self, mem, base):
        for a, obj in self.segs:
            if not isinstance(obj, int):
                mem[base + a:base + a + len(obj)] = obj


def copy_data(mem, base, data):
    """Load a data area (bytearray, Padded or ZData) at mem[base:]; zeros are already zero."""
    if isinstance(data, ZData):
        data.copy_into(mem, base)
    else:
        view = memoryview(data)
        mem[base:base + len(view)] = view
