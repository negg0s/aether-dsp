import ctypes
from enum import IntEnum
import os
import sys
from typing import List, Tuple, Optional

class Preset(IntEnum):
    HYPERBOLIC_COSMIC_BELL = 0
    CHLADNI_PLATE_GONG = 1
    CHAOTIC_QUANTUM_DRONE = 2
    HYPERBOLIC_AETHER_SPACE = 3
    BOWED_CRYSTAL_PLATE = 4
    VON_KARMAN_TURBULENT_GONG = 5
    PSEUDOSPHERE_TRACTRIX_HORN = 6
    HENON_HEILES_CHAOS = 7
    HYPERBOLIC_STRING_JAWARI = 8
    SINE_GORDON_BREATHER = 9
    ACOUSTIC_BLACK_HOLE = 10
    ACOUSTIC_WORMHOLE = 11
    TOPOLOGICAL_METAMATERIAL = 12
    KDV_SOLITON_LATTICE = 13

class ScalaCatalog:
    SCALES = [
        "12-EDO Equal Temperament",
        "Bohlen-Pierce Scale (13 steps in 3:1)",
        "Wendy Carlos Alpha Scale",
        "Wendy Carlos Beta Scale",
        "Javanese Slendro (5-tone)",
        "Balinese Pelog (7-tone)",
        "Harry Partch 43-Tone Just Intonation",
        "Fibonacci Golden Ratio Scale"
    ]

    @classmethod
    def list_scales(cls) -> List[str]:
        return list(cls.SCALES)

class AetherEngine:
    def __init__(self, sample_rate: int = 48000, lib_path: Optional[str] = None):
        self.sample_rate = sample_rate
        self.lib = self._load_library(lib_path)
        self._setup_c_api()
        self.handle = self.lib.aether_engine_create(sample_rate)
        if not self.handle:
            raise RuntimeError("Failed to allocate AetherDSP engine instance.")

    def _load_library(self, custom_path: Optional[str]):
        if custom_path and os.path.isfile(custom_path):
            return ctypes.CDLL(custom_path)

        candidates = [
            os.path.join(os.path.dirname(__file__), "..", "..", "..", "build", "aether-dsp.dll"),
            os.path.join(os.path.dirname(__file__), "..", "..", "..", "build", "libaether-dsp.so"),
            os.path.join(os.path.dirname(__file__), "..", "..", "..", "build", "libaether-dsp.dylib"),
            "aether-dsp.dll",
            "libaether-dsp.so"
        ]

        for path in candidates:
            if os.path.isfile(path):
                try:
                    return ctypes.CDLL(path)
                except OSError:
                    continue

        raise FileNotFoundError("Could not locate compiled aether-dsp dynamic library (aether-dsp.dll/so).")

    def _setup_c_api(self):
        self.lib.aether_engine_create.argtypes = [ctypes.c_uint32]
        self.lib.aether_engine_create.restype = ctypes.c_void_p

        self.lib.aether_engine_destroy.argtypes = [ctypes.c_void_p]
        self.lib.aether_engine_destroy.restype = None

        self.lib.aether_engine_load_preset.argtypes = [ctypes.c_void_p, ctypes.c_int]
        self.lib.aether_engine_load_preset.restype = None

        self.lib.aether_engine_note_on.argtypes = [ctypes.c_void_p, ctypes.c_float, ctypes.c_float, ctypes.c_float]
        self.lib.aether_engine_note_on.restype = None

        self.lib.aether_engine_note_off.argtypes = [ctypes.c_void_p]
        self.lib.aether_engine_note_off.restype = None

        self.lib.aether_engine_process_interleaved.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_float), ctypes.c_size_t]
        self.lib.aether_engine_process_interleaved.restype = None

    def load_preset(self, preset: Preset):
        self.lib.aether_engine_load_preset(self.handle, int(preset))

    def note_on(self, strike_pos: Tuple[float, float] = (0.3, 0.4), velocity: float = 0.9):
        self.lib.aether_engine_note_on(self.handle, strike_pos[0], strike_pos[1], velocity)

    def note_off(self):
        self.lib.aether_engine_note_off(self.handle)

    def render(self, duration_sec: float = 2.0, block_size: int = 64) -> List[Tuple[float, float]]:
        if duration_sec <= 0:
            return []
        if block_size <= 0:
            raise ValueError("block_size must be greater than 0")
        total_frames = int(duration_sec * self.sample_rate)
        buffer = (ctypes.c_float * (block_size * 2))()
        frames = []

        rendered = 0
        while rendered < total_frames:
            chunk = min(block_size, total_frames - rendered)
            self.lib.aether_engine_process_interleaved(self.handle, buffer, chunk)
            for i in range(chunk):
                frames.append((buffer[i * 2], buffer[i * 2 + 1]))
            rendered += chunk

        return frames

    def __del__(self):
        if hasattr(self, "handle") and self.handle and hasattr(self, "lib") and self.lib:
            self.lib.aether_engine_destroy(self.handle)
            self.handle = None
