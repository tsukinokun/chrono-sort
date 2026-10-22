"""BGM（パブリックドメイン / CC0 のクラシックの演奏）を Wikimedia Commons から取得し、mp3 に変換する。

    python Tools/audio/fetch_bgm.py

曲の演奏そのものにも著作権（著作隣接権）があるため、演奏の録音が PD / CC0 のもの
（Musopen が公開した録音など）だけを使う。出典は ChronoSort/Assets/Data/Credits.json の audio に記録する。
変換には ffmpeg（PATH 上）を使う。
"""

import json
import re
import shutil
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "art"))
from fetch_art import api_get, download, license_ok, meta  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
OUT_DIR = ROOT / "ChronoSort" / "Assets" / "Audio" / "Bgm"
WORK_DIR = ROOT / "Tools" / "audio" / "raw"
CREDITS = ROOT / "ChronoSort" / "Assets" / "Data" / "Credits.json"

# (ゲーム内の名前, 曲名, 検索語の候補)
TRACKS = [
    ("title", "サティ：ジムノペディ第1番", ["Gymnopedie 1 Satie Musopen", "Satie Gymnopedie No. 1", "Gymnopédie"]),
    ("game", "バッハ：平均律クラヴィーア曲集 第1巻 前奏曲 ハ長調", ["Bach Prelude C major BWV 846", "Well-Tempered Clavier Book 1 Prelude 1", "BWV 846 Prelude"]),
    ("result", "ショパン：夜想曲 変ホ長調 作品9-2", ["Chopin Nocturne Op. 9 No. 2", "Chopin Nocturne E flat major Op 9 no 2"]),
]

AUDIO_MIMES = ("application/ogg", "audio/ogg", "audio/x-flac", "audio/flac", "audio/mpeg", "audio/wav", "audio/x-wav")


def find_audio(queries):
    for query in queries:
        data = api_get({
            "action": "query",
            "generator": "search",
            "gsrsearch": f"{query} filetype:audio",
            "gsrnamespace": "6",
            "gsrlimit": "15",
            "prop": "imageinfo",
            "iiprop": "url|extmetadata|size|mime",
        })
        pages = sorted(data.get("query", {}).get("pages", []), key=lambda p: p.get("index", 0))
        for page in pages:
            info = (page.get("imageinfo") or [{}])[0]
            title = page.get("title", "")
            if info.get("mime") not in AUDIO_MIMES:
                continue
            # 抜粋や MIDI 由来の短い録音は避ける（2分以上を目安に、容量で判断する）
            if info.get("size", 0) < 1_500_000:
                continue
            if re.search(r"midi|excerpt|sample", title, re.I):
                continue
            if license_ok(meta(info, "LicenseShortName")):
                return title, info
    return None, None


def main():
    if shutil.which("ffmpeg") is None:
        print("ffmpeg not found on PATH")
        return 1
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    WORK_DIR.mkdir(parents=True, exist_ok=True)
    credits = json.loads(CREDITS.read_text(encoding="utf-8")) if CREDITS.exists() else {"images": [], "audio": []}
    audio = {a["id"]: a for a in credits.get("audio", [])}

    for name, piece, queries in TRACKS:
        title, info = find_audio(queries)
        if not info:
            print(f"{name}: no usable recording found")
            continue
        source = WORK_DIR / (name + Path(info["url"]).suffix)
        print(f"{name}: {title} [{meta(info, 'LicenseShortName')}]")
        download(info["url"], source)
        # 音量をそろえ、モノラル 44.1kHz の mp3 にする（容量を抑える。BGM なので定位は不要）
        subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-i", str(source), "-ac", "1", "-ar", "44100",
                        "-af", "loudnorm=I=-20:TP=-2", "-b:a", "128k", str(OUT_DIR / f"{name}.mp3")], check=True)
        audio[name] = {
            "id": name,
            "title": piece,
            "file": title,
            "page": info.get("descriptionurl", ""),
            "artist": meta(info, "Artist") or "Unknown",
            "license": meta(info, "LicenseShortName"),
            "licenseUrl": meta(info, "LicenseUrl"),
        }

    credits["audio"] = sorted(audio.values(), key=lambda a: a["id"])
    CREDITS.write_text(json.dumps(credits, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
