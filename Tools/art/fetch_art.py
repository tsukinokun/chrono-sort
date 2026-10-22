"""Wikimedia Commons からカードの絵を取得し、出典を Credits.json に記録する。

    python Tools/art/fetch_art.py                 まだ絵の無いカードだけ取得
    python Tools/art/fetch_art.py --force         全カードを取り直す
    python Tools/art/fetch_art.py --only a,b      指定したカードだけ取り直す

使えるライセンスは パブリックドメイン / CC0 / CC BY のみ。
CC BY-SA（継承）・NC（非営利）・ND（改変禁止）は、カードの絵として加工して配るのに向かないので使わない。

カードごとの検索語はデッキ JSON の artQuery、ファイルの指定は artFile（Tools/art/card_list.py で管理）。
取得した画像は Tools/art/raw/<id>.<拡張子>（gitignore）、出典は ChronoSort/Assets/Data/Credits.json。
"""

import argparse
import html
import json
import re
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DECK_DIR = ROOT / "ChronoSort" / "Assets" / "Data" / "Decks"
RAW_DIR = ROOT / "Tools" / "art" / "raw"
CREDITS = ROOT / "ChronoSort" / "Assets" / "Data" / "Credits.json"

API = "https://commons.wikimedia.org/w/api.php"
# Wikimedia の利用方針に沿って、何のための取得かが分かる User-Agent を付ける
USER_AGENT = "ChronoSortAssetFetcher/1.0 (hobby history card game; one-off asset fetch)"
REQUEST_INTERVAL = 2.0
MAX_RETRIES = 5
THUMB_WIDTH = 900


def fetch(url, timeout):
    """GET して本文を返す。429（混雑）のときは Retry-After に従って待ってから取り直す。"""
    for attempt in range(MAX_RETRIES):
        req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
        try:
            with urllib.request.urlopen(req, timeout=timeout) as res:
                body = res.read()
            time.sleep(REQUEST_INTERVAL)
            return body
        except urllib.error.HTTPError as e:
            if e.code != 429 or attempt == MAX_RETRIES - 1:
                raise
            wait = int(e.headers.get("Retry-After", "0") or 0) or 20 * (attempt + 1)
            print(f"  rate limited, waiting {wait}s")
            time.sleep(wait)
    raise RuntimeError("unreachable")


def api_get(params):
    params = {"format": "json", "formatversion": "2", **params}
    return json.loads(fetch(API + "?" + urllib.parse.urlencode(params), 30))


def download(url, path):
    path.write_bytes(fetch(url, 60))


def strip_html(text):
    text = re.sub(r"<[^>]+>", "", text or "")
    return html.unescape(text).strip()


def license_ok(short_name):
    """配布に使えるライセンスかを判定する。"""
    name = (short_name or "").lower()
    if not name:
        return False
    if "sa" in re.split(r"[\s\-]+", name) or "nc" in name or "nd" in re.split(r"[\s\-]+", name):
        return False
    if "public domain" in name or name.startswith("pd") or "cc0" in name or "no restrictions" in name:
        return True
    return re.match(r"cc[ -]by[ -]\d", name) is not None


def meta(info, key):
    return strip_html(info.get("extmetadata", {}).get(key, {}).get("value", ""))


def candidates_from(data):
    pages = data.get("query", {}).get("pages", [])
    # generator=search のときは index（検索順位）で並べ直す
    pages.sort(key=lambda p: p.get("index", 0))
    for page in pages:
        infos = page.get("imageinfo") or []
        if not infos:
            continue
        info = infos[0]
        yield page.get("title", ""), info


def acceptable(info):
    if info.get("mime") not in ("image/jpeg", "image/png", "image/tiff"):
        return False, "mime"
    w, h = info.get("width", 0), info.get("height", 0)
    if w < 400 or h < 300:
        return False, "small"
    if not (0.4 <= w / max(h, 1) <= 2.8):
        return False, "aspect"
    lic = meta(info, "LicenseShortName")
    if not license_ok(lic):
        return False, f"license:{lic}"
    return True, lic


def find_image(card):
    iiprops = {"prop": "imageinfo", "iiprop": "url|extmetadata|size|mime", "iiurlwidth": str(THUMB_WIDTH)}
    if card.get("artFile"):
        title = card["artFile"] if card["artFile"].startswith("File:") else "File:" + card["artFile"]
        data = api_get({"action": "query", "titles": title, **iiprops})
    else:
        data = api_get({
            "action": "query",
            "generator": "search",
            "gsrsearch": f'{card["artQuery"]} filetype:bitmap',
            "gsrnamespace": "6",
            "gsrlimit": "15",
            **iiprops,
        })
    reasons = []
    for title, info in candidates_from(data):
        ok, why = acceptable(info)
        if ok:
            return title, info
        reasons.append(f"{title[:50]}({why})")
    print(f"  no usable image for {card['id']}: {'; '.join(reasons[:4])}")
    return None, None


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--force", action="store_true")
    parser.add_argument("--only", default="")
    args = parser.parse_args()
    only = {s for s in args.only.split(",") if s}

    RAW_DIR.mkdir(parents=True, exist_ok=True)
    credits = json.loads(CREDITS.read_text(encoding="utf-8")) if CREDITS.exists() else {"images": [], "audio": []}
    by_id = {c["id"]: c for c in credits.get("images", [])}

    cards = []
    for deck in sorted(DECK_DIR.glob("*.json")):
        cards += json.loads(deck.read_text(encoding="utf-8"))["cards"]

    fetched = missing = 0
    for card in cards:
        cid = card["id"]
        if only and cid not in only:
            continue
        existing = list(RAW_DIR.glob(f"{cid}.*"))
        if existing and not args.force and not only and cid in by_id:
            continue
        print(f"{cid}: {card.get('artFile') or card['artQuery']}")
        try:
            title, info = find_image(card)
        except Exception as e:  # 通信の失敗は飛ばして次へ（後で --only で取り直せる）
            print(f"  error: {e}")
            missing += 1
            continue
        if not info:
            by_id.pop(cid, None)
            missing += 1
            continue
        url = info.get("thumburl") or info["url"]
        ext = ".png" if url.lower().endswith(".png") else ".jpg"
        for old in existing:
            old.unlink()
        download(url, RAW_DIR / f"{cid}{ext}")
        by_id[cid] = {
            "id": cid,
            "title": card["title"],
            "file": title,
            "page": info.get("descriptionurl", ""),
            "artist": meta(info, "Artist") or "Unknown",
            "license": meta(info, "LicenseShortName"),
            "licenseUrl": meta(info, "LicenseUrl"),
        }
        fetched += 1
        print(f"  -> {title} [{by_id[cid]['license']}]")

    credits["images"] = sorted(by_id.values(), key=lambda c: c["id"])
    CREDITS.write_text(json.dumps(credits, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"fetched {fetched}, missing {missing}, credited {len(credits['images'])}")


if __name__ == "__main__":
    sys.exit(main())
