"""デッキ JSON からカード面の画像（表・裏）と .tmat を焼き込む。

エンジンは 3D 空間に文字を描けないため、出来事名と年号を画像に焼き込んでおく。

    python Tools/art/bake_cards.py                全カードと卓・山札の素材を焼く
    python Tools/art/bake_cards.py --only a,b     指定したカードの面だけ焼く

入力:
    ChronoSort/Assets/Data/Decks/*.json
    Tools/art/raw/<id>.(png|jpg|jpeg)   … カードの絵（無ければ仮の絵柄を描く）
出力（ChronoSort/Assets/Textures/Cards/）:
    <id>_front.png / .tmat   … 表：絵と出来事名。年号は「？」で伏せる
    <id>_back.png  / .tmat   … 裏：絵と出来事名と年号（置いたあとに裏返して見せる面）
    table.png / table.tmat   … 卓の布
"""

import argparse
import json
import math
import random
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter, ImageFont, ImageOps

ROOT = Path(__file__).resolve().parents[2]
DECK_DIR = ROOT / "ChronoSort" / "Assets" / "Data" / "Decks"
RAW_DIR = ROOT / "Tools" / "art" / "raw"
OUT_DIR = ROOT / "ChronoSort" / "Assets" / "Textures" / "Cards"
TABLE_DIR = ROOT / "ChronoSort" / "Assets" / "Textures" / "Table"
BOARD_DIR = ROOT / "ChronoSort" / "Assets" / "Textures" / "Board"
# .tmat に書くパスはワークスペースルートからの相対
ASSET_PREFIX = "ChronoSort/Assets/Textures"
FONT_PATH = ROOT / "External" / "TsukinoEngine" / "Tsukino.BuiltIn" / "Assets" / "Fonts" / "NotoSansCJKjp-Regular.otf"
# 見出し（出来事名・年号）は明朝で。OS 同梱の Noto Serif JP（OFL）を画像に焼き込むだけなので配布物には含まれない
SERIF_PATH = Path("C:/Windows/Fonts/NotoSerifJP-VF.ttf")

W, H = 512, 768
SAVE_SIZE = (384, 576)
CORNER = 28
ART_BOX = (32, 32, W - 32, 500)
TITLE_BOX = (32, 512, W - 32, 640)
YEAR_BOX = (96, 646, W - 96, 722)

INK = (46, 32, 22)
PARCHMENT = (240, 228, 200)
GOLD = (196, 158, 82)
FRAME = (70, 46, 30)

# 仮の絵柄に使う地域ごとの色（上・下のグラデーション）
REGION_COLORS = {
    "JP": ((150, 48, 46), (52, 30, 44)),
    "WORLD": ((40, 92, 120), (26, 36, 60)),
}


def font(size):
    return ImageFont.truetype(str(FONT_PATH), size)


def serif(size, weight="SemiBold"):
    if not SERIF_PATH.exists():
        return font(size)
    f = ImageFont.truetype(str(SERIF_PATH), size)
    try:
        f.set_variation_by_name(weight)
    except Exception:
        pass
    return f


def fit_text(draw, text, box, max_size, min_size=20, face=None):
    """box に収まる最大の文字サイズを探し、必要なら2行に折り返す。"""
    face = face or serif
    width = box[2] - box[0]
    height = box[3] - box[1]
    for size in range(max_size, min_size - 1, -2):
        f = face(size)
        if draw.textlength(text, font=f) <= width:
            return [text], f
        # 2行に分けて収まるか（文字数で半分に割る。日本語は単語区切りが無いため）
        half = (len(text) + 1) // 2
        lines = [text[:half], text[half:]]
        if max(draw.textlength(l, font=f) for l in lines) <= width and size * 2.3 <= height:
            return lines, f
    return [text], face(min_size)


def draw_centered(draw, lines, f, box, fill):
    line_h = f.size * 1.2
    total = line_h * len(lines)
    y = (box[1] + box[3]) / 2 - total / 2
    for line in lines:
        draw.text(((box[0] + box[2]) / 2, y + line_h / 2), line, font=f, fill=fill, anchor="mm")
        y += line_h


def placeholder_art(card, size):
    """絵がまだ無いカード用に、地域色のグラデーションと大きな1文字の絵柄を作る。"""
    w, h = size
    top, bottom = REGION_COLORS.get(card["region"], REGION_COLORS["WORLD"])
    art = Image.new("RGB", size)
    px = art.load()
    for y in range(h):
        t = y / (h - 1)
        c = tuple(int(top[i] * (1 - t) + bottom[i] * t) for i in range(3))
        for x in range(w):
            px[x, y] = c
    # 出来事名の頭文字を薄く重ねる
    glyph = Image.new("L", size, 0)
    ImageDraw.Draw(glyph).text((w / 2, h / 2), card["title"][0], font=font(int(h * 0.55)), fill=70, anchor="mm")
    art.paste((255, 240, 220), mask=glyph)
    return art


