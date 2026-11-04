#!/usr/bin/env python3
"""
AetherDSP Python C-ABI Integration Example (Zero External Dependencies)
Uses standard library 'ctypes' and 'wave' to render audio through the C++20 engine.
"""

import ctypes
import os
import sys
import wave
import struct

def find_library():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    candidates = [
        os.path.join(script_dir, "..", "build", "aether-dsp.dll"),
        os.path.join(script_dir, "..", "build", "Release", "aether-dsp.dll"),
        os.path.join(script_dir, "..", "build", "libaether-dsp.so"),
        os.path.join(script_dir, "..", "build", "libaether-dsp.dylib"),
    ]
    for c in candidates:
        if os.path.exists(c):
            return os.path.abspath(c)
    return None

def main():
    print("=== AetherDSP Python C-ABI Bridge ===")
    lib_path = find_library()
    if not lib_path:
        print("[INFO] Shared library not found. Build with CMake to produce aether-dsp shared library.")
        return

    print(f"Loading native library: {lib_path}")
    lib = ctypes.CDLL(lib_path)

    # Function signatures
    lib.aether_engine_create.argtypes = [ctypes.c_uint32]
    lib.aether_engine_create.restype = ctypes.c_void_p

    lib.aether_engine_destroy.argtypes = [ctypes.c_void_p]
    lib.aether_engine_destroy.restype = None

    lib.aether_engine_load_preset.argtypes = [ctypes.c_void_p, ctypes.c_int]
    lib.aether_engine_load_preset.restype = None

    lib.aether_engine_note_on.argtypes = [ctypes.c_void_p, ctypes.c_float, ctypes.c_float, ctypes.c_float]
    lib.aether_engine_note_on.restype = None

    lib.aether_engine_process_interleaved.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_float), ctypes.c_size_t]
    lib.aether_engine_process_interleaved.restype = None

    SAMPLE_RATE = 48000
    DURATION_SEC = 3
    NUM_FRAMES = SAMPLE_RATE * DURATION_SEC

    engine = lib.aether_engine_create(SAMPLE_RATE)
    if not engine:
        print("[ERROR] Failed to instantiate AetherDSP engine.")
        return

    print(f"Engine instantiated. Triggering note on Poincare Conformal Disk...")
    lib.aether_engine_load_preset(engine, 0) # Hyperbolic Cosmic Bell
    lib.aether_engine_note_on(engine, ctypes.c_float(0.35), ctypes.c_float(0.45), ctypes.c_float(0.9))

    out_buffer = (ctypes.c_float * (NUM_FRAMES * 2))()
    print(f"Rendering {DURATION_SEC}s of stereo audio through C++20 AVX2 core...")
    lib.aether_engine_process_interleaved(engine, out_buffer, NUM_FRAMES)

    lib.aether_engine_destroy(engine)

    # Save to 16-bit PCM WAV using Python's standard wave library
    out_wav = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "output", "python_rendered_bell.wav")
    os.makedirs(os.path.dirname(out_wav), exist_ok=True)

    with wave.open(out_wav, 'wb') as wav_file:
        wav_file.setnchannels(2)
        wav_file.setsampwidth(2)
        wav_file.setframerate(SAMPLE_RATE)

        pcm_data = bytearray()
        for i in range(NUM_FRAMES * 2):
            val = max(-1.0, min(1.0, out_buffer[i]))
            s16 = int(val * 32767.0)
            pcm_data.extend(struct.pack('<h', s16))

        wav_file.writeframes(pcm_data)

    print(f"[DONE] Saved rendered WAV to: {out_wav}")

if __name__ == '__main__':
    main()
