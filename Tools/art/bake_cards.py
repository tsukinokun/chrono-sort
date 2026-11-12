"""デッキ JSON からカード面の画像（表・裏）と .tmat を焼き込む。

エンジンは 3D 空間に文字を描けないため、出来事名と年号を画像に焼き込んでおく。

    python Tools/art/bake_cards.py                全カードと卓・山札の素材を焼く
    python Tools/art/bake_cards.py --only a,b     指定したカードの面だけ焼く
    python Tools/art/bake_cards.py --retitle [--only a,b]
                                                  焼いた面の出来事名の欄だけを描き直す
                                                  （raw/ の絵が手元に無くても、今の絵を残したまま題名を直せる）
    python Tools/art/bake_cards.py --lang en [--only a,b]
                                                  日本語の面の文字の欄を英語で描き直し、Cards/en/ に置く
                                                  （訳は Data/Text/en.json と Data/Text/Cards/en.json）

入力:
    ChronoSort/Assets/Data/Decks/*.json
    Tools/art/raw/<id>.(png|jpg|jpeg)   … カードの絵（無ければ仮の絵柄を描く）
出力（ChronoSort/Assets/Textures/Cards/）:
    <id>_front.png / .tmat   … 表：絵と出来事名。年号は「？」で伏せる
    <id>_back.png  / .tmat   … 裏：絵と出来事名と年号（置いたあとに裏返して見せる面）
    table.png / table.tmat   … 卓の布
"""

import argparse
import functools
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
# Noto Serif JP に字形の無い字（「岸」など）だけを描く、OS 同梱の明朝
FALLBACK_SERIF_PATH = Path("C:/Windows/Fonts/yumindb.ttf")

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


LINE_HEIGHT = 1.2      # 行の高さ（文字の大きさの倍率）
TWO_LINE_MARGIN = 12   # 2行に折り返したとき、欄の上下に残す余白の合計（絵と年号の札に詰まって見えないように）


@functools.lru_cache(maxsize=None)
def font(size):
    return ImageFont.truetype(str(FONT_PATH), size)


@functools.lru_cache(maxsize=None)
def fallback(size):
    if FALLBACK_SERIF_PATH.exists():
        return ImageFont.truetype(str(FALLBACK_SERIF_PATH), size)
    return font(size)


@functools.lru_cache(maxsize=None)
def notdef_mask(f):
    # 字形の無い字は .notdef（豆腐）で描かれる。私用面の字で、その形を覚えておく
    return bytes(f.getmask("\U000F0000"))


@functools.lru_cache(maxsize=None)
def has_glyph(f, c):
    return c.isspace() or bytes(f.getmask(c)) != notdef_mask(f)


def runs(text, f):
    """字形の有無で text を区切り、(文字列, フォント) の並びにする。無い字は代わりの明朝で描く。"""
    out = []
    for c in text:
        face = f if has_glyph(f, c) else fallback(f.size)
        if out and out[-1][1] is face:
            out[-1] = (out[-1][0] + c, face)
        else:
            out.append((c, face))
    return out


def text_width(draw, text, f):
    return sum(draw.textlength(part, font=face) for part, face in runs(text, f))


def draw_line(draw, center, text, f, fill):
    """1行を center（中心）に描く。代わりのフォントの字も、元のフォントのベースラインに揃える。"""
    ascent, descent = f.getmetrics()
    x = center[0] - text_width(draw, text, f) / 2
    baseline = center[1] + (ascent - descent) / 2
    for part, face in runs(text, f):
        draw.text((x, baseline), part, font=face, fill=fill, anchor="ls")
        x += draw.textlength(part, font=face)


@functools.lru_cache(maxsize=None)
def serif(size, weight="SemiBold"):
    if not SERIF_PATH.exists():
        return font(size)
    f = ImageFont.truetype(str(SERIF_PATH), size)
    try:
        f.set_variation_by_name(weight)
    except Exception:
        pass
    return f


