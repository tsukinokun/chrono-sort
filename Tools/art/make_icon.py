"""exe とウィンドウのアイコン（ChronoSort/Assets/Icon/App.ico）を描く。

    python Tools/art/make_icon.py

山札の裏と同じ羅針盤を、焦げ茶の角丸の四角に金で描く。
小さいサイズ（32px 以下）は細い線が潰れるので、線を太くし、細かい飾りを省いた絵を別に描く。
premake5.lua の tsukino_icon() が exe のリソースに埋め込む。
"""

import math
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "ChronoSort" / "Assets" / "Icon" / "App.ico"

S = 512    # 描くときの大きさ（最後に各サイズへ縮める）
SIZES = (16, 24, 32, 48, 64, 128, 256)

GOLD = (212, 170, 92)
GOLD_DIM = (150, 118, 64)
GOLD_LIGHT = (240, 210, 140)
DARK = (58, 38, 30)


def compass(draw, cx, cy, r, simple):
    """羅針盤を描く。simple なら針を 8 本にして輪を 1 本にする。"""
    rings = ((0.88, 22),) if simple else ((0.88, 10), (0.72, 6))
    for scale, width in rings:
        rr = r * scale
        draw.ellipse((cx - rr, cy - rr, cx + rr, cy + rr), outline=GOLD, width=width)
    count = 8 if simple else 16
    for i in range(count):
        a = i / count * math.tau - math.pi / 2
        major = i % (count // 4) == 0
        if simple:
            length = r if major else r * 0.62
            width = r * (0.26 if major else 0.18)
        else:
            length = r if major else (r * 0.7 if i % 2 == 0 else r * 0.55)
            width = r * (0.16 if major else 0.09)
        tip = (cx + math.cos(a) * length, cy + math.sin(a) * length)
        left = (cx + math.cos(a + math.pi / 2) * width / 2, cy + math.sin(a + math.pi / 2) * width / 2)
        right = (cx + math.cos(a - math.pi / 2) * width / 2, cy + math.sin(a - math.pi / 2) * width / 2)
        draw.polygon([left, tip, right], fill=GOLD if (major or not simple and i % 2 == 0) else GOLD_DIM)
    dot = r * (0.14 if simple else 0.1)
    draw.ellipse((cx - dot, cy - dot, cx + dot, cy + dot), fill=GOLD_LIGHT)


def draw_icon(simple):
    img = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    # 小さいときは余白を詰め、枠を太くして、背景から形がはっきり分かれるようにする
    inset = 8 if simple else 24
    d.rounded_rectangle((inset, inset, S - inset, S - inset), 96, fill=DARK, outline=GOLD, width=28 if simple else 12)
    if not simple:
        d.rounded_rectangle((52, 52, S - 52, S - 52), 72, outline=GOLD_DIM, width=4)
    compass(d, S / 2, S / 2, 200 if simple else 190, simple)
    return img


def main():
    detailed = draw_icon(simple=False)
    simple = draw_icon(simple=True)
    images = [(simple if size <= 32 else detailed).resize((size, size), Image.LANCZOS) for size in SIZES]
    OUT.parent.mkdir(parents=True, exist_ok=True)
    # 一番大きい絵を基に、残りのサイズは append_images の絵をそのまま使う
    images[-1].save(OUT, format="ICO", sizes=[(s, s) for s in SIZES], append_images=images[:-1])
    print(f"wrote {OUT} ({', '.join(str(s) for s in SIZES)})")


if __name__ == "__main__":
    main()
