#!/usr/bin/env python3
"""
Script to dump the contents of a metatiles.bin file as human-readable JSON.

Binary format (2 bytes per entry, little-endian):
  Bits 0-9:   tile_index
  Bit 10:     h_flip
  Bit 11:     v_flip
  Bits 12-15: pal_index

Usage:
    uv run scripts/dump_metatiles_json.py <path_to_metatiles.bin>
"""

import argparse
import json
import struct
import sys


def parse_metatiles_bin(path):
    with open(path, "rb") as f:
        data = f.read()

    if len(data) % 2 != 0:
        print(
            f"Error: File size ({len(data)} bytes) is not a multiple of 2, possibly corrupted.",
            file=sys.stderr,
        )
        sys.exit(1)

    entries = []
    for index, offset in enumerate(range(0, len(data), 2)):
        (entry_bits,) = struct.unpack_from("<H", data, offset)
        entries.append(
            {
                "index": index,
                "entry": {
                    "tile_index": entry_bits & 0x03FF,
                    "h_flip": bool((entry_bits >> 10) & 0x0001),
                    "v_flip": bool((entry_bits >> 11) & 0x0001),
                    "pal_index": (entry_bits >> 12) & 0x000F,
                },
            }
        )

    return entries


def main():
    parser = argparse.ArgumentParser(
        description="Dump a metatiles.bin file as human-readable JSON."
    )
    parser.add_argument("path", help="Path to the metatiles.bin file")
    args = parser.parse_args()

    entries = parse_metatiles_bin(args.path)
    result = {"entries_count": len(entries), "entries": entries}
    json.dump(result, sys.stdout, indent=2)
    print()


if __name__ == "__main__":
    main()