def char_kind(c):
    """折り返しの位置を選ぶための、字の種類。"""
    if "゠" <= c <= "ヿ" or c == "ー":
        return "katakana"
    if "぀" <= c <= "ゟ":
        return "hiragana"
    if "一" <= c <= "鿿" or c in "々〆":
        return "kanji"
    if c.isascii() and c.isalnum():
        return "latin"
    return "other"


def break_score(text, i):
    """text を i 文字目の前で折り返したときの良さ。日本語は単語区切りが無いので、字の並びから見る。"""
    before, after = text[i - 1], text[i]
    kb, ka = char_kind(before), char_kind(after)
    score = 0.0
    if before in "のがをにへとで、』」）" or after in "『「（":
        score += 2.0    # 助詞や区切りの記号の後は、切れ目として読みやすい
    if after in "のがをにへとで":
        score -= 4.0    # 助詞を行頭に残さない
    if after in "・ー、』」）＝" or before in "『「（＝":
        score -= 6.0    # 記号・長音・「＝」を行頭に、括弧の始まりや「＝」を行末に残さない
    if after == "号" or (before.isdigit() and ka == "kanji") or (after.isdigit() and kb == "katakana"):
        score -= 6.0    # 「メイフラワー号」「チャールズ1世」「18日」のように、名前や数に付く番号・「号」・助数詞を離さない
    if text[:i].count("『") > text[:i].count("』") or text[:i].count("「") > text[:i].count("」"):
        score -= 6.0    # 書名や台詞の括弧の中では切らない
    if kb != ka:
        score += 2.0    # 字の種類が変わる所（カタカナの名前と漢字の間など）
    elif kb in ("katakana", "latin"):
        score -= 6.0    # カタカナ語・英数の途中では切らない
    elif kb == "kanji":
        score -= 1.0    # 熟語の途中もなるべく避ける
    # 2行の長さが揃うほど良い
    score -= abs(len(text) - 2 * i) * 0.5
    return score


def split_two_lines(draw, text, f, width):
    """2行に折り返して width に収まる分け方のうち、いちばん良いものを返す（無ければ None）。"""
    best = None
    for i in range(1, len(text)):
        lines = [text[:i], text[i:]]
        if max(text_width(draw, l, f) for l in lines) > width:
            continue
        score = break_score(text, i)
        if best is None or score > best[0]:
            best = (score, lines)
    return best


# 字の並びだけでは語の切れ目が分からない題名の、折り返す位置（"|" の所で切る）
TITLE_BREAKS = {
    "shimonoseki_hogeki": "四国艦隊|下関砲撃事件",
    "east_india_company": "イギリス|東インド会社の設立",
    "voc": "オランダ|東インド会社の設立",
    "dna_helix": "DNA二重らせん|構造の発表",
    "enkai_yochi": "『大日本沿海輿地全図』の|完成",
    "safavid": "サファヴィー朝の|成立",
    "milvian_bridge": "ミルウィウス橋の|戦い",
    "paris_1783": "パリ条約|（アメリカ独立の承認）",
    "lockheed": "ロッキード事件の|発覚",
    "ontake": "登山者を襲った|御嶽山の噴火",
    "kome_sodo": "全国に広がった|米騒動",
    "russian_revolution": "ロマノフ朝を倒した|ロシア革命",
    "bloody_sunday": "ロシアの|血の日曜日事件",
    "quebec": "アブラハム平原の戦い|（ケベック）",
}

# 折り返しの良さの基準と、その基準で探す文字の大きさの下限。
# 良い切れ目が少し小さい文字で見つかるなら、大きい文字で語の途中を切るよりそちらを選ぶ
FIT_PASSES = ((0.0, 36), (-4.0, 0), (float("-inf"), 0))


