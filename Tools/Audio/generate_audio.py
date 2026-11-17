"""YJJDefense 효과음·배경음 합성기. 외부 음원 없이 파형을 직접 만든다 (상업적 사용 자유).

실행 (Tools/Art/BuildArt.ps1이 대신 해 준다):
    python Tools/Audio/generate_audio.py <출력 폴더>
"""

import math
import os
import random
import struct
import sys
import wave

RATE = 22050


def osc(kind, phase):
    p = phase % 1.0
    if kind == "sine":
        return math.sin(2 * math.pi * p)
    if kind == "square":
        return 1.0 if p < 0.5 else -1.0
    if kind == "pulse":
        return 1.0 if p < 0.25 else -1.0
    if kind == "saw":
        return 2.0 * p - 1.0
    return 4.0 * abs(p - 0.5) - 1.0  # triangle


def tone(freq, seconds, kind="square", volume=0.5, attack=0.005, release=0.08, end_freq=None, vibrato=0.0):
    """주파수가 freq → end_freq로 미끄러지는 음 하나."""
    count = int(seconds * RATE)
    out = []
    phase = 0.0
    for i in range(count):
        t = i / RATE
        u = i / max(count - 1, 1)
        f = freq if end_freq is None else freq * (end_freq / freq) ** u
        if vibrato:
            f *= 1.0 + vibrato * math.sin(2 * math.pi * 7.0 * t)
        phase += f / RATE
        env = min(t / attack, 1.0) * min((seconds - t) / release, 1.0)
        out.append(osc(kind, phase) * volume * max(env, 0.0))
    return out


def noise(seconds, volume=0.5, decay=6.0, lowpass=0.0, seed=1):
    rng = random.Random(seed)
    count = int(seconds * RATE)
    out = []
    last = 0.0
    for i in range(count):
        t = i / RATE
        sample = rng.uniform(-1.0, 1.0)
        if lowpass:
            last += (sample - last) * lowpass
            sample = last
        out.append(sample * volume * math.exp(-decay * t))
    return out


def mix(*tracks):
    length = max(len(t) for t in tracks)
    out = [0.0] * length
    for track in tracks:
        for i, sample in enumerate(track):
            out[i] += sample
    return out


def seq(*tracks):
    out = []
    for track in tracks:
        out.extend(track)
    return out


def at(track, seconds):
    return [0.0] * int(seconds * RATE) + track


def save(path, samples):
    peak = max(max(abs(s) for s in samples), 1e-6)
    scale = min(1.0, 0.9 / peak)
    with wave.open(path, "wb") as f:
        f.setnchannels(1)
        f.setsampwidth(2)
        f.setframerate(RATE)
        f.writeframes(b"".join(struct.pack("<h", int(max(-1.0, min(1.0, s * scale)) * 32767)) for s in samples))


NOTE = {"C": 0, "D": 2, "E": 4, "F": 5, "G": 7, "A": 9, "B": 11}


def hz(name):
    """'C4', 'F#5' → 주파수."""
    semitone = NOTE[name[0]] + (1 if "#" in name else 0)
    octave = int(name[-1])
    return 440.0 * 2 ** ((semitone - 9) / 12.0 + (octave - 4))


def make_bgm():
    """느긋하고 엉뚱한 들판 행진곡. 8마디 루프."""
    bpm = 112.0
    beat = 60.0 / bpm
    lead = [
        ("E5", 1), ("G5", 1), ("A5", 1), ("G5", 1), ("E5", 1), ("C5", 1), ("D5", 2),
        ("E5", 1), ("G5", 1), ("A5", 1), ("C6", 1), ("B5", 1), ("G5", 1), ("A5", 2),
        ("F5", 1), ("A5", 1), ("C6", 1), ("A5", 1), ("G5", 1), ("E5", 1), ("G5", 2),
        ("F5", 1), ("E5", 1), ("D5", 1), ("G5", 1), ("E5", 1), ("D5", 1), ("C5", 2),
    ]
    bass = ["C3", "C3", "A2", "A2", "F2", "F2", "G2", "G2"]

    out = [0.0] * (int(16 * beat * RATE * 2) + RATE)

    def add(track, seconds):
        start = int(seconds * RATE)
        for i, sample in enumerate(track):
            if start + i < len(out):
                out[start + i] += sample

    t = 0.0
    for name, eighths in lead:
        seconds = eighths * beat * 0.5
        add(tone(hz(name), seconds * 0.9, "pulse", 0.2, release=0.05, vibrato=0.004), t)
        t += seconds
    song = t

    # 베이스: 마디마다 쿵-짝
    bar = song / len(bass)
    for index, name in enumerate(bass):
        for step in range(4):
            freq = hz(name) * (1.0 if step % 2 == 0 else 1.5)
            add(tone(freq, bar / 4 * 0.8, "triangle", 0.3, release=0.06), index * bar + step * bar / 4)
        # 뿅뿅 하는 엇박
        for step in (1, 3):
            add(noise(0.05, 0.12, decay=60.0, seed=index * 4 + step), index * bar + step * bar / 4)

    return out[: int(song * RATE)]


