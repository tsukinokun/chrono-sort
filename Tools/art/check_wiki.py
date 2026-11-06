"""card_notes.py の記事名を、Wikipedia（日本語版）で確かめる。

    python Tools/art/check_wiki.py      無い記事・曖昧さ回避・転送を一覧にする

転送は転送先の記事名を出す（card_notes.py の記事名を直す目安）。
記事の冒頭は Tools/art/raw/wiki_intro.json（gitignore）に控える。説明文を確かめるときに読む。
"""

import json
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path

import card_notes

RAW = Path(__file__).resolve().parent / "raw" / "wiki_intro.json"
API = "https://ja.wikipedia.org/w/api.php"
# Wikimedia の利用方針に沿って、何のための取得かが分かる User-Agent を付ける
USER_AGENT = "ChronoSortAssetFetcher/1.0 (hobby history card game; one-off title check)"
BATCH = 20               # extracts は1回に20件まで返る
REQUEST_INTERVAL = 3.0
MAX_RETRIES = 6


def fetch(params):
    """API を呼ぶ。429（混雑）のときは Retry-After に従って待ってから取り直す。"""
    url = API + "?" + urllib.parse.urlencode({"format": "json", "formatversion": "2", **params})
    for attempt in range(MAX_RETRIES):
        request = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
        try:
            with urllib.request.urlopen(request, timeout=30) as response:
                body = response.read()
            time.sleep(REQUEST_INTERVAL)
            return json.loads(body)
        except urllib.error.HTTPError as e:
            if e.code != 429 or attempt == MAX_RETRIES - 1:
                raise
            wait = int(e.headers.get("Retry-After", "0") or 0) or 30 * (attempt + 1)
            print(f"  rate limited, waiting {wait}s", flush=True)
            time.sleep(wait)
    raise RuntimeError("unreachable")


def query(titles):
    """記事名の一覧を問い合わせ、{もとの記事名: (転送後の記事名, ページ)} を返す。"""
    pages, moved = {}, {}
    params = {
        "action": "query", "titles": "|".join(titles), "redirects": 1,
        "prop": "extracts|pageprops", "ppprop": "disambiguation",
        "exintro": 1, "explaintext": 1, "exlimit": "max", "exchars": 400,
    }
    while True:
        result = fetch(params)
        body = result.get("query", {})
        for entry in body.get("normalized", []) + body.get("redirects", []):
            moved[entry["from"]] = entry["to"]
        for page in body.get("pages", []):
            pages.setdefault(page["title"], {}).update(page)
        if "continue" not in result:
            break
        params.update(result["continue"])
    resolved = {}
    for title in titles:
        final = title
        while final in moved:
            final = moved[final]
        resolved[title] = (final, pages.get(final, {"missing": True}))
    return resolved


def main():
    titles = sorted({wiki for wiki, _ in card_notes.NOTES.values()})
    resolved = {}
    for i in range(0, len(titles), BATCH):
        resolved.update(query(titles[i:i + BATCH]))
        print(f"{min(i + BATCH, len(titles))}/{len(titles)}", flush=True)

    missing, ambiguous, redirected, intros = [], [], [], {}
    for card_id, (wiki, _) in card_notes.NOTES.items():
        final, page = resolved[wiki]
        if page.get("missing"):
            missing.append((card_id, wiki))
            continue
        if "disambiguation" in page.get("pageprops", {}):
            ambiguous.append((card_id, wiki))
        if final != wiki:
            redirected.append((card_id, wiki, final))
        intros[card_id] = {"wiki": final, "intro": page.get("extract", "")}

    RAW.parent.mkdir(exist_ok=True)
    RAW.write_text(json.dumps(intros, ensure_ascii=False, indent=1), encoding="utf-8")
    for label, rows in (("MISSING", missing), ("AMBIGUOUS", ambiguous), ("REDIRECT", redirected)):
        print(f"--- {label}: {len(rows)}")
        for row in rows:
            print("|".join(row))
    return 1 if missing or ambiguous else 0


if __name__ == "__main__":
    sys.exit(main())