def fit_text(draw, text, box, max_size, min_size=20, face=None):
    """box に収まる最大の文字サイズを探し、必要なら2行に折り返す。
    text に "|" があればそこで折り返す。無ければ切れ目の良い所を選び、語の途中で切るのは他に収まる手が無いときだけにする。"""
    face = face or serif
    width = box[2] - box[0]
    height = box[3] - box[1]
    if "|" in text:
        lines = text.split("|", 1)
        for size in range(max_size, min_size - 1, -2):
            f = face(size)
            if max(text_width(draw, l, f) for l in lines) <= width and size * LINE_HEIGHT * 2 <= height - TWO_LINE_MARGIN:
                return lines, f
        text = text.replace("|", "")
    for threshold, smallest in FIT_PASSES:
        for size in range(max_size, max(min_size, smallest) - 1, -2):
            f = face(size)
            if text_width(draw, text, f) <= width:
                return [text], f
            if size * LINE_HEIGHT * 2 > height - TWO_LINE_MARGIN:
                continue
            best = split_two_lines(draw, text, f, width)
            if best is not None and best[0] >= threshold:
                return best[1], f
    return [text], face(min_size)


def draw_centered(draw, lines, f, box, fill):
    line_h = f.size * LINE_HEIGHT
    total = line_h * len(lines)
    y = (box[1] + box[3]) / 2 - total / 2
    for line in lines:
        draw_line(draw, ((box[0] + box[2]) / 2, y + line_h / 2), line, f, fill)
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

    draw_title(draw, card)

    # 年号の札。表では伏せ、裏で明かす
    draw.rounded_rectangle(YEAR_BOX, 14, fill=FRAME if reveal else (205, 190, 160), outline=GOLD, width=3)
    label = card["yearLabel"] if reveal else "？？？？"
    lines, f = fit_text(draw, label, (YEAR_BOX[0] + 12, YEAR_BOX[1], YEAR_BOX[2] - 12, YEAR_BOX[3]), 50, face=lambda n: serif(n, "Bold"))
    draw_centered(draw, lines, f, YEAR_BOX, (250, 226, 160) if reveal else FRAME)

    draw.text((W / 2, H - 20), card["category"], font=font(18), fill=GOLD, anchor="md")

    img.putalpha(rounded_mask())
    return img


def draw_title(draw, card):
    lines, f = fit_text(draw, TITLE_BREAKS.get(card["id"], card["title"]), TITLE_BOX, 56)
    draw_centered(draw, lines, f, TITLE_BOX, INK)


def save_face(face, path):
    # 描くのは 512x768 で、保存は 3/4 に縮めて 256 色に減色する。
    # セピア寄りの絵は減色してもほぼ見分けがつかず、容量は 1/4 ほどになる（リポジトリが重くならないように）
    face = face.quantize(256, method=Image.Quantize.FASTOCTREE, dither=Image.Dither.FLOYDSTEINBERG)
    face.save(path, optimize=True)


def retitle_face(card, path):
    """焼いた面の、出来事名の欄だけを描き直す（絵・年号・枠は今の画像のまま）。"""
    scale = SAVE_SIZE[0] / W
    # 欄は羊皮紙の無地なので、原寸で描いてから欄の分だけ縮めて貼る（欄の位置と大きさは 3/4 で割り切れる）
    patch = Image.new("RGBA", (W, H), PARCHMENT + (255,))
    draw_title(ImageDraw.Draw(patch), card)
    box = TITLE_BOX
    region = patch.crop(box).resize((round((box[2] - box[0]) * scale), round((box[3] - box[1]) * scale)), Image.LANCZOS)
    face = Image.open(path).convert("RGBA")
    face.paste(region, (round(box[0] * scale), round(box[1] * scale)))
    save_face(face, path)


# ---------------------------------------------------------------------------
# 日本語以外の言語の面（--lang）
#
# 元の絵（raw/）が手元に無くても作れるよう、焼いてある日本語の面をもとに、
# 文字の欄（出来事名・年号の札・地域の札・分類）だけをその言語で描き直す。
# 欄は原寸の透明な層に描いてから 3/4 に縮め、日本語の面に重ねる。
# ---------------------------------------------------------------------------

