"""CPU の名札に使う歴史家の肖像を Wikimedia Commons から取得し、名札用に加工する。

    python Tools/art/fetch_portraits.py

CPU の名前（ヘロドトス・司馬遷・ギボン）は Rules/Match.cpp の kCpuNames と対応している。
出典は Credits.json の images に portrait_* の id で追記する。
"""

import json
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter, ImageOps

sys.path.insert(0, str(Path(__file__).resolve().parent))
from fetch_art import CREDITS, RAW_DIR, acceptable, api_get, candidates_from, download, meta  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
UI_DIR = ROOT / "ChronoSort" / "Assets" / "Textures" / "UI"

# (ファイル名に使う名前, 表示名, 検索語)
PORTRAITS = [
    ("herodotus", "ヘロドトス", "Herodotus bust Massimo"),
    ("sima_qian", "司馬遷", "Sima Qian portrait"),
    ("gibbon", "ギボン", "Edward Gibbon portrait Reynolds"),
]


def make_portrait(source, out):
    """古い肖像画風に：顔の辺りを切り抜き、セピアにして楕円の窓に収める。"""
    size = (140, 144)
    img = ImageOps.fit(Image.open(source).convert("RGB"), size, Image.LANCZOS, centering=(0.5, 0.3))
    img = ImageOps.colorize(ImageOps.grayscale(img), (46, 30, 18), (244, 226, 190))
    mask = Image.new("L", size, 0)
    ImageDraw.Draw(mask).ellipse((6, 4, size[0] - 6, size[1] - 4), fill=255)
    mask = mask.filter(ImageFilter.GaussianBlur(3))
    img = img.convert("RGBA")
    img.putalpha(mask)
    img.save(out)


def main():
    credits = json.loads(CREDITS.read_text(encoding="utf-8"))
    by_id = {c["id"]: c for c in credits.get("images", [])}
    for key, name, query in PORTRAITS:
        data = api_get({
            "action": "query", "generator": "search", "gsrsearch": f"{query} filetype:bitmap", "gsrnamespace": "6", "gsrlimit": "10",
            "prop": "imageinfo", "iiprop": "url|extmetadata|size|mime", "iiurlwidth": "600",
        })
        chosen = None
        for title, info in candidates_from(data):
            ok, _ = acceptable(info)
            if ok:
                chosen = (title, info)
                break
        if not chosen:
            print(f"{key}: no usable portrait")
            continue
        title, info = chosen
        raw = RAW_DIR / f"portrait_{key}.jpg"
        download(info.get("thumburl") or info["url"], raw)
        make_portrait(raw, UI_DIR / f"portrait_{key}.png")
        by_id[f"portrait_{key}"] = {
            "id": f"portrait_{key}",
            "title": f"{name}（CPU の肖像）",
            "file": title,
            "page": info.get("descriptionurl", ""),
            "artist": meta(info, "Artist") or "Unknown",
            "license": meta(info, "LicenseShortName"),
            "licenseUrl": meta(info, "LicenseUrl"),
        }
        print(f"{key}: {title} [{by_id[f'portrait_{key}']['license']}]")
    credits["images"] = sorted(by_id.values(), key=lambda c: c["id"])
    CREDITS.write_text(json.dumps(credits, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
