#!/usr/bin/env python3
"""Validate application-captured snapshots without comparing live identities.
Synthetic controls exercise the validators; they never replace a live capture.
"""
import re

PID_MAX = (1 << 31) - 1
INT_MAX = (1 << 63) - 1
ADDRESS_MAX = (1 << 64) - 1


def rows(data):
    lines = data.splitlines()
    if not lines or any(not line.strip() for line in lines):
        raise RuntimeError('empty snapshot or empty row')
    return lines


def processes(data):
    seen = set()
    for line in rows(data):
        match = re.fullmatch(rb'([0-9]+) ([0-9]+) ([0-9]+) (.+)', line)
        if not match:
            raise RuntimeError('malformed process row')
        pid, parent, rss = map(int, match.groups()[:3])
        # Missing parents are legitimate when processes exit during collection.
        if not 0 < pid <= PID_MAX or not 0 <= parent <= PID_MAX or parent == pid:
            raise RuntimeError('invalid PID or parent PID')
        if pid in seen or rss > INT_MAX:
            raise RuntimeError('duplicate PID or invalid RSS')
        seen.add(pid)
    return len(seen)


def maps(data):
    previous_end = 0
    count = 0
    for line in rows(data):
        match = re.fullmatch(rb'([0-9a-fA-F]+)-([0-9a-fA-F]+) ([r-][w-][x-][ps]) ([0-9a-fA-F]+) ([0-9a-fA-F]+):([0-9a-fA-F]+) ([0-9]+)(?: (.*))?', line)
        if not match:
            raise RuntimeError('malformed map row or permissions')
        start, end, offset = (int(match.group(i), 16) for i in (1, 2, 4))
        inode = int(match.group(7))
        if not 0 <= start < end <= ADDRESS_MAX or start < previous_end:
            raise RuntimeError('empty, reversed, unordered or overlapping map range')
        if offset > ADDRESS_MAX or inode > ADDRESS_MAX:
            raise RuntimeError('map metadata outside u64 range')
        previous_end = end
        count += 1
    return count


def windows(data):
    lines = rows(data)
    screen = re.fullmatch(rb'screen ([0-9]+) ([0-9]+)', lines[0])
    if not screen or len(lines) < 2:
        raise RuntimeError('missing screen dimensions or windows')
    width, height = map(int, screen.groups())
    if not 0 < width <= 16384 or not 0 < height <= 16384:
        raise RuntimeError('invalid screen dimensions')
    for line in lines[1:]:
        match = re.fullmatch(rb'(-?[0-9]+) (-?[0-9]+) ([0-9]+) ([0-9]+) (.+)', line)
        if not match:
            raise RuntimeError('malformed window rectangle')
        x, y, w, h = map(int, match.groups()[:4])
        # Off-screen and duplicate geometry are valid; empty/overflow is not.
        if not 0 < w <= INT_MAX or not 0 < h <= INT_MAX or w * h > INT_MAX:
            raise RuntimeError('empty or overflowing window rectangle')
        if any(v < -INT_MAX - 1 or v > INT_MAX for v in (x, y, x + w, y + h)):
            raise RuntimeError('window coordinate outside signed range')
    return len(lines) - 1


def process_report(data):
    if b'== process tree (' not in data:
        raise RuntimeError('live process collection missing structure')
    unknown = re.search(rb'\([0-9]+\) +\?(?: +sum |\n)', data)
    explanation = re.search(rb'RSS of ([0-9]+) other users.*needs privileges, shown as \?', data)
    if unknown and not explanation:
        raise RuntimeError('unreadable RSS lacks privilege explanation')
    if explanation and int(explanation.group(1)) < 1:
        raise RuntimeError('invalid unreadable RSS count')


