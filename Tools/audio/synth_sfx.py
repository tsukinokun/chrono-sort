"""効果音を合成して wav に書き出す（標準ライブラリのみ。素材のライセンスを気にしなくてよい）。

    python Tools/audio/synth_sfx.py

紙のカードを扱う机の上の音を目指して、ノイズの擦れ・板の鳴り・ベルの倍音を組み合わせる。
出力: ChronoSort/Assets/Audio/Sfx/<name>.wav（44.1kHz / 16bit / モノラル）
"""

import math
import random
import struct
import wave
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUT_DIR = ROOT / "ChronoSort" / "Assets" / "Audio" / "Sfx"
RATE = 44100


def silence(seconds):
    return [0.0] * int(RATE * seconds)


def mix(*tracks):
    length = max(len(t) for t in tracks)
    out = [0.0] * length
    for t in tracks:
        for i, v in enumerate(t):
            out[i] += v
    return out


def offset(track, seconds):
    return silence(seconds) + track


def envelope(n, attack, decay_rate):
    """立ち上がり attack 秒、その後 exp(-decay_rate × t) で減衰する包絡。"""
    a = max(1, int(RATE * attack))
    return [i / a if i < a else math.exp(-decay_rate * (i - a) / RATE) for i in range(n)]


def noise(seconds, seed):
    rng = random.Random(seed)
    return [rng.uniform(-1.0, 1.0) for _ in range(int(RATE * seconds))]


def lowpass(track, cutoff):
    """1次のローパス。cutoff は固定値かサンプル番号を受け取る関数。"""
    out, y = [], 0.0
    for i, x in enumerate(track):
        c = cutoff(i) if callable(cutoff) else cutoff
        k = 1.0 - math.exp(-2.0 * math.pi * c / RATE)
        y += (x - y) * k
        out.append(y)
    return out


def highpass(track, cutoff):
    low = lowpass(track, cutoff)
    return [x - l for x, l in zip(track, low)]


def tone(freq, seconds, partials=((1.0, 1.0),), decay=6.0, attack=0.004, vibrato=0.0):
    """倍音を重ねた減衰音。partials は (周波数の倍率, 音量) の組。"""
    n = int(RATE * seconds)
    env = envelope(n, attack, decay)
    out = []
    for i in range(n):
        t = i / RATE
        f = freq * (1.0 + vibrato * math.sin(2 * math.pi * 5.5 * t))
        v = sum(a * math.sin(2 * math.pi * f * r * t) for r, a in partials)
        out.append(v * env[i])
    return out


def scale(track, gain):
    return [v * gain for v in track]


def apply(track, env):
    return [v * e for v, e in zip(track, env)]