def build():
    sounds = {}
    sounds["SFX_Click"] = tone(880, 0.06, "square", 0.4, end_freq=1320)
    sounds["SFX_Pop"] = mix(tone(520, 0.09, "sine", 0.6, end_freq=1400), noise(0.05, 0.2, decay=50.0))
    sounds["SFX_Leak"] = tone(330, 0.4, "saw", 0.5, end_freq=110, vibrato=0.03)
    sounds["SFX_Build"] = seq(tone(392, 0.07, "square", 0.4), tone(523, 0.07, "square", 0.4), tone(784, 0.14, "square", 0.4))
    sounds["SFX_Coin"] = seq(tone(988, 0.06, "square", 0.35), tone(1319, 0.16, "square", 0.35))
    sounds["SFX_Arrow"] = mix(noise(0.09, 0.35, decay=28.0, lowpass=0.5, seed=3), tone(900, 0.08, "sine", 0.15, end_freq=300))
    sounds["SFX_Cannon"] = mix(noise(0.35, 0.7, decay=9.0, lowpass=0.12, seed=4), tone(120, 0.3, "sine", 0.6, end_freq=45))
    sounds["SFX_Magic"] = tone(700, 0.16, "sine", 0.4, end_freq=1500, vibrato=0.05)
    sounds["SFX_Boom"] = mix(noise(0.45, 0.8, decay=7.0, lowpass=0.2, seed=5), tone(150, 0.35, "triangle", 0.6, end_freq=50))
    sounds["SFX_Cast"] = mix(tone(300, 0.22, "triangle", 0.5, end_freq=1200), noise(0.2, 0.15, decay=10.0, seed=6))
    sounds["SFX_Reaction"] = mix(seq(tone(660, 0.07, "pulse", 0.4), tone(880, 0.07, "pulse", 0.4), tone(1320, 0.2, "pulse", 0.4, vibrato=0.03)),
                                 noise(0.3, 0.25, decay=8.0, seed=7))
    sounds["SFX_Zap"] = mix(tone(1800, 0.14, "saw", 0.35, end_freq=300), noise(0.12, 0.3, decay=18.0, seed=8))
    sounds["SFX_Hit"] = mix(noise(0.06, 0.5, decay=40.0, lowpass=0.4, seed=9), tone(220, 0.05, "square", 0.2, end_freq=140))
    sounds["SFX_Horn"] = mix(tone(hz("G3"), 0.7, "saw", 0.3, attack=0.05, release=0.25, vibrato=0.01),
                             tone(hz("D4"), 0.7, "saw", 0.25, attack=0.05, release=0.25, vibrato=0.01))
    sounds["SFX_Victory"] = seq(*[tone(hz(n), d, "pulse", 0.4, release=0.05) for n, d in
                                  (("C5", 0.12), ("E5", 0.12), ("G5", 0.12), ("C6", 0.24), ("G5", 0.12), ("C6", 0.5))])
    sounds["SFX_Defeat"] = seq(*[tone(hz(n), d, "triangle", 0.5, release=0.1, vibrato=0.02) for n, d in
                                 (("E4", 0.25), ("D4", 0.25), ("C4", 0.25), ("A3", 0.7))])
    sounds["BGM_Meadow"] = make_bgm()
    return sounds


def main():
    out_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(os.path.abspath(__file__)), "Generated")
    os.makedirs(out_dir, exist_ok=True)
    sounds = build()
    for name, samples in sounds.items():
        save(os.path.join(out_dir, name + ".wav"), samples)
    print("%d sounds -> %s" % (len(sounds), out_dir))


if __name__ == "__main__":
    main()