def window_list(data, all_windows=False):
    lines = data.splitlines()
    if not lines:
        raise RuntimeError('empty window list')
    header = re.fullmatch(rb'== ([0-9]+) windows \((all|on-screen)\), front to back; ([0-9]+) on screen, ([0-9]+) with a readable title', lines[0])
    if not header or header.group(2) != (b'all' if all_windows else b'on-screen'):
        raise RuntimeError('invalid window list mode/header')
    count, on_count, title_count = (int(header.group(i)) for i in (1, 3, 4))
    if not 0 < count <= 1024 or not 0 <= on_count <= count or not 0 <= title_count <= count:
        raise RuntimeError('invalid window list counts')
    if len(lines) < count + 3 or lines[1].split() != [b'id', b'pid', b'layer', b'on', b'alpha', b'x', b'y', b'w', b'h', b'owner', b'/', b'title']:
        raise RuntimeError('missing window list columns')
    ids, owners, layers = set(), {}, {}
    actual_on = actual_titles = 0
    positive_visible = 0
    for line in lines[2:2 + count]:
        match = re.fullmatch(rb' *([0-9]+) +([0-9]+) +(-?[0-9]+) +([yn]) +([0-9]+\.[0-9]{2}) +(-?[0-9]+) +(-?[0-9]+) +([0-9]+) +([0-9]+)  (.+?) / (.+)', line)
        if not match:
            raise RuntimeError('malformed window list row')
        ident, pid, layer = (int(match.group(i)) for i in (1, 2, 3))
        on = match.group(4) == b'y'
        alpha = float(match.group(5))
        x, y, w, h = (int(match.group(i)) for i in (6, 7, 8, 9))
        owner, title = match.group(10, 11)
        if not 0 < ident <= (1 << 32) - 1 or ident in ids or not 0 < pid <= PID_MAX:
            raise RuntimeError('invalid or duplicate window id/PID')
        if not 0 <= alpha <= 1 or abs(layer) > PID_MAX:
            raise RuntimeError('invalid window alpha/layer')
        if any(abs(v) > INT_MAX for v in (x, y, w, h, x + w, y + h)) or w * h > INT_MAX:
            raise RuntimeError('invalid window list rectangle')
        if not all_windows and not on:
            raise RuntimeError('hidden window in on-screen list')
        ids.add(ident)
        actual_on += on
        positive_visible += on and w > 0 and h > 0
        actual_titles += title != b'-'
        key = (owner, pid)
        n, visible = owners.get(key, (0, 0))
        owners[key] = (n + 1, visible + on)
        layers[layer] = layers.get(layer, 0) + 1
    if (actual_on, actual_titles) != (on_count, title_count):
        raise RuntimeError('window header disagrees with rows')
    rest = b'\n'.join(lines[2 + count:])
    app_marker = b'\n== windows per application\n'
    layer_marker = b'\n== windows per layer (0 is the normal application layer)\n'
    largest_marker = b'\n== largest on-screen windows (points)\n'
    if not rest.startswith(app_marker) or layer_marker not in rest or largest_marker not in rest:
        raise RuntimeError('missing window aggregate sections')
    app_text, tail = rest[len(app_marker):].split(layer_marker, 1)
    layer_text, largest_text = tail.split(largest_marker, 1)
    actual_owners = {}
    for line in app_text.strip().splitlines():
        match = re.fullmatch(rb'(.+?) +pid +([0-9]+) +([0-9]+) windows? *, ([0-9]+) on screen', line)
        if not match:
            raise RuntimeError('malformed application aggregate')
        key = (match.group(1).rstrip(), int(match.group(2)))
        if key in actual_owners:
            raise RuntimeError('duplicate application aggregate')
        actual_owners[key] = (int(match.group(3)), int(match.group(4)))
    actual_layers = {}
    for line in layer_text.strip().splitlines():
        match = re.fullmatch(rb'layer +(-?[0-9]+) +([0-9]+)', line)
        if not match or int(match.group(1)) in actual_layers:
            raise RuntimeError('malformed or duplicate layer aggregate')
        actual_layers[int(match.group(1))] = int(match.group(2))
    if actual_owners != owners or actual_layers != layers:
        raise RuntimeError('window aggregates disagree with rows')
    # Areas use original double bounds; printed rounded coordinates cannot
    # reconstruct them exactly. Check syntax/order, not a false equality.
    previous = float('inf')
    largest_count = 0
    for line in largest_text.strip().splitlines():
        if re.fullmatch(rb'\([0-9]+ windows reported, [0-9]+ listed\)', line):
            continue
        if not line:
            continue
        match = re.fullmatch(rb' *([0-9]+)  .+ / .+', line)
        if not match or int(match.group(1)) > previous:
            raise RuntimeError('invalid largest-window area/order')
        previous = int(match.group(1))
        largest_count += 1
    if not min(5, positive_visible) <= largest_count <= min(5, on_count):
        raise RuntimeError('missing or excess largest windows')
    return count