def write(name, track, peak=0.85):
    top = max(1e-6, max(abs(v) for v in track))
    gain = peak / top
    # 末尾のクリック音を防ぐため、最後の 5ms をなだらかに消す
    fade = int(RATE * 0.005)
    for i in range(fade):
        track[-1 - i] *= i / fade
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    with wave.open(str(OUT_DIR / f"{name}.wav"), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(b"".join(struct.pack("<h", int(max(-1.0, min(1.0, v * gain)) * 32767)) for v in track))


BELL = ((1.0, 1.0), (2.0, 0.35), (2.76, 0.25), (5.4, 0.08))
PLUCK = ((1.0, 1.0), (2.0, 0.5), (3.0, 0.25), (4.0, 0.12))
WOOD = ((1.0, 1.0), (2.3, 0.4), (4.1, 0.15))


def card_slide():
    n = noise(0.22, 1)
    env = [math.sin(math.pi * i / len(n)) ** 1.5 for i in range(len(n))]
    return apply(highpass(lowpass(n, 5500), 900), env)


def deal():
    n = noise(0.09, 2)
    flick = apply(highpass(lowpass(n, 7000), 1800), envelope(len(n), 0.003, 45))
    return mix(flick, scale(tone(1900, 0.03, WOOD, decay=120), 0.15))


def card_flip():
    a = apply(highpass(lowpass(noise(0.07, 3), 6000), 1200), envelope(int(RATE * 0.07), 0.002, 55))
    b = apply(highpass(lowpass(noise(0.09, 4), 4500), 700), envelope(int(RATE * 0.09), 0.002, 40))
    return mix(a, offset(scale(b, 0.8), 0.06))


def card_place():
    thump = tone(95, 0.3, ((1.0, 1.0), (1.5, 0.3)), decay=18, attack=0.002)
    click = apply(highpass(noise(0.03, 5), 2500), envelope(int(RATE * 0.03), 0.001, 150))
    paper = apply(lowpass(noise(0.12, 6), 2500), envelope(int(RATE * 0.12), 0.002, 35))
    return mix(thump, scale(click, 0.35), scale(paper, 0.5))


def whoosh():
    n = noise(0.4, 7)
    length = len(n)
    sweep = lambda i: 300 + 2600 * math.sin(math.pi * i / length)
    env = [math.sin(math.pi * i / length) ** 2 for i in range(length)]
    return apply(lowpass(n, sweep), env)


def tick():
    return mix(tone(2300, 0.05, WOOD, decay=110, attack=0.001), scale(apply(highpass(noise(0.01, 8), 3000), envelope(int(RATE * 0.01), 0.0005, 300)), 0.2))


def correct():
    # ハ長調の上行（ド・ミ・ソ・ド）をベルの音色で
    notes = [1046.5, 1318.5, 1568.0, 2093.0]
    tracks = [offset(scale(tone(f, 0.9, BELL, decay=5.5), 0.6 if k < 3 else 0.8), 0.07 * k) for k, f in enumerate(notes)]
    return mix(*tracks)


def wrong():
    low = tone(196, 0.55, ((1.0, 1.0), (2.0, 0.25), (3.0, 0.15)), decay=7, attack=0.01)
    lower = offset(tone(174.6, 0.6, ((1.0, 1.0), (2.0, 0.25), (3.0, 0.15)), decay=6, attack=0.01), 0.14)
    thud = tone(70, 0.3, ((1.0, 1.0),), decay=14)
    return mix(scale(low, 0.6), scale(lower, 0.7), scale(thud, 0.6))


def stamp():
    hit = tone(120, 0.25, ((1.0, 1.0), (2.1, 0.3)), decay=25, attack=0.001)
    crunch = apply(lowpass(noise(0.08, 9), 3500), envelope(int(RATE * 0.08), 0.001, 50))
    return mix(hit, scale(crunch, 0.6))


def button():
    return mix(scale(tone(1500, 0.06, PLUCK, decay=70), 0.6), scale(card_flip()[: int(RATE * 0.05)], 0.4))


def fanfare():
    # 明るい分散和音を撥弦の音色で（ド・ミ・ソ → 上のド・ミの和音）
    seq = [(523.3, 0.0), (659.3, 0.12), (784.0, 0.24), (1046.5, 0.36)]
    tracks = [offset(tone(f, 1.0, PLUCK, decay=4.0), t) for f, t in seq]
    chord = [offset(tone(f, 1.6, BELL, decay=2.5), 0.55) for f in (1046.5, 1318.5, 1568.0)]
    return mix(*tracks, *[scale(c, 0.6) for c in chord])


def lose():
    seq = [(440.0, 0.0), (392.0, 0.22), (349.2, 0.44), (293.7, 0.66)]
    return mix(*[offset(tone(f, 0.9, PLUCK, decay=4.5), t) for f, t in seq])


def page():
    """紙をめくる音（結果画面・メニュー）。"""
    n = noise(0.35, 10)
    length = len(n)
    env = [(math.sin(math.pi * i / length) ** 3) for i in range(length)]
    return apply(highpass(lowpass(n, lambda i: 1500 + 4000 * i / length), 600), env)


SOUNDS = {
    "card_slide": card_slide,
    "deal": deal,
    "card_flip": card_flip,
    "card_place": card_place,
    "whoosh": whoosh,
    "tick": tick,
    "correct": correct,
    "wrong": wrong,
    "stamp": stamp,
    "button": button,
    "fanfare": fanfare,
    "lose": lose,
    "page": page,
}


def main():
    for name, make in SOUNDS.items():
        write(name, make())
    print(f"wrote {len(SOUNDS)} sounds to {OUT_DIR}")


if __name__ == "__main__":
    main()