TEXT_DIR = ROOT / "ChronoSort" / "Assets" / "Data" / "Text"

# 地域の札の多角形（bake_face と同じ形）
def region_tab(box=ART_BOX):
    return [(box[0] + 14, box[1]), (box[0] + 78, box[1]), (box[0] + 78, box[1] + 70), (box[0] + 46, box[1] + 56), (box[0] + 14, box[1] + 70)]


def split_lines_at_spaces(draw, text, f, width, count):
    """空白の所でだけ count 行に分け、width に収まる分け方のうち行の長さが最も揃うものを返す（無ければ None）。"""
    words = text.split(" ")
    if len(words) < count:
        return None
    best = None

    def walk(start, left, lines):
        nonlocal best
        if left == 1:
            line = " ".join(words[start:])
            if text_width(draw, line, f) > width:
                return
            candidate = lines + [line]
            widths = [text_width(draw, l, f) for l in candidate]
            score = max(widths) - min(widths)
            if best is None or score < best[0]:
                best = (score, candidate)
            return
        for end in range(start + 1, len(words) - left + 2):
            line = " ".join(words[start:end])
            if text_width(draw, line, f) > width:
                break
            walk(end, left - 1, lines + [line])

    walk(0, count, [])
    return best[1] if best else None


def fit_latin(draw, text, box, max_size, min_size=16, face=None):
    """英文を box に収める。いちばん大きく描ける文字の大きさで、1〜3行（空白の所でだけ折り返す）に分ける。"""
    face = face or serif
    width = box[2] - box[0]
    height = box[3] - box[1]
    for size in range(max_size, min_size - 1, -2):
        f = face(size)
        for count in (1, 2, 3):
            if size * LINE_HEIGHT * count > height - (TWO_LINE_MARGIN if count > 1 else 0):
                break
            if count == 1:
                if text_width(draw, text, f) <= width:
                    return [text], f
                continue
            lines = split_lines_at_spaces(draw, text, f, width, count)
            if lines:
                return lines, f
    return [text], face(min_size)


def load_language(lang):
    """言語の文字列の表とカードの訳を読む。"""
    table = json.loads((TEXT_DIR / f"{lang}.json").read_text(encoding="utf-8"))
    cards = json.loads((TEXT_DIR / "Cards" / f"{lang}.json").read_text(encoding="utf-8"))
    return table, cards


def year_label(table, year):
    key = "card.yearBC" if year < 0 else "card.year"
    return table[key].replace("{0}", str(abs(year)))


