#!/usr/bin/env python3

import os
import re
import json
import argparse
import numpy as np
from PIL import Image
import cv2

def ensure_dir(path):
    os.makedirs(path, exist_ok=True)

def parse_grid_arg(grid_str):
    m = re.match(r"^\s*(\d+)\s*x\s*(\d+)\s*$", grid_str, flags=re.I)
    if not m:
        raise argparse.ArgumentTypeError("grid muss wie '3x3' angegeben werden.")
    r, c = int(m.group(1)), int(m.group(2))
    if r <= 0 or c <= 0: raise argparse.ArgumentTypeError("rows/cols müssen > 0 sein.")
    return r, c

def load_rgba(path):
    img = Image.open(path).convert("RGBA")
    return img

def pil_to_cv(img_rgba):
    # PIL RGBA -> OpenCV BGRA
    arr = np.array(img_rgba)
    return cv2.cvtColor(arr, cv2.COLOR_RGBA2BGRA)

def cv_to_pil(img_bgra):
    # OpenCV BGRA -> PIL RGBA
    arr = cv2.cvtColor(img_bgra, cv2.COLOR_BGRA2RGBA)
    return Image.fromarray(arr)

def extract_mask_from_alpha(img_rgba, alpha_thresh=1):
    alpha = np.array(img_rgba.split()[-1])  # Alpha-Kanal
    mask = (alpha >= alpha_thresh).astype(np.uint8) * 255
    return mask

def largest_external_contour(mask):
    contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_NONE)
    if not contours:
        return None
    return max(contours, key=cv2.contourArea)

def simplify_contour_to_max_points(contour, max_points=20):
    if contour is None or len(contour) == 0:
        return None
    peri = cv2.arcLength(contour, True)
    if peri <= 0:
        x,y,w,h = cv2.boundingRect(contour)
        pts = np.array([[x,y],[x+w,y],[x+w,y+h],[x,y+h]], dtype=np.int32)
        return pts
    eps = max(1e-6, 0.01 * peri)  # Start: 1% vom Umfang
    approx = cv2.approxPolyDP(contour, eps, True)
    it = 0
    while len(approx) > max_points and it < 60:
        eps *= 1.25
        approx = cv2.approxPolyDP(contour, eps, True)
        it += 1
    if len(approx) > max_points:
        idx = np.linspace(0, len(approx)-1, max_points, dtype=int)
        approx = approx[idx]
    return approx.reshape(-1, 2).astype(np.int32)

def draw_polygon_overlay(img_rgba, polygon_pts, thickness=2):
    cv = pil_to_cv(img_rgba)   # BGRA
    if polygon_pts is not None and len(polygon_pts) >= 2:
        pts = polygon_pts.reshape((-1,1,2))
        cv2.polylines(cv, [pts], isClosed=True, color=(0,255,0,255), thickness=thickness, lineType=cv2.LINE_AA)
    return cv_to_pil(cv)

def crop_grid_cells(img_rgba, rows, cols):
    W, H = img_rgba.size
    if W % cols != 0 or H % rows != 0:
        raise ValueError(f"Bildgröße {W}x{H} nicht durch Grid {rows}x{cols} teilbar.")
    cw, ch = W // cols, H // rows
    cells = []
    boxes = []
    for r in range(rows):
        for c in range(cols):
            box = (c*cw, r*ch, (c+1)*cw, (r+1)*ch)
            crop = img_rgba.crop(box)
            cells.append(crop)
            boxes.append(box)
    return cells, boxes, (cw, ch)

def save_json(path, data):
    with open(path, "w", encoding="utf-8") as f:
        json.dump(data, f, ensure_ascii=False, indent=2)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--atlas", required=True, help="Pfad zur Atlas-PNG (RGBA empfohlen)")
    ap.add_argument("--grid", type=str, help="z.B. '3x3' für rows x cols")
    ap.add_argument("--rows", type=int, default=None)
    ap.add_argument("--cols", type=int, default=None)
    ap.add_argument("--max_points", type=int, default=20)
    ap.add_argument("--alpha_thresh", type=int, default=1, help="Alpha-Schwelle [1..255]")
    ap.add_argument("--outdir", default="polygon")
    ap.add_argument("--start_index", type=int, default=1, help="Start-Index der Ausgabedateien")
    args = ap.parse_args()

    # Grid aus --grid oder --rows/--cols lesen
    if args.grid:
        r, c = parse_grid_arg(args.grid)
        rows, cols = r, c
    else:
        if args.rows is None or args.cols is None:
            ap.error("Bitte --grid 3x3 ODER --rows und --cols angeben.")
        rows, cols = args.rows, args.cols

    ensure_dir(args.outdir)

    atlas = load_rgba(args.atlas)
    cells, boxes, (cw, ch) = crop_grid_cells(atlas, rows, cols)
    print(f"Atlas: {atlas.size[0]}x{atlas.size[1]} px, Grid: {rows}x{cols}, Cell: {cw}x{ch}")

    all_polys = {}
    for i, cell in enumerate(cells):
        idx = args.start_index + i

        # Maske
        mask = extract_mask_from_alpha(cell, alpha_thresh=args.alpha_thresh)

        # größte Außenkontur
        cnt = largest_external_contour(mask)

        # vereinfachen
        poly = simplify_contour_to_max_points(cnt, max_points=args.max_points)
        poly_list = [] if (poly is None or len(poly) == 0) else [[int(x), int(y)] for x, y in poly]

        # speichern
        save_json(os.path.join(args.outdir, f"{idx}.json"), {"index": idx, "points": poly_list})

        overlay = draw_polygon_overlay(cell, poly)
        overlay.save(os.path.join(args.outdir, f"{idx}.png"))

        # optional: den Crop auch speichern
        cell.save(os.path.join(args.outdir, f"{idx}_texture.png"))

        all_polys[str(idx)] = poly_list

    save_json(os.path.join(args.outdir, "polygons.json"), all_polys)

    print(f"Fertig. Dateien in: {os.path.abspath(args.outdir)}")
    print("Pro Zelle: <n>.png (Overlay), <n>_texture.png (Crop), <n>.json (Punkte). Gesamt: polygons.json")

if __name__ == "__main__":
    main()