def check_controls():
    good = [
        (processes, b'1 0 0 init\n2 1 1024 child\n3 99 0 orphan\n'),
        (maps, b'0-1000 ---p 0 00:00 0 \n1000-2000 rw-p 0 00:00 0 [heap]\n3000-4000 r-xs 10 01:02 7 /image\n'),
        (windows, b'screen 100 100\n-20 0 50 20 off screen\n0 0 50 20 same\n0 0 50 20 same\n'),
        (process_report, b'== process tree (1 processes, 0 resident)\ninit (1)  0.0M\n'),
        (process_report, b"== process tree (1 processes, 0 resident; RSS of 1 other users' processes needs privileges, shown as ?)\ninit (1)      ?\n"),
    ]
    listing = (b'== 1 windows (on-screen), front to back; 1 on screen, 0 with a readable title\n'
               b'    id    pid layer on alpha       x       y      w      h  owner / title\n'
               b'    10      1     0  y  1.00       0       0    100    100  App / -\n'
               b'\n== windows per application\nApp                              pid 1        1 window , 1 on screen\n'
               b'\n== windows per layer (0 is the normal application layer)\nlayer 0        1\n'
               b'\n== largest on-screen windows (points)\n    10000  App / -\n')
    good.extend([(window_list, listing),
                 (lambda data: window_list(data, True), listing.replace(b'(on-screen)', b'(all)'))])
    bad = [(fn, b'') for fn in (processes, maps, windows, process_report)] + [
        (processes, b'1 0 0 init\nmalformed\n'),
        (processes, b'1 0 0 init\n1 0 0 duplicate\n'),
        (processes, b'0 0 0 bad\n'),
        (processes, b'1 1 0 self\n'),
        (processes, b'1 -1 0 parent\n'),
        (processes, b'1 0 -1 rss\n'),
        (processes, b'2147483648 0 0 pid\n'),
        (maps, b'1000-1000 rw-p 0 00:00 0\n'),
        (maps, b'2000-1000 rw-p 0 00:00 0\n'),
        (maps, b'1000-2000 rw-p 0 00:00 0\n1000-2000 rw-p 0 00:00 0\n'),
        (maps, b'1000-3000 rw-p 0 00:00 0\n2000-4000 rw-p 0 00:00 0\n'),
        (maps, b'3000-4000 rw-p 0 00:00 0\n1000-2000 rw-p 0 00:00 0\n'),
        (maps, b'1000-2000 rw-q 0 00:00 0\n'),
        (maps, b'0-10000000000000000 rw-p 0 00:00 0\n'),
        (maps, b'1000-2000 rw-p invalid 00:00 0\n'),
        (windows, b'screen 100 100\n'),
        (windows, b'screen 0 100\n0 0 10 10 bad\n'),
        (windows, b'screen 16385 100\n0 0 10 10 bad\n'),
        (windows, b'screen 100 100\n0 0 0 10 empty\n'),
        (windows, b'screen 100 100\n0 0 10 10\n'),
        (windows, b'screen 100 100\nscreen 100 100\n'),
        (windows, b'screen 100 100\n0 0 9223372036854775807 2 overflow\n'),
        (process_report, b'== process tree (1 processes)\ninit (1)      ?\n'),
        (process_report, b"== process tree (1 processes; RSS of 0 other users' processes needs privileges, shown as ?)\n"),
    ]
    row = listing.splitlines()[2] + b'\n'
    duplicate_list = listing.replace(b'1 windows (', b'2 windows (').replace(
        b'1 on screen, 0 with', b'2 on screen, 0 with').replace(row, row + row)
    bad.extend([(processes, b'1 0 9223372036854775808 rss\n'),
                (windows, b'screen 100 100\n9223372036854775807 0 1 1 overflow\n'),
                (window_list, duplicate_list), (window_list, b''), (window_list, listing.replace(b'1 on screen,', b'0 on screen,')),
                (window_list, listing.replace(b'    10      1', b'     0      1')),
                (window_list, listing.replace(b'  1.00', b'  2.00')),
                (window_list, listing.replace(b'1 window ,', b'2 windows,')),
                (window_list, listing.replace(b'    10000  App / -\n', b'')),
                (window_list, listing.replace(b'(on-screen)', b'(all)'))])
    for fn, data in good:
        fn(data)
    for fn, data in bad:
        try:
            fn(data)
        except RuntimeError:
            continue
        raise RuntimeError(f'{fn.__name__}: accepted negative control {data!r}')
    print(f'app snapshot controls: {len(good)} positive, {len(bad)} negative passed')


if __name__ == '__main__':
    check_controls()
