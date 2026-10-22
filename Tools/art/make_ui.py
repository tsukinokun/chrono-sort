"""画面 UI の素材（古地図・研究者の机）を生成する。

    python Tools/art/make_ui.py

破れた紙片・封蝋・インク壺・しおり・朱印・ピン・テープなど。
どれも「机の上に置かれた物」に見えるよう、薄い影と紙の質感を焼き込む。
文字が入る素材は、ゲーム側で FontComponent を重ねる前提で無地にしておく（朱印だけは字を焼く）。
"""

import math
import random
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageFont

ROOT = Path(__file__).resolve().parents[2]
OUT_DIR = ROOT / "ChronoSort" / "Assets" / "Textures" / "UI"
SERIF = Path("C:/Windows/Fonts/NotoSerifJP-VF.ttf")
SANS = ROOT / "External" / "TsukinoEngine" / "Tsukino.BuiltIn" / "Assets" / "Fonts" / "NotoSansCJKjp-Regular.otf"

PAPER = (236, 224, 196)
PAPER_DARK = (205, 186, 150)
INK = (46, 32, 22)
WAX = (150, 28, 32)
GOLD = (196, 158, 82)


def serif(size, weight="SemiBold"):
    if SERIF.exists():
        f = ImageFont.truetype(str(SERIF), size)
        try:
            f.set_variation_by_name(weight)
        except Exception:
            pass
        return f
    return ImageFont.truetype(str(SANS), size)