def load_art(card, size):
    for ext in (".png", ".jpg", ".jpeg"):
        path = RAW_DIR / f"{card['id']}{ext}"
        if path.exists():
            art = ImageOps.fit(Image.open(path).convert("RGB"), size, Image.LANCZOS, centering=(0.5, 0.42))
            # 出どころの違う絵の色調を揃えるため、軽いセピアを掛ける
            sepia = ImageOps.colorize(ImageOps.grayscale(art), (40, 26, 16), (250, 236, 210))
            art = Image.blend(art, sepia, 0.38)
            # 縁を少し暗くして、古い図版のように見せる
            shade = Image.new("L", size, 0)
            ImageDraw.Draw(shade).ellipse((-size[0] * 0.25, -size[1] * 0.25, size[0] * 1.25, size[1] * 1.25), fill=255)
            shade = shade.filter(ImageFilter.GaussianBlur(40))
            return Image.composite(art, Image.new("RGB", size, (40, 26, 18)), shade.point(lambda v: 90 + v * 165 // 255))
    return placeholder_art(card, size)


def rounded_mask():
    mask = Image.new("L", (W, H), 0)
    ImageDraw.Draw(mask).rounded_rectangle((0, 0, W - 1, H - 1), CORNER, fill=255)
    return mask


def bake_face(card, art, reveal):
    img = Image.new("RGBA", (W, H), FRAME + (255,))
    draw = ImageDraw.Draw(img)

    # 羊皮紙の地と金の内枠
    draw.rounded_rectangle((12, 12, W - 13, H - 13), CORNER - 8, fill=PARCHMENT)
    draw.rounded_rectangle((20, 20, W - 21, H - 21), CORNER - 12, outline=GOLD, width=3)

    img.paste(art, ART_BOX[:2])
    draw.rectangle(ART_BOX, outline=FRAME, width=3)

    # 地域の札（絵の左上）。日本史は朱、世界史は藍
    tab_color = (168, 40, 36) if card["region"] == "JP" else (36, 70, 120)
    tab_text = "日本" if card["region"] == "JP" else "世界"
    draw.polygon([(ART_BOX[0] + 14, ART_BOX[1]), (ART_BOX[0] + 78, ART_BOX[1]), (ART_BOX[0] + 78, ART_BOX[1] + 70), (ART_BOX[0] + 46, ART_BOX[1] + 56), (ART_BOX[0] + 14, ART_BOX[1] + 70)], fill=tab_color)
    draw.text((ART_BOX[0] + 46, ART_BOX[1] + 26), tab_text, font=serif(22, "Bold"), fill=(250, 236, 210), anchor="mm")

    lines, f = fit_text(draw, card["title"], TITLE_BOX, 56)
    draw_centered(draw, lines, f, TITLE_BOX, INK)

    # 年号の札。表では伏せ、裏で明かす
    draw.rounded_rectangle(YEAR_BOX, 14, fill=FRAME if reveal else (205, 190, 160), outline=GOLD, width=3)
    label = card["yearLabel"] if reveal else "？？？？"
    lines, f = fit_text(draw, label, (YEAR_BOX[0] + 12, YEAR_BOX[1], YEAR_BOX[2] - 12, YEAR_BOX[3]), 50, face=lambda n: serif(n, "Bold"))
    draw_centered(draw, lines, f, YEAR_BOX, (250, 226, 160) if reveal else FRAME)

    draw.text((W / 2, H - 20), card["category"], font=font(18), fill=GOLD, anchor="md")

    img.putalpha(rounded_mask())
    return img


def write_tmat(path, texture, roughness=0.6):
    path.write_text(
        "# generated by Tools/art/bake_cards.py\n"
        "ShadingModel = PBR\n"
        f"Roughness    = {roughness}\n"
        "Specular     = 0.3\n"
        "AlphaCutoff  = 0.5\n"
        f"AlbedoMap    = {texture}\n",
        encoding="utf-8",
    )


def bake_table():
    """卓の布。古地図風の地にうっすら経緯線を引き、周辺を暗くする。"""
    tw, th = 2048, 1280
    rng = random.Random(1)
    base = Image.new("RGB", (tw, th), (92, 66, 44))
    noise = Image.effect_noise((tw // 4, th // 4), 28).resize((tw, th), Image.BICUBIC)
    base = Image.composite(Image.new("RGB", (tw, th), (120, 90, 60)), base, noise)
    draw = ImageDraw.Draw(base)
    for x in range(0, tw, 128):
        draw.line((x, 0, x, th), fill=(110, 82, 56), width=2)
    for y in range(0, th, 128):
        draw.line((0, y, tw, y), fill=(110, 82, 56), width=2)
    for _ in range(40):
        cx, cy, r = rng.randrange(tw), rng.randrange(th), rng.randrange(40, 160)
        draw.ellipse((cx - r, cy - r, cx + r, cy + r), outline=(104, 76, 50), width=2)
    vignette = Image.new("L", (tw, th), 0)
    ImageDraw.Draw(vignette).ellipse((-tw * 0.2, -th * 0.3, tw * 1.2, th * 1.3), fill=255)
    vignette = vignette.filter(ImageFilter.GaussianBlur(160))
    base = Image.composite(base, Image.new("RGB", (tw, th), (30, 20, 14)), vignette)

    TABLE_DIR.mkdir(parents=True, exist_ok=True)
    base.save(TABLE_DIR / "table.png")
    write_tmat(TABLE_DIR / "table.tmat", f"{ASSET_PREFIX}/Table/table.png", roughness=0.9)


def bake_board():
    """山札の裏模様と、挿入位置の目印（枠だけのカード）。"""
    BOARD_DIR.mkdir(parents=True, exist_ok=True)

    # 裏模様：濃い革に金の枠と羅針盤
    cover = Image.new("RGBA", (W, H), FRAME + (255,))
    draw = ImageDraw.Draw(cover)
    draw.rounded_rectangle((12, 12, W - 13, H - 13), CORNER - 8, fill=(58, 38, 30))
    draw.rounded_rectangle((26, 26, W - 27, H - 27), CORNER - 14, outline=GOLD, width=4)
    draw.rounded_rectangle((40, 40, W - 41, H - 41), CORNER - 18, outline=(150, 118, 64), width=2)
    cx, cy = W / 2, H / 2
    for r in (150, 120):
        draw.ellipse((cx - r, cy - r, cx + r, cy + r), outline=GOLD, width=3)
    for i in range(16):
        a = i / 16 * math.tau
        length = 170 if i % 4 == 0 else (120 if i % 2 == 0 else 95)
        width = 22 if i % 4 == 0 else 12
        tip = (cx + math.cos(a) * length, cy + math.sin(a) * length)
        left = (cx + math.cos(a + math.pi / 2) * width / 2, cy + math.sin(a + math.pi / 2) * width / 2)
        right = (cx + math.cos(a - math.pi / 2) * width / 2, cy + math.sin(a - math.pi / 2) * width / 2)
        draw.polygon([left, tip, right], fill=GOLD if i % 2 == 0 else (150, 118, 64))
    draw.ellipse((cx - 14, cy - 14, cx + 14, cy + 14), fill=(232, 200, 120))
    draw.text((cx, H - 90), "ChronoSort", font=font(40), fill=GOLD, anchor="mm")
    cover.putalpha(rounded_mask())
    cover.save(BOARD_DIR / "cover.png")
    write_tmat(BOARD_DIR / "cover.tmat", f"{ASSET_PREFIX}/Board/cover.png", roughness=0.45)

    # 目印：角丸の枠線だけ（中は抜く）
    for name, color in (("marker", (255, 214, 120)), ("marker_warning", (235, 70, 55))):
        marker = Image.new("RGBA", (W, H), (0, 0, 0, 0))
        mdraw = ImageDraw.Draw(marker)
        mdraw.rounded_rectangle((6, 6, W - 7, H - 7), CORNER, outline=color + (255,), width=18)
        mdraw.rounded_rectangle((44, 44, W - 45, H - 45), CORNER - 16, outline=color + (255,), width=6)
        marker.save(BOARD_DIR / f"{name}.png")
        write_tmat(BOARD_DIR / f"{name}.tmat", f"{ASSET_PREFIX}/Board/{name}.png", roughness=0.4)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--only", default="")
    args = parser.parse_args()
    # 指定したカードの面だけを焼く（卓・山札の素材と、他のカードの面には触れない）
    only = {s for s in args.only.split(",") if s}

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    if not only:
        bake_board()
    art_size = (ART_BOX[2] - ART_BOX[0], ART_BOX[3] - ART_BOX[1])
    count = 0
    for deck_path in sorted(DECK_DIR.glob("*.json")):
        deck = json.loads(deck_path.read_text(encoding="utf-8"))
        for card in deck["cards"]:
            if only and card["id"] not in only:
                continue
            art = load_art(card, art_size)
            for face, reveal in (("front", False), ("back", True)):
                name = f"{card['id']}_{face}"
                # 描くのは 512x768 で、保存は 3/4 に縮めて 256 色に減色する。
                # セピア寄りの絵は減色してもほぼ見分けがつかず、容量は 1/4 ほどになる（リポジトリが重くならないように）
                face = bake_face(card, art, reveal).resize(SAVE_SIZE, Image.LANCZOS)
                face = face.quantize(256, method=Image.Quantize.FASTOCTREE, dither=Image.Dither.FLOYDSTEINBERG)
                face.save(OUT_DIR / f"{name}.png", optimize=True)
                write_tmat(OUT_DIR / f"{name}.tmat", f"{ASSET_PREFIX}/Cards/{name}.png")
            count += 1
    if not only:
        bake_table()
    print(f"baked {count} cards to {OUT_DIR}")


if __name__ == "__main__":
    main()
