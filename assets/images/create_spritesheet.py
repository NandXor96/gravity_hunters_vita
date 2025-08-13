#!/usr/bin/env python3
import sys
import os
from PIL import Image

def create_spritesheet(folder, tile_width, tile_height, output="spritesheet.png"):
    png_files = [f for f in os.listdir(folder) if f.lower().endswith(".png")]
    png_files.sort()  # Optionale Sortierung nach Dateiname

    if not png_files:
        print("Keine PNG-Dateien gefunden.")
        return

    cols = int(len(png_files) ** 0.5 + 0.999)  # fast quadratisches Sheet
    rows = (len(png_files) + cols - 1) // cols

    sheet_width = cols * tile_width
    sheet_height = rows * tile_height
    sheet = Image.new("RGBA", (sheet_width, sheet_height), (0, 0, 0, 0))

    for idx, filename in enumerate(png_files):
        path = os.path.join(folder, filename)
        img = Image.open(path).convert("RGBA")

        # proportional skalieren, falls größer als Tile
        scale = min(tile_width / img.width, tile_height / img.height, 1.0)
        if scale < 1.0:
            new_size = (int(img.width * scale), int(img.height * scale))
            img = img.resize(new_size, Image.LANCZOS)

        # Position auf dem Sheet berechnen
        col = idx % cols
        row = idx // cols
        x = col * tile_width + (tile_width - img.width) // 2
        y = row * tile_height + (tile_height - img.height) // 2

        sheet.paste(img, (x, y), img)

    sheet.save(output)
    print(f"Spritesheet gespeichert als: {output}")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print(f"Verwendung: {sys.argv[0]} <Ordner> <TileBreite>x<TileHöhe> [Ausgabedatei]")
        sys.exit(1)

    folder = sys.argv[1]
    try:
        tile_w, tile_h = map(int, sys.argv[2].lower().split("x"))
    except ValueError:
        print("Fehler: Tilegröße muss im Format 50x50 angegeben werden.")
        sys.exit(1)

    output_file = sys.argv[3] if len(sys.argv) > 3 else "spritesheet.png"
    create_spritesheet(folder, tile_w, tile_h, output_file)
