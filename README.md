# AetherDSP

[![CI](https://github.com/negg0s/aether-dsp/actions/workflows/ci.yml/badge.svg)](https://github.com/negg0s/aether-dsp/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++20](https://img.shields.io/badge/Language-C%2B%2B20-00599C?logo=cplusplus)](https://en.cppreference.com/w/cpp/20)
[![SIMD](https://img.shields.io/badge/SIMD-AVX2%20Fused%20Multiply--Add-blueviolet)](#)
[![Python Package](https://img.shields.io/badge/Python-Package%20%26%20ctypes%20C--ABI-blue?logo=python)](bindings/python)
[![Rust Crate](https://img.shields.io/badge/Rust-FFI%20Crate-orange?logo=rust)](bindings/rust)
[![CLAP](https://img.shields.io/badge/Plugin-CLAP%20Native%20Standard-red)](#)
[![Audio](https://img.shields.io/badge/Audio-WASAPI%20%7C%20CoreAudio%20%7C%20ALSA%20%7C%20WebAudio-success)](#)
[![MIDI](https://img.shields.io/badge/MIDI-SMF%20Format%200%2F1-yellow)](#)
[![Tests](https://img.shields.io/badge/Tests-31%20Invariant%20Tests%20Passed-brightgreen)](#)
[![Zero-AI](https://img.shields.io/badge/Engine-100%25%20Pure%20Math%20%26%20DSP%20(No%20AI)-orange)](#)

```
    ___        __  __                 ____  _____ ____  
   /   | ___  / /_/ /_  ___  _____   / __ \/ ___// __ \ 
  / /| |/ _ \/ __/ __ \/ _ \/ ___/  / / / /\__ \/ /_/ / 
 / ___ /  __/ /_/ / / /  __/ /     / /_/ /___/ / ____/  
/_/  |_\___/\__/_/ /_/\___/_/     /_____//____/_/       
```

**AetherDSP** is an ultra-high-performance physical modeling sound synthesis engine built in modern C++20 with AVX2 SIMD acceleration. It combines **2D Non-Euclidean Hyperbolic Waveguide Acoustics** (Poincaré conformal disk manifolds), **2D Kirchhoff-Love Thin Plate Modal Resonators** (Chladni cymatics), **3D Woodworth Binaural Spatialization**, **Microtonal Tuning Systems** (Bohlen-Pierce), and **Unitary Hyperbolic FDN Reverb** into a unified, zero-allocation real-time audio architecture.

Unlike standard physical modeling synthesizers constrained to 1D strings (Karplus-Strong) or rectangular Euclidean rooms, AetherDSP solves acoustic wave propagation on curved Riemannian geometries ($K = -1$) and thin plate biharmonic surfaces, generating shimmering, inharmonic timbres that cannot exist in standard 3D Euclidean space.

---

## Key Highlights

- **2D Non-Euclidean Hyperbolic Waveguide (Poincaré Disk):**
  Solves the 2D wave equation on a Riemannian manifold with constant negative Gaussian curvature $K = -1$ using Finite-Difference Time-Domain (FDTD). Wavefronts expand exponentially towards the horocycle boundary without modal flutter.
- **Nonlinear Föppl-von Kármán Large-Deflection Plate Resonator:**
  Physical modeling of large-deflection thin plate dynamics with cubic Duffing restoring forces $\beta_k q_k^3$ and inter-modal energy cascading $\sum_j \Gamma_{kj} q_k q_j^2$, recreating the iconic shimmering crescendo and amplitude-dependent pitch glide of orchestral gongs and tam-tams.
- **Poincaré Upper Half-Plane ($\mathbb{H}^2$) & Modular Group $\text{SL}(2, \mathbb{Z})$ Möbius Isometries:**
  Implements Cayley transforms $\mathbb{D} \leftrightarrow \mathbb{H}^2$ and Möbius transformation matrices for non-harmonic acoustic diffusion delay lines with zero comb filtering.
- **Hamiltonian Geodesic Sound Ray Tracer & Manifold Convolution Reverb:**
  Integrates Christoffel geodesic equations $\ddot{x}^i + \Gamma^i_{jk}\dot{x}^j\dot{x}^k = 0$ using 4th-order Runge-Kutta numerical integration to trace acoustic rays along curved hyperbolic geodesics and synthesize Manifold Impulse Responses (MIR).
- **AVX2 SIMD Vectorized FDTD Kernel:**
  Processes 8 spatial grid cells per instruction using 8-wide vectorized fused multiply-add (`_mm256_fmadd_ps`) operations.
- **Beltrami Pseudosphere Tractrix Horn Resonator:**
  Acoustic wave propagation inside an axisymmetric surface of constant negative Gaussian curvature $K = -1$. Solves the Webster Horn Equation with tractrix cross-sectional area $S(z) = \pi r^2 \operatorname{sech}^2(u)$, modeling brass acoustic flares and cutoff transmission frequencies without mouth reflections.
- **4th-Order Symplectic Yoshida Integrator:**
  Phase-space preserving geometric numerical integrator ($O(\Delta t^4)$ global accuracy) with Yoshida sub-step constants ($w_1 = \frac{1}{2 - 2^{1/3}}$), ensuring exact conservation of the shadow Hamiltonian and zero energy drift over millions of samples.
- **Hénon-Heiles Galactic Gravitational Chaos Oscillator:**
  Models non-integrable stellar orbits in an axisymmetric galactic gravitational potential $V(x,y) = \frac{1}{2}(x^2+y^2) + x^2 y - \frac{1}{3}y^3$. Accurately renders chaotic orbits and Kolmogorov–Arnold–Moser (KAM) invariant tori.
- **8-Voice Zero-Allocation Polyphonic Voice Manager:**
  Real-time polyphonic voice allocation with deterministic Least-Recently-Used (LRU) voice stealing, microtonal scale mapping, and soft dynamic headroom normalization for dense chords.
- **Nonlinear Stiff Kirchhoff String with Hyperbolic Jawari Bridge:**
  1D FDTD solver for the nonlinear Kirchhoff wave equation with dynamic tension modulation and bilateral unilateral contact with a curved hyperbolic bridge obstacle (the iconic buzzing sitar/tanpura "Jawari" harmonic bloom).
- **3D Non-Euclidean Poincaré Ball ($\mathbb{H}^3$) Acoustic Spatializer:**
  Sound propagation on a 3D Riemannian manifold of constant negative Gaussian curvature $K = -1$. Models exponential acoustic wavefront expansion ($I \propto 1 / \sinh^2(d_H)$) and Woodworth spherical head binaural shadowing.
- **In-Place Radix-2 Cooley-Tukey FFT & Spectral Power Analyzer:**
  Zero-allocation $O(N \log N)$ Fast Fourier Transform engine with in-place bit-reversal permutation, Hann/Hamming/Blackman-Harris windowing, and single-sided power spectral density calculation.
- **Terminal Unicode Braille Phase Space & 2D Spectrogram:**
  High-resolution sub-pixel graphics engine inside standard terminals using Unicode Braille Patterns (U+2800..U+28FF) for 30 FPS rolling oscilloscopes, dynamic TrueColor stereo VU meters (`--play <wav>`), 2D Time-Frequency spectral waterfalls (`--spectrogram <wav>`), and 2D symplectic phase space orbits (`--phase-space`).
- **Stribeck Stick-Slip Non-Linear Friction Exciter:**
  Physically models continuous friction bowing (crystal singing bowls, glass harmonicas) with static/kinetic friction transitions and velocity thresholding.
- **3D Binaural HRTF Spatializer (Woodworth Spherical Head Model):**
  Calculates true Interaural Time Difference (ITD: $\Delta t = \frac{a}{c}(\theta + \sin\theta)$) and Interaural Level Difference (ILD) with frequency-dependent head shadowing, allowing sounds to orbit 360-degrees around the listener's head.
- **2D Modal Cymatics Plate (Kirchhoff-Love Thin Elastic Plate):**
  Calculates the analytical biharmonic eigenmode lattice of vibrating thin plates with customizable stiffness and Poisson ratios. Computes exact nodal lines ($u(x,y) = 0$) where sand accumulates in real-time.
- **High-Resolution SVG Vector Graphics Exporter:**
  Exports mathematically exact Chladni nodal curves and Poincaré $\{7, 3\}$ reflection group tessellations to crisp, scalable `.svg` vector files.
- **Microtonal Tuning Engine & Scala Catalog:**
  Embedded library of 8 exotic scales (Bohlen-Pierce 13-EDT, Wendy Carlos Alpha & Beta, Javanese Slendro, Balinese Pelog, Harry Partch 43-tone, Fibonacci Golden Ratio) plus full `.scl` file parsing.
- **Hyperbolic Feedback Delay Network (FDN) Reverb:**
  4-channel unitary Householder feedback matrix with mutually prime delay lines derived from the $\{7, 3\}$ hyperbolic heptagonal tessellation.
- **Standard MIDI File (SMF 0/1) Engine:**
  Zero-dependency binary parser and serializer for `.mid` files. Export and sequence scores directly into digital audio workstations (DAWs).
- **Python Package & Rust FFI Crates:**
  Full pip-installable Python package (`bindings/python/`) with typed classes and pure-Python spectral analyzer, alongside a complete Rust crate (`bindings/rust/`).
- **CLAP (CLever Audio Plugin) Architecture:**
  Standard-compliant native CLAP plugin definitions for DAW hosting (Bitwig, Reaper, FL Studio).
- **Live Real-Time Audio Hardware Streaming:**
  Low-latency hardware audio streaming via `miniaudio` (WASAPI, CoreAudio, ALSA, PulseAudio, WebAudio).
- **Interactive 3D Web Visualizer:**
  Standalone browser app featuring 3D isometric plate topography, 6,000 sand particle cymatics physics, Poincaré disk ripples, and a real-time FFT spectrum & oscilloscope.

---

## Mathematical Foundations

### 1. Non-Euclidean Hyperbolic Wave Equation
In the Poincaré conformal disk model $\mathbb{D} = \{z \in \mathbb{C} : |z| < 1\}$, the Riemannian metric tensor is given by:

$$ds^2 = \frac{4 (dx^2 + dy^2)}{(1 - (x^2 + y^2))^2}$$

The Laplace-Beltrami operator $\Delta_H$ scales the Euclidean Laplacian $\nabla^2$ by the inverse conformal factor squared:

$$\Delta_H u = \frac{(1 - r^2)^2}{4} \left( \frac{\partial^2 u}{\partial x^2} + \frac{\partial^2 u}{\partial y^2} \right)$$

The wave equation with dissipation $\gamma$:

$$\frac{\partial^2 u}{\partial t^2} = c^2 \Delta_H u - \gamma \frac{\partial u}{\partial t}$$

Because the distance to the boundary $r \to 1$ diverges asymptotically according to the geodesic distance:

$$d_H(z_1, z_2) = \operatorname{arcosh}\left( 1 + \frac{2|z_1 - z_2|^2}{(1 - |z_1|^2)(1 - |z_2|^2)} \right)$$

Acoustic reflections disperse into fractal horocyclic wavefronts, completely eliminating harmonic room resonances.

### 2. 2D Kirchhoff-Love Thin Plate Biharmonic Equation
The transverse vibrations $w(x,y,t)$ of a thin elastic plate are governed by:

$$D \nabla^4 w + \rho h \frac{\partial^2 w}{\partial t^2} = 0$$

Where $D = \frac{E h^3}{12(1 - \nu^2)}$ is the flexural rigidity. The resulting eigenmode frequencies scale inharmonically:

$$f_{m,n} \propto \left( \frac{m^2}{L_x^2} + \frac{n^2}{L_y^2} \right)$$

Nodal lines occur where:

$$w_{m,n}(x,y) = \cos\left(\frac{m\pi x}{L}\right)\cos\left(\frac{n\pi y}{L}\right) - \cos\left(\frac{n\pi x}{L}\right)\cos\left(\frac{m\pi y}{L}\right) = 0$$

---

## Building from Source

### Prerequisites
- C++20 compliant compiler (MSVC 2022/2026, Clang 16+, GCC 13+)
- CMake 3.20+
- Ninja (optional, recommended)

### Quick Build (CMake & Ninja)
```bash
# Clone repository
git clone https://github.com/negg0s/aether-dsp.git
cd aether-dsp

# Configure
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# Build targets (CLI synthesizer, shared C-ABI library & test suite)
cmake --build build --config Release

# Run complete 13-test invariant suite
ctest --test-dir build --output-on-failure
```

---

## Python Integration (C-ABI Bridge)

Run the zero-dependency Python script to generate audio through the native C++20 engine:

```bash
python examples/python_demo.py
```

```python
import ctypes

lib = ctypes.CDLL("./build/aether-dsp.dll")
engine = lib.aether_engine_create(48000)
lib.aether_engine_load_preset(engine, 0) # Hyperbolic Cosmic Bell
lib.aether_engine_note_on(engine, ctypes.c_float(0.35), ctypes.c_float(0.45), ctypes.c_float(0.9))

buffer = (ctypes.c_float * (48000 * 2))()
lib.aether_engine_process_interleaved(engine, buffer, 48000)
lib.aether_engine_destroy(engine)
```

---

## CLI Usage

### Interactive Live Synthesizer (Audio Hardware Playback)
Stream directly to your speakers or headphones with sub-millisecond latency:
```bash
./build/aether-dsp --live
```
```text
Play with keyboard:
  Notes     : [A] [W] [S] [E] [D] [F] [T] [G] [Y] [H] [U] [J] [K]
  Presets   : [1] Bell   [2] Gong   [3] Drone   [4] Aether
  Quit      : [Q] or [ESC]
```

### External Audio Resynthesis
Drive the Poincaré hyperbolic waveguide with any external audio sample:
```bash
./build/aether-dsp --resynthesize input.wav output_resynthesized.wav
```

### Benchmark & Batch WAV/MIDI Rendering
```bash
./build/aether-dsp
```

### Rendered Files (Saved to `output/`)
| File | Format | Character |
| :--- | :--- | :--- |
| `01_hyperbolic_cosmic_bell.wav` | 32-bit Float WAV | Ethereal metallic chime with exponential dispersion |
| `02_chladni_plate_gong.wav` | 32-bit Float WAV | Heavy acoustic gong with rich inharmonic partials |
| `03_chaotic_quantum_drone.wav` | 32-bit Float WAV | Evolving deterministic sub-harmonic drone |
| `04_hyperbolic_aether_space.wav` | 32-bit Float WAV | Shimmering cosmic acoustic field |
| `05_fibonacci_non_euclidean_sonata.wav` | 32-bit Float WAV | Algorithmic polyphonic sonata based on Fibonacci & Xenakis sieve |
| `06_binaural_3d_orbit_demo.wav` | 32-bit Float WAV | 3D Woodworth spherical head model binaural 360-degree orbit |
| `07_hyperbolic_resynthesized_gong.wav` | 32-bit Float WAV | Acoustic resynthesis driven by external audio excitation |
| `08_bowed_crystal_plate.wav` | 32-bit Float WAV | Continuous Stribeck non-linear stick-slip friction crystal bowing |
| `09_hyperbolic_geodesic_reverb.wav` | 32-bit Float WAV | Hamiltonian geodesic sound ray-traced convolution reverb |
| `10_von_karman_turbulent_gong.wav` | 32-bit Float WAV | Large-deflection nonlinear modal coupling with shimmering crescendo |
| `11_pseudosphere_tractrix_horn.wav` | 32-bit Float WAV | Beltrami Pseudosphere tractrix horn Webster acoustic transmission flare |
| `12_henon_heiles_galactic_chaos.wav` | 32-bit Float WAV | Astrophysical galactic Hamiltonian chaos integrated with 4th-order Yoshida method |
| `13_polyphonic_microtonal_cathedral.wav` | 32-bit Float WAV | 8-Voice polyphonic Bohlen-Pierce microtonal chord progression on Chladni resonators |
| `14_sitar_hyperbolic_jawari.wav` | 32-bit Float WAV | Nonlinear stiff Kirchhoff string with dynamic tension modulation & buzzing Jawari bridge |
| `15_hyperbolic_ball_3d_spatial.wav` | 32-bit Float WAV | 3D Poincaré Ball (H^3) non-Euclidean binaural orbit with exponential horizon dropoff |
| `fibonacci_score.mid` | Standard MIDI Format 1 | Exported polyphonic score ready for DAW import (Ableton/FL Studio) |
| `chladni_nodal_lines.svg` | Scalable Vector Graphics | 800x800 high-res vector visualization of Chladni nodal plate |
| `poincare_tiling.svg` | Scalable Vector Graphics | 800x800 high-res vector art of Poincaré {7,3} hyperbolic tessellation |

---

## Architecture

```
aether-dsp/
├── include/aether/
│   ├── types.hpp                  # Sample representations, 2D/3D vectors, stereo frames
│   ├── c_api.h                    # C99/C++ extern "C" ABI export declarations
│   ├── ring_buffer.hpp            # Lock-free, wait-free SPSC ring buffer (cache-line aligned)
│   ├── wav_writer.hpp             # Lossless RIFF WAV exporter (32-bit float / 24-bit PCM)
│   ├── wav_reader.hpp             # Zero-dependency RIFF WAV reader & decoder
│   ├── svg_exporter.hpp           # Vector graphics (.svg) exporter for Chladni curves & tilings
│   ├── preset_manager.hpp         # Human-readable .aether preset file parser & serializer
│   ├── engine.hpp                 # Master engine orchestrator, voices & FDN reverb
│   ├── polyphony.hpp              # 8-Voice zero-allocation polyphonic voice pool & LRU stealing
│   ├── ui/
│   │   └── terminal_visualizer.hpp # Unicode Braille sub-pixel waveform & Poincaré disk canvas
│   ├── audio/
│   │   ├── miniaudio.h            # Single-header cross-platform audio hardware driver
│   │   └── playback_device.hpp    # Low-latency audio streaming wrapper
│   ├── midi/
│   │   └── midi_file.hpp          # Standard MIDI File (SMF 0/1) binary parser & exporter
│   ├── plugin/
│   │   └── clap_entry.hpp         # CLAP (CLever Audio Plugin) native standard definitions
│   ├── math/
│   │   ├── poincare.hpp           # Riemannian metric, conformal factors, geodesic distance
│   │   ├── runge_kutta.hpp        # 4th-order Runge-Kutta ODE numerical solver
│   │   ├── fft.hpp                # Radix-2 Cooley-Tukey FFT & spectral power estimation
│   │   ├── hyperbolic_tiling.hpp  # Non-Euclidean {7, 3} & {5, 4} tessellation generator
│   │   ├── hyperbolic_halfplane.hpp # Upper Half-Plane H^2, Cayley map & SL(2, Z) Mobius
│   │   ├── geodesic_raytracer.hpp # Hamiltonian geodesic sound ray tracer on curved manifolds
│   │   ├── yoshida_integrator.hpp # 4th-order symplectic phase-space volume preserving integrator
│   │   └── algorithmic_composer.hpp # Xenakis Sieve & Fibonacci score generator
│   ├── dsp/
│   │   ├── biquad.hpp             # Direct Form II Transposed biquad filter
│   │   ├── envelope.hpp           # Sample-accurate exponential ADSR envelope
│   │   ├── tuning.hpp             # Microtonal tuning engine (Bohlen-Pierce & Scala .scl)
│   │   ├── scala_catalog.hpp      # Bank of 8 historical & mathematical microtonal scales
│   │   └── fx/
│   │       ├── hyperbolic_reverb.hpp # 4-channel unitary Householder FDN reverb
│   │       ├── convolution_reverb.hpp# Time-domain convolution for Manifold Impulse Responses
│   │       ├── binaural_panner.hpp   # 3D Woodworth spherical head binaural spatializer
│   │       └── hyperbolic_spatializer_3d.hpp # 3D Poincare Ball (H^3) acoustic spatializer
│   └── engines/
│       ├── hyperbolic_waveguide.hpp # AVX2 SIMD Non-Euclidean FDTD hyperbolic wave solver
│       ├── chladni_plate.hpp        # Kirchhoff-Love 2D thin plate modal synthesizer
│       ├── von_karman_plate.hpp     # Nonlinear large-deflection plate with shimmering crescendo
│       ├── pseudosphere_horn.hpp    # Beltrami pseudosphere tractrix Webster acoustic horn
│       ├── hyperbolic_string.hpp    # Nonlinear stiff string with tension modulation & Jawari bridge
│       ├── stick_slip_exciter.hpp   # Stribeck non-linear friction stick-slip continuous exciter
│       ├── chaotic_oscillator.hpp   # Lorenz, Rössler & Clifford strange attractors
│       └── hamiltonian_chaos.hpp    # Hénon-Heiles galactic gravitational chaos oscillator
├── bindings/
│   ├── python/                    # Pip-installable Python package (pyproject.toml, setup.py)
│   └── rust/                      # Complete Rust crate with C-ABI bindings (Cargo.toml)
├── src/
│   ├── main.cpp                   # CLI dashboard, live audio, Braille visualizer & benchmarks
│   ├── audio_device.cpp           # Miniaudio hardware backend compilation unit
│   └── c_api.cpp                  # C-ABI implementation
├── tests/
│   └── unit_tests.cpp             # 31 extended mathematical invariants & audio stability tests
├── examples/
│   ├── python_demo.py             # Python ctypes zero-dependency integration demo
│   └── spectral_analysis.py       # Pure-Python physical inharmonicity & DFT spectral analyzer
├── scripts/
│   ├── build_msvc.bat             # Fast MSVC 2026 build script
│   ├── serve_web.py               # Zero-dependency local server for 3D Web visualizer
│   ├── build_wasm.bat             # Emscripten WebAssembly build script (Windows)
│   └── build_wasm.sh              # Emscripten WebAssembly build script (Linux/macOS)
├── presets/
│   ├── cosmic_bell.aether         # Poincaré disk hyperbolic resonator preset
│   ├── singing_bowl.aether        # Stribeck stick-slip friction singing bowl preset
│   └── tibetan_gong.aether        # Kirchhoff-Love thin elastic plate gong preset
├── web/
│   ├── index.html                 # Standalone interactive browser GUI & 3D wireframe
│   └── app.js                     # 3D Canvas visualizer, sand physics & Web Audio synth
└── CMakeLists.txt
```

---

## License

This project is licensed under the [MIT License](LICENSE) - Copyright (c) 2026 Emir (negg0s).
