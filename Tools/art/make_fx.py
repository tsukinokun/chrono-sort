"""演出用の粒子テクスチャ（64x64、白基調でアルファに形を持たせる）を生成する。

    python Tools/art/make_fx.py

色はゲーム側の tintColor で付けるため、ここでは白と形だけを作る。
"""

import math
import random
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw, ImageFilter

ROOT = Path(__file__).resolve().parents[2]
OUT_DIR = ROOT / "ChronoSort" / "Assets" / "Textures" / "Fx"
SIZE = 64


def save_alpha(alpha, name):
    img = Image.new("RGBA", (SIZE, SIZE), (255, 255, 255, 0))
    img.putalpha(alpha)
    img.save(OUT_DIR / name)


def soft():
    """中心が濃く、縁へなだらかに消える丸。砂埃や光の粒に使う。"""
    alpha = Image.new("L", (SIZE, SIZE), 0)
    px = alpha.load()
    c = (SIZE - 1) / 2
    for y in range(SIZE):
        for x in range(SIZE):
            d = math.hypot(x - c, y - c) / c
            px[x, y] = int(255 * max(0.0, 1.0 - d) ** 2)
    save_alpha(alpha, "soft.png")


def spark():
    """細い十字と芯の光。火花に使う。"""
    alpha = Image.new("L", (SIZE, SIZE), 0)
    draw = ImageDraw.Draw(alpha)
    c = SIZE // 2
    draw.polygon([(c, 2), (c + 3, c), (c, SIZE - 2), (c - 3, c)], fill=255)
    draw.polygon([(2, c), (c, c - 3), (SIZE - 2, c), (c, c + 3)], fill=255)
    glow = Image.new("L", (SIZE, SIZE), 0)
    ImageDraw.Draw(glow).ellipse((c - 10, c - 10, c + 10, c + 10), fill=255)
    glow = glow.filter(ImageFilter.GaussianBlur(5))
    combined = ImageChops.lighter(alpha, glow)
    save_alpha(combined.filter(ImageFilter.GaussianBlur(0.6)), "spark.png")


def ink():
    """縁がいびつな墨の滴。"""
    rng = random.Random(3)
    alpha = Image.new("L", (SIZE, SIZE), 0)
    draw = ImageDraw.Draw(alpha)
    c = SIZE / 2
    points = []
    for i in range(18):
        a = i / 18 * math.tau
        r = 18 + rng.uniform(-5, 6)
        points.append((c + math.cos(a) * r, c + math.sin(a) * r))
    draw.polygon(points, fill=255)
    for _ in range(5):
        a = rng.uniform(0, math.tau)
        r = rng.uniform(20, 27)
        s = rng.uniform(2, 4)
        x, y = c + math.cos(a) * r, c + math.sin(a) * r
        draw.ellipse((x - s, y - s, x + s, y + s), fill=255)
    save_alpha(alpha.filter(ImageFilter.GaussianBlur(1.0)), "ink.png")


def confetti():
    """少し傾いた細長い紙片。"""
    alpha = Image.new("L", (SIZE, SIZE), 0)
    draw = ImageDraw.Draw(alpha)
    c = SIZE / 2
    w, h, a = 10, 22, math.radians(25)
    corners = [(-w, -h), (w, -h), (w, h), (-w, h)]
    rotated = [(c + x * math.cos(a) - y * math.sin(a), c + x * math.sin(a) + y * math.cos(a)) for x, y in corners]
    draw.polygon(rotated, fill=255)
    save_alpha(alpha.filter(ImageFilter.GaussianBlur(0.5)), "confetti.png")


def main():
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    soft()
    spark()
    ink()
    confetti()
    print(f"wrote fx textures to {OUT_DIR}")


if __name__ == "__main__":
    main()
