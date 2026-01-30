#!/usr/bin/env python3
"""
Script to set a specific pixel in a PNG file with RGBA values.

Usage:
    uv run --with pillow Scripts/set_pixel.py <image_path> <row> <col> <r> <g> <b> <a>

Example:
    uv run --with pillow Scripts/set_pixel.py image.png 10 20 255 0 0 255
"""

import argparse
from PIL import Image

def set_pixel(image_path, row, col, rgba):
    """
    Set a specific pixel in a PNG image.

    Args:
        image_path: Path to the PNG file
        row: Row index (0-based)
        col: Column index (0-based)
        rgba: Tuple of (R, G, B, A) values (0-255)
    """
    # Open the image
    img = Image.open(image_path)

    # Convert to RGBA if not already
    if img.mode != 'RGBA':
        img = img.convert('RGBA')

    # Load pixel data
    pixels = img.load()

    # Set the pixel (PIL uses x,y coordinates where x=col, y=row)
    pixels[col, row] = rgba

    # Save the image
    img.save(image_path)
    print(f"Set pixel at row {row}, col {col} to RGBA{rgba}")
    print(f"Saved to {image_path}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Set a specific pixel in a PNG file with RGBA values.')
    parser.add_argument('image_path', help='Path to the PNG file')
    parser.add_argument('row', type=int, help='Row index (0-based)')
    parser.add_argument('col', type=int, help='Column index (0-based)')
    parser.add_argument('r', type=int, help='Red value (0-255)')
    parser.add_argument('g', type=int, help='Green value (0-255)')
    parser.add_argument('b', type=int, help='Blue value (0-255)')
    parser.add_argument('a', type=int, help='Alpha value (0-255)')

    args = parser.parse_args()

    # Validate RGBA values
    for value, name in [(args.r, 'Red'), (args.g, 'Green'), (args.b, 'Blue'), (args.a, 'Alpha')]:
        if not 0 <= value <= 255:
            parser.error(f"{name} value must be between 0 and 255")

    rgba = (args.r, args.g, args.b, args.a)
    set_pixel(args.image_path, args.row, args.col, rgba)
