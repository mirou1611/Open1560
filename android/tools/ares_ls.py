#!/usr/bin/env python3
"""List the contents of an ARES archive (core.ar, 1560.ar, ui.ar, audio.ar).

Only the index is needed, so a truncated copy of the front of the archive works:

    adb exec-out "dd if=/sdcard/.../core.ar bs=4096 count=52" > core_index.bin
    python ares_ls.py core_index.bin              # whole tree
    python ares_ls.py core_index.bin CHICAGOLM    # full paths containing a string

Two things about the format are easy to get wrong from a raw dump, and both cost
real time on this port:

  * a byte 0x01 inside a stored name is a placeholder for the node's own integer,
    so "CULL\\x01_H" with integer 1 is "CULL01_H" (VirtualFileSystem::ExpandName)
  * the extension is held in a separate field, so a leaf that dumps as "CULL01_H"
    actually resolves as "CULL01_H.BMS"

Lookups are case-insensitive in effect - VirtualFileSystem::NormalizeName upper-cases
the requested path - and directories nest, so a mesh is BMS/<name>/<group>.bms rather
than a single flat name.
"""
import struct
import sys

path = sys.argv[1]
want = sys.argv[2] if len(sys.argv) > 2 else None

data = open(path, "rb").read()
magic, node_count, root_count, names_size = struct.unpack_from("<4I", data, 0)

if magic != 0x53455241:
    sys.exit(f"not an ARES archive (magic {magic:#x})")

nodes_off = 0x10
names_off = nodes_off + node_count * 12
names = data[names_off:names_off + names_size]

if len(names) < names_size:
    sys.exit(f"index truncated: need {names_off + names_size} bytes, have {len(data)}")


def node(i):
    return struct.unpack_from("<3I", data, nodes_off + i * 12)


def name_of(i):
    _, f4, f8 = node(i)

    name_off = (f8 >> 14) & 0x3FFFF
    value = (f8 >> 1) & 0x1FFF

    raw = names[name_off:names.index(b"\0", name_off)]
    out = "".join(str(value) if c == 1 else chr(c) for c in raw)

    ext_off = (f4 >> 23) & 0x1FF

    if ext_off:
        out += "." + names[ext_off:names.index(b"\0", ext_off)].decode()

    return out


def walk(start, count, prefix, depth):
    for i in range(start, start + count):
        f0, f4, f8 = node(i)
        name = name_of(i)
        full = prefix + name

        if f8 & 1:
            if want is None:
                print(f"{'  ' * depth}{name}/  ({f4 & 0x7FFFFF} entries)")
            walk(f0, f4 & 0x7FFFFF, full + "/", depth + 1)
        elif want is None:
            print(f"{'  ' * depth}{name}")
        elif want.upper() in full.upper():
            print(full)


print(f"nodes={node_count} roots={root_count} names={names_size}", file=sys.stderr)
walk(0, root_count, "", 0)
