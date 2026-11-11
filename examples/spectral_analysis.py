#!/usr/bin/env python3
"""
AetherDSP Spectral Analysis Tool
================================
Analyzes the physical inharmonicity and spectral distribution of
rendered non-Euclidean hyperbolic waveguide and Chladni plate audio.
"""

import wave
import struct
import math
import os
import sys

def read_riff_wav(filename: str):
    with open(filename, 'rb') as f:
        data = f.read()

    if data[:4] != b'RIFF' or data[8:12] != b'WAVE':
        raise ValueError("Not a valid RIFF/WAVE file")

    idx = 12
    n_channels = 2
    framerate = 48000
    format_tag = 3
    sampwidth = 4
    samples = []

    while idx < len(data) - 8:
        chunk_id = data[idx:idx+4]
        chunk_size = struct.unpack('<I', data[idx+4:idx+8])[0]
        chunk_data = data[idx+8:idx+8+chunk_size]
        idx += 8 + chunk_size
        if chunk_size % 2 == 1:
            idx += 1  # RIFF word alignment

        if chunk_id == b'fmt ':
            format_tag, n_channels, framerate, _, _, bits_per_sample = struct.unpack('<HHIIHH', chunk_data[:16])
            sampwidth = bits_per_sample // 8
        elif chunk_id == b'data':
            if format_tag == 3 and sampwidth == 4:
                count = len(chunk_data) // 4
                floats = struct.unpack(f'<{count}f', chunk_data)
                if n_channels == 2:
                    samples = [(floats[i] + floats[i+1]) * 0.5 for i in range(0, count - 1, 2)]
                else:
                    samples = list(floats)
            elif format_tag == 1 and sampwidth == 2:
                count = len(chunk_data) // 2
                shorts = struct.unpack(f'<{count}h', chunk_data)
                if n_channels == 2:
                    samples = [(shorts[i] + shorts[i+1]) / (2.0 * 32768.0) for i in range(0, count - 1, 2)]
                else:
                    samples = [s / 32768.0 for s in shorts]

    return samples, framerate, n_channels, sampwidth, format_tag

def analyze_wav(filename: str):
    if not os.path.isfile(filename):
        print(f"[ERROR] File not found: {filename}")
        return

    print(f"\n\033[1;36m=== AetherDSP Physical Inharmonic Spectral Analysis: {os.path.basename(filename)} ===\033[0m")
    try:
        samples, framerate, n_channels, sampwidth, format_tag = read_riff_wav(filename)
    except Exception as e:
        print(f"[ERROR] Failed to decode WAV: {e}")
        return

    fmt_name = "IEEE 32-bit Float" if format_tag == 3 else "PCM"
    print(f"Format: {n_channels} channels | {framerate} Hz | {sampwidth * 8}-bit ({fmt_name})")

    if not samples:
        print("[ERROR] No audio samples extracted.")
        return

    # Windowed FFT (using pure python Cooley-Tukey or DFT on N=2048)
    N = 2048
    if len(samples) < N:
        samples.extend([0.0] * (N - len(samples)))
    
    # Hann window
    windowed = [samples[i] * 0.5 * (1.0 - math.cos(2.0 * math.pi * i / (N - 1))) for i in range(N)]

    if framerate <= 0:
        print('[ERROR] Invalid framerate.')
        return

    # Compute DFT magnitudes for first 512 frequency bins
    bin_width = framerate / N
    if bin_width <= 0:
        print('[ERROR] Invalid bin width.')
        return
    magnitudes = []
    for k in range(N // 4):
        omega = 2.0 * math.pi * k / N
        real = sum(windowed[n] * math.cos(omega * n) for n in range(0, N, 2))
        imag = sum(-windowed[n] * math.sin(omega * n) for n in range(0, N, 2))
        mag = math.sqrt(real * real + imag * imag)
        magnitudes.append((k * bin_width, mag))

    # Find top 6 spectral peaks
    sorted_peaks = sorted(magnitudes, key=lambda x: x[1], reverse=True)
    top_peaks = []
    for freq, mag in sorted_peaks:
        if freq < 40.0: continue
        if any(abs(freq - tf) < 30.0 for tf, _ in top_peaks): continue
        top_peaks.append((freq, mag))
        if len(top_peaks) >= 6: break

    top_peaks.sort(key=lambda x: x[0])
    fund = top_peaks[0][0] if top_peaks else 1.0

    print("\n\033[1m[Prominent Eigenmode Spectral Peaks]:\033[0m")
    print(f"| Mode # | Frequency (Hz) | Ratio (f / f0) | Theoretical Origin |")
    print(f"| :---:  | :---:          | :---:          | :---               |")
    for i, (freq, mag) in enumerate(top_peaks):
        ratio = freq / fund
        origin = "Fundamental Mode" if i == 0 else "Non-Euclidean Bessel Mode" if abs(ratio - 1.593) < 0.2 else "Chladni Modal Lattice"
        print(f"| {i+1}      | {freq:8.1f} Hz    | {ratio:6.3f}x        | {origin} |")

    # ASCII Spectrum Bar Visualization
    print("\n\033[1;33m[Spectral Power Distribution (0 - 4000 Hz)]:\033[0m")
    max_m = max((m for f, m in magnitudes[:int(4000 / bin_width)]), default=1.0)
    step_hz = 120.0
    for bar_f in range(100, 3600, int(step_hz)):
        bin_idx = int(bar_f / bin_width)
        val = magnitudes[bin_idx][1] / (max_m if max_m > 0 else 1.0)
        bars = int(val * 45)
        print(f"  {bar_f:4d} Hz | \033[36m{'#' * bars}\033[0m")
    print()

if __name__ == "__main__":
    test_file = sys.argv[1] if len(sys.argv) > 1 else "output/01_hyperbolic_cosmic_bell.wav"
    analyze_wav(test_file)