def noise(size, scale, amount, seed):
    rng = random.Random(seed)
    small = Image.new("L", (max(1, size[0] // scale), max(1, size[1] // scale)))
    small.putdata([128 + int(rng.uniform(-amount, amount)) for _ in range(small.width * small.height)])
    return small.resize(size, Image.BICUBIC)


def torn_mask(size, margin, roughness, seed):
    """縁がぎざぎざに破れた紙の形（アルファ）。"""
    rng = random.Random(seed)
    w, h = size
    pts = []
    step = 6
    for x in range(margin, w - margin, step):
        pts.append((x, margin + rng.uniform(-roughness, roughness)))
    for y in range(margin, h - margin, step):
        pts.append((w - margin + rng.uniform(-roughness, roughness), y))
    for x in range(w - margin, margin, -step):
        pts.append((x, h - margin + rng.uniform(-roughness, roughness)))
    for y in range(h - margin, margin, -step):
        pts.append((margin + rng.uniform(-roughness, roughness), y))
    mask = Image.new("L", size, 0)
    ImageDraw.Draw(mask).polygon(pts, fill=255)
    return mask.filter(ImageFilter.GaussianBlur(0.8))


def paper_fill(size, seed, base=PAPER, stains=3):
    """紙の地：色むら・繊維・染み。"""
    w, h = size
    img = Image.new("RGB", size, base)
    grain = noise(size, 3, 22, seed)
    blotch = noise(size, 40, 40, seed + 1)
    img = ImageChops.multiply(img, Image.merge("RGB", [Image.eval(grain, lambda v: 200 + v // 4)] * 3))
    img = ImageChops.multiply(img, Image.merge("RGB", [Image.eval(blotch, lambda v: 215 + v // 6)] * 3))
    rng = random.Random(seed + 2)
    draw = ImageDraw.Draw(img, "RGBA")
    for _ in range(stains):
        cx, cy, r = rng.uniform(0, w), rng.uniform(0, h), rng.uniform(min(w, h) * 0.1, min(w, h) * 0.35)
        draw.ellipse((cx - r, cy - r, cx + r, cy + r), outline=(150, 115, 70, 40), width=3)
        draw.ellipse((cx - r * 0.9, cy - r * 0.9, cx + r * 0.9, cy + r * 0.9), fill=(170, 135, 90, 14))
    return img


def with_shadow(img, mask, offset=(6, 8), blur=8, opacity=110):
    """机に置いた影を焼き込む。"""
    w, h = img.size
    pad = blur * 3
    canvas = Image.new("RGBA", (w + pad * 2, h + pad * 2), (0, 0, 0, 0))
    shadow = Image.new("L", canvas.size, 0)
    shadow.paste(mask, (pad + offset[0], pad + offset[1]))
    shadow = shadow.filter(ImageFilter.GaussianBlur(blur)).point(lambda v: v * opacity // 255)
    canvas.paste((20, 12, 6, 255), mask=shadow)
    piece = img.convert("RGBA")
    piece.putalpha(mask)
    canvas.alpha_composite(piece, (pad, pad))
    return canvas


def darken_edges(img, mask, amount=60):
    """紙の縁を少し焼けた色にする。"""
    edge = ImageChops.subtract(mask, mask.filter(ImageFilter.MinFilter(9))).filter(ImageFilter.GaussianBlur(4))
    burnt = Image.new("RGB", img.size, (120, 85, 50))
    return Image.composite(burnt, img, edge.point(lambda v: min(255, v * amount // 100)))


def make_scrap(name, size, seed, lines=False):
    img = paper_fill(size, seed)
    mask = torn_mask(size, 6, 4, seed)
    if lines:
        draw = ImageDraw.Draw(img)
        for y in range(size[1] // 3, size[1] - 10, 26):
            draw.line((14, y, size[0] - 14, y), fill=(150, 170, 190), width=1)
        draw.line((44, 8, 44, size[1] - 8), fill=(200, 120, 110), width=1)
    img = darken_edges(img, mask)
    with_shadow(img, mask).save(OUT_DIR / f"{name}.png")


def make_wax_seal():
    s = 220
    rng = random.Random(11)
    mask = Image.new("L", (s, s), 0)
    draw = ImageDraw.Draw(mask)
    c = s / 2
    pts = []
    for i in range(48):
        a = i / 48 * math.tau
        r = 92 + rng.uniform(-7, 9)
        pts.append((c + math.cos(a) * r, c + math.sin(a) * r))
    draw.polygon(pts, fill=255)
    mask = mask.filter(ImageFilter.GaussianBlur(1.5))
    img = Image.new("RGB", (s, s), WAX)
    shade = Image.new("L", (s, s), 0)
    ImageDraw.Draw(shade).ellipse((20, 14, s - 34, s - 40), fill=255)
    shade = shade.filter(ImageFilter.GaussianBlur(25))
    img = Image.composite(Image.new("RGB", (s, s), (200, 60, 58)), img, shade.point(lambda v: v // 2))
    d = ImageDraw.Draw(img)
    d.ellipse((c - 66, c - 66, c + 66, c + 66), outline=(110, 16, 20), width=6)
    d.ellipse((c - 62, c - 62, c + 62, c + 62), outline=(196, 80, 74), width=2)
    with_shadow(img, mask, offset=(4, 6), blur=6).save(OUT_DIR / "wax_seal.png")


def make_ink_bottle(name, empty=False):
    w, h = 110, 140
    img = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    glass = (40, 48, 70, 235) if not empty else (120, 120, 130, 110)
    d.rounded_rectangle((14, 46, w - 14, h - 10), 22, fill=glass, outline=(20, 20, 30, 255), width=3)
    d.rectangle((38, 22, w - 38, 50), fill=glass, outline=(20, 20, 30, 255), width=3)
    d.rectangle((34, 12, w - 34, 26), fill=(110, 70, 40, 255), outline=(50, 30, 15, 255), width=2)
    if not empty:
        d.rounded_rectangle((18, 78, w - 18, h - 14), 18, fill=(12, 14, 32, 245))
        d.ellipse((26, 58, 42, 92), fill=(255, 255, 255, 70))
    label = (200, 180, 140, 255) if not empty else (170, 160, 140, 140)
    d.rectangle((26, 90, w - 26, 116), fill=label)
    d.line((32, 103, w - 32, 103), fill=(90, 60, 40, 255), width=2)
    mask = img.getchannel("A")
    with_shadow(img.convert("RGB"), mask, offset=(4, 5), blur=5).save(OUT_DIR / f"{name}.png")


def make_ribbon():
    w, h = 96, 320
    img = Image.new("RGB", (w, h), (140, 30, 36))
    d = ImageDraw.Draw(img)
    for x in range(0, w, 4):
        d.line((x, 0, x, h), fill=(150 + (x % 8) * 2, 34, 40), width=2)
    d.line((8, 0, 8, h), fill=(205, 165, 90), width=3)
    d.line((w - 9, 0, w - 9, h), fill=(205, 165, 90), width=3)
    mask = Image.new("L", (w, h), 0)
    ImageDraw.Draw(mask).polygon([(0, 0), (w, 0), (w, h), (w // 2, h - 40), (0, h)], fill=255)
    with_shadow(img, mask, offset=(5, 4), blur=6).save(OUT_DIR / "ribbon.png")


def make_stamp(name, text, color):
    s = 256
    rng = random.Random(hash(name) & 0xFFFF)
    alpha = Image.new("L", (s, s), 0)
    d = ImageDraw.Draw(alpha)
    c = s / 2
    d.ellipse((14, 14, s - 14, s - 14), outline=255, width=14)
    d.ellipse((34, 34, s - 34, s - 34), outline=255, width=4)
    d.text((c, c + 4), text, font=serif(140, "Black"), fill=255, anchor="mm")
    # かすれ：ところどころインクが乗っていない
    grit = noise((s, s), 2, 120, 5).point(lambda v: 255 if v > 70 else 0)
    alpha = ImageChops.multiply(alpha, grit.filter(ImageFilter.GaussianBlur(0.6)))
    img = Image.new("RGBA", (s, s), color + (0,))
    img.putalpha(alpha)
    img = img.rotate(rng.uniform(-8, 8), resample=Image.BICUBIC)
    img.save(OUT_DIR / f"{name}.png")


def make_pin():
    s = 56
    img = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    d.ellipse((14, 10, 42, 38), fill=(180, 40, 36, 255), outline=(90, 15, 15, 255), width=2)
    d.ellipse((20, 15, 28, 23), fill=(255, 190, 180, 200))
    mask = img.getchannel("A")
    with_shadow(img.convert("RGB"), mask, offset=(5, 7), blur=3, opacity=140).save(OUT_DIR / "pin.png")


def make_tape():
    w, h = 150, 46
    img = Image.new("RGBA", (w, h), (232, 222, 180, 150))
    mask = torn_mask((w, h), 2, 2.5, 21).point(lambda v: v * 150 // 255)
    img.putalpha(mask)
    img.save(OUT_DIR / "tape.png")


def make_portrait_frame():
    """CPU の名札の台紙（肖像は別素材を重ねる）。"""
    w, h = 170, 210
    img = paper_fill((w, h), 31, base=(222, 206, 172))
    d = ImageDraw.Draw(img)
    d.rectangle((14, 14, w - 14, h - 52), fill=(70, 52, 40), outline=GOLD, width=3)
    mask = torn_mask((w, h), 4, 2.5, 31)
    with_shadow(darken_edges(img, mask), mask).save(OUT_DIR / "portrait_frame.png")


def make_silhouette():
    """肖像画が無いときの横顔の影絵。"""
    w, h = 142, 144
    img = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    d.ellipse((38, 18, 104, 92), fill=(200, 175, 130, 255))
    d.polygon([(20, h), (36, 104), (71, 90), (106, 104), (122, h)], fill=(200, 175, 130, 255))
    img.save(OUT_DIR / "silhouette.png")


def make_glow():
    """ホバー時に紙の下に敷く柔らかい光。"""
    s = 128
    alpha = Image.new("L", (s, s), 0)
    ImageDraw.Draw(alpha).ellipse((16, 16, s - 16, s - 16), fill=255)
    alpha = alpha.filter(ImageFilter.GaussianBlur(18))
    img = Image.new("RGBA", (s, s), (255, 220, 150, 0))
    img.putalpha(alpha)
    img.save(OUT_DIR / "glow.png")


def make_vignette():
    """画面の四隅を暗くする覆い（UI の下、3D の上に敷く）。"""
    w, h = 800, 450
    alpha = Image.new("L", (w, h), 255)
    ImageDraw.Draw(alpha).ellipse((-w * 0.15, -h * 0.25, w * 1.15, h * 1.25), fill=0)
    alpha = alpha.filter(ImageFilter.GaussianBlur(90)).point(lambda v: v * 150 // 255)
    img = Image.new("RGBA", (w, h), (12, 6, 2, 0))
    img.putalpha(alpha)
    img.save(OUT_DIR / "vignette.png")


def make_title_logo():
    """タイトルの題字（羊皮紙の帯に墨の字）。"""
    w, h = 900, 260
    img = paper_fill((w, h), 41, stains=4)
    d = ImageDraw.Draw(img)
    d.text((w / 2, h / 2 - 18), "ChronoSort", font=serif(130, "Bold"), fill=INK, anchor="mm")
    d.text((w / 2, h - 46), "歴 史 を 並 べ る 卓", font=serif(34, "Medium"), fill=(110, 70, 40), anchor="mm")
    mask = torn_mask((w, h), 8, 6, 41)
    with_shadow(darken_edges(img, mask, 80), mask, offset=(8, 12), blur=12).save(OUT_DIR / "title_logo.png")


def make_ink_line():
    """読込画面の進み具合を示す、筆で引いた墨の線（横に伸ばして使う）。"""
    w, h = 512, 28
    rng = random.Random(51)
    alpha = Image.new("L", (w, h), 0)
    d = ImageDraw.Draw(alpha)
    for x in range(w):
        # 筆の入りと抜きで細くし、途中は少し太さを揺らす
        t = x / (w - 1)
        thick = (6 + 4 * math.sin(t * math.pi)) * (0.85 + 0.15 * rng.random())
        d.line((x, h / 2 - thick, x, h / 2 + thick), fill=230)
    alpha = alpha.filter(ImageFilter.GaussianBlur(1.2))
    img = Image.new("RGBA", (w, h), INK + (0,))
    img.putalpha(alpha)
    img.save(OUT_DIR / "ink_line.png")


def main():
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    make_ink_line()
    make_scrap("scrap_wide", (520, 150), 1)
    make_scrap("scrap_note", (360, 120), 2, lines=True)
    make_scrap("scrap_square", (300, 260), 3)
    make_scrap("scrap_tall", (260, 360), 4)
    make_scrap("scrap_result", (760, 600), 5)
    make_wax_seal()
    make_ink_bottle("ink_full")
    make_ink_bottle("ink_empty", empty=True)
    make_ribbon()
    make_stamp("stamp_correct", "正", (196, 30, 32))
    make_stamp("stamp_wrong", "誤", (58, 34, 72))
    make_pin()
    make_tape()
    make_portrait_frame()
    make_silhouette()
    make_glow()
    make_vignette()
    make_title_logo()
    print(f"wrote ui textures to {OUT_DIR}")


if __name__ == "__main__":
    main()
