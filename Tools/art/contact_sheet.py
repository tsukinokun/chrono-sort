"""取得したカードの絵を一覧（コンタクトシート）にして、題材と合っているかを目で確かめる。

    python Tools/art/contact_sheet.py <出力先フォルダ>

1枚に 30 件ずつ（6 列 × 5 段）並べ、各絵の下に id と出来事名を書く。
"""

import json
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont, ImageOps

ROOT = Path(__file__).resolve().parents[2]
DECK_DIR = ROOT / "ChronoSort" / "Assets" / "Data" / "Decks"
RAW_DIR = ROOT / "Tools" / "art" / "raw"
FONT = ROOT / "External" / "TsukinoEngine" / "Tsukino.BuiltIn" / "Assets" / "Fonts" / "NotoSansCJKjp-Regular.otf"

COLS, ROWS = 6, 5
CELL_W, CELL_H, THUMB_H = 260, 250, 200


def main():
    out_dir = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "Tools" / "art" / "raw"
    out_dir.mkdir(parents=True, exist_ok=True)
    cards = []
    for deck in sorted(DECK_DIR.glob("*.json")):
        cards += json.loads(deck.read_text(encoding="utf-8"))["cards"]
    font = ImageFont.truetype(str(FONT), 15)
    per_page = COLS * ROWS
    for page in range((len(cards) + per_page - 1) // per_page):
        sheet = Image.new("RGB", (COLS * CELL_W, ROWS * CELL_H), (40, 34, 30))
        draw = ImageDraw.Draw(sheet)
        for i, card in enumerate(cards[page * per_page:(page + 1) * per_page]):
            x, y = (i % COLS) * CELL_W, (i // COLS) * CELL_H
            files = list(RAW_DIR.glob(f"{card['id']}.*"))
            if files:
                thumb = ImageOps.contain(Image.open(files[0]).convert("RGB"), (CELL_W - 10, THUMB_H))
                sheet.paste(thumb, (x + (CELL_W - thumb.width) // 2, y + 5))
            else:
                draw.rectangle((x + 5, y + 5, x + CELL_W - 5, y + THUMB_H), outline=(200, 60, 60), width=3)
            draw.text((x + 6, y + THUMB_H + 8), card["id"], font=font, fill=(200, 200, 200))
            draw.text((x + 6, y + THUMB_H + 26), f"{card['title']} ({card['yearLabel']})", font=font, fill=(240, 220, 170))
        sheet.save(out_dir / f"contact_{page + 1}.png")
    print(f"wrote {page + 1} sheets to {out_dir}")


if __name__ == "__main__":
    main()