def relabel_face(card, text, table, reveal, source, dest):
    """日本語の面 source の文字の欄を、訳 text で描き直して dest に保存する。"""
    face = Image.open(source).convert("RGBA")
    # 減色した面の羊皮紙の色は PARCHMENT から少しずれるので、題名の欄でいちばん多い色を地の色として使う
    scale = SAVE_SIZE[0] / W
    title_area = face.crop(tuple(round(v * scale) for v in TITLE_BOX))
    paper = max(title_area.getcolors(title_area.width * title_area.height))[1]

    layer = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    draw = ImageDraw.Draw(layer)

    # 出来事名の欄（羊皮紙の無地なので、無地で塗ってから描く）
    draw.rectangle(TITLE_BOX, fill=paper)
    lines, f = fit_latin(draw, text["title"], TITLE_BOX, 52)
    draw_centered(draw, lines, f, TITLE_BOX, INK + (255,))

    # 年号の札（下の羊皮紙ごと塗り直して、同じ形の札を描く）
    # 塗る四角の端は 4 の倍数にそろえる（3/4 に縮めたとき画素の境目に乗り、縁がにじまない）
    draw.rectangle((92, 640, 420, 728), fill=paper)
    draw.rounded_rectangle(YEAR_BOX, 14, fill=(FRAME if reveal else (205, 190, 160)) + (255,), outline=GOLD + (255,), width=3)
    label = year_label(table, card["year"]) if reveal else "????"
    lines, f = fit_latin(draw, label, (YEAR_BOX[0] + 12, YEAR_BOX[1], YEAR_BOX[2] - 12, YEAR_BOX[3]), 50, face=lambda n: serif(n, "Bold"))
    draw_centered(draw, lines, f, YEAR_BOX, ((250, 226, 160) if reveal else FRAME) + (255,))

    # 地域の札（同じ多角形で覆い、札の幅に入る大きさで書く）
    tab_color = (168, 40, 36) if card["region"] == "JP" else (36, 70, 120)
    draw.polygon(region_tab(), fill=tab_color + (255,))
    region = table[f"region.{card['region']}"].upper()
    tab_width = 64 - 8
    size = 22
    while size > 10 and text_width(draw, region, serif(size, "Bold")) > tab_width:
        size -= 1
    draw.text((ART_BOX[0] + 46, ART_BOX[1] + 26), region, font=serif(size, "Bold"), fill=(250, 236, 210, 255), anchor="mm")

    # 分類（下端の羊皮紙の帯。日本語の字は金の内枠の線に少しかかるので、線ごと塗ってから線を引き直す）
    draw.rectangle((144, 728, 368, 752), fill=paper)
    draw.rectangle((144, H - 23, 368, H - 21), fill=GOLD + (255,))
    category = table.get(f"category.{card['category']}", card["category"])
    draw.text((W / 2, H - 20), category, font=font(18), fill=GOLD + (255,), anchor="md")

    # 縮めるのは BOX で（LANCZOS は四角の縁に明暗の筋を出す）
    face.alpha_composite(layer.resize(face.size, Image.BOX))
    save_face(face, dest)


def bake_language(lang, only):
    table, cards = load_language(lang)
    out_dir = OUT_DIR / lang
    out_dir.mkdir(parents=True, exist_ok=True)
    count = 0
    missing = []
    for deck_path in sorted(DECK_DIR.glob("*.json")):
        deck = json.loads(deck_path.read_text(encoding="utf-8"))
        for card in deck["cards"]:
            if only and card["id"] not in only:
                continue
            text = cards.get(card["id"])
            if text is None:
                missing.append(card["id"])
                continue
            for face, reveal in (("front", False), ("back", True)):
                name = f"{card['id']}_{face}"
                relabel_face(card, text, table, reveal, OUT_DIR / f"{name}.png", out_dir / f"{name}.png")
                write_tmat(out_dir / f"{name}.tmat", f"{ASSET_PREFIX}/Cards/{lang}/{name}.png")
            count += 1
    if missing:
        print(f"no {lang} text for: {', '.join(missing)}")
    print(f"baked {count} {lang} cards to {out_dir}")


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
    parser.add_argument("--retitle", action="store_true")
    parser.add_argument("--lang", default="")
    args = parser.parse_args()
    # 指定したカードの面だけを焼く（卓・山札の素材と、他のカードの面には触れない）
    only = {s for s in args.only.split(",") if s}

    if args.lang:
        bake_language(args.lang, only)
        return

    if args.retitle:
        count = 0
        for deck_path in sorted(DECK_DIR.glob("*.json")):
            deck = json.loads(deck_path.read_text(encoding="utf-8"))
            for card in deck["cards"]:
                if only and card["id"] not in only:
                    continue
                for face in ("front", "back"):
                    retitle_face(card, OUT_DIR / f"{card['id']}_{face}.png")
                count += 1
        print(f"retitled {count} cards in {OUT_DIR}")
        return

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
                save_face(bake_face(card, art, reveal).resize(SAVE_SIZE, Image.LANCZOS), OUT_DIR / f"{name}.png")
                write_tmat(OUT_DIR / f"{name}.tmat", f"{ASSET_PREFIX}/Cards/{name}.png")
            count += 1
    if not only:
        bake_table()
    print(f"baked {count} cards to {OUT_DIR}")


if __name__ == "__main__":
    main()
