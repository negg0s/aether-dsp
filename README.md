# AetherDSP

[![CI](https://github.com/negg0s/aether-dsp/actions/workflows/ci.yml/badge.svg)](https://github.com/negg0s/aether-dsp/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++20](https://img.shields.io/badge/Language-C%2B%2B20-00599C?logo=cplusplus)](https://en.cppreference.com/w/cpp/20)
[![SIMD](https://img.shields.io/badge/SIMD-AVX2%20Fused%20Multiply--Add-blueviolet)](#)
[![Audio](https://img.shields.io/badge/Audio-WASAPI%20%7C%20CoreAudio%20%7C%20ALSA%20%7C%20WebAudio-success)](#)
[![MIDI](https://img.shields.io/badge/MIDI-SMF%20Format%200%2F1-yellow)](#)
[![Zero-AI](https://img.shields.io/badge/Engine-100%25%20Pure%20Math%20%26%20DSP%20(No%20AI)-orange)](#)

```
    ___        __  __                 ____  _____ ____  
   /   | ___  / /_/ /_  ___  _____   / __ \/ ___// __ \ 
  / /| |/ _ \/ __/ __ \/ _ \/ ___/  / / / /\__ \/ /_/ / 
 / ___ /  __/ /_/ / / /  __/ /     / /_/ /___/ / ____/  
/_/  |_\___/\__/_/ /_/\___/_/     /_____//____/_/       
```

**AetherDSP** is an ultra-high-performance, physical modeling sound synthesis engine built in modern C++20 with AVX2 SIMD acceleration. It combines **2D Non-Euclidean Hyperbolic Waveguide Acoustics** (Poincaré conformal disk manifolds), **2D Kirchhoff-Love Thin Plate Modal Resonators** (Chladni cymatics), **Microtonal Tuning Systems** (Bohlen-Pierce), and **Unitary Hyperbolic FDN Reverb** into a unified, zero-allocation real-time audio architecture.

Unlike standard physical modeling synthesizers constrained to 1D strings (Karplus-Strong) or rectangular Euclidean rooms, AetherDSP solves acoustic wave propagation on curved Riemannian geometries ($K = -1$) and thin plate biharmonic surfaces, generating shimmering, inharmonic timbres that cannot exist in standard 3D Euclidean space.

---

## Key Highlights

- **2D Non-Euclidean Hyperbolic Waveguide (Poincaré Disk):**
  Solves the 2D wave equation on a Riemannian manifold with constant negative Gaussian curvature $K = -1$ using Finite-Difference Time-Domain (FDTD). Wavefronts expand exponentially towards the horocycle boundary without modal flutter.
- **AVX2 SIMD Vectorized FDTD Kernel:**
  Processes 8 spatial grid cells per instruction using 8-wide vectorized fused multiply-add (`_mm256_fmadd_ps`) operations.
- **2D Modal Cymatics Plate (Kirchhoff-Love Thin Elastic Plate):**
  Calculates the analytical biharmonic eigenmode lattice of vibrating thin plates with customizable stiffness and Poisson ratios. Computes exact nodal lines ($u(x,y) = 0$) where sand accumulates in real-time.
- **Microtonal Tuning Engine (Bohlen-Pierce & Scala .scl Support):**
  Non-Euclidean scales dividing the tritave (3:1) into 13 equal steps, perfectly matching the inharmonic mode distribution of 2D circular/square plates. Includes full Scala `.scl` file parsing.
- **Hyperbolic Feedback Delay Network (FDN) Reverb:**
  4-channel unitary Householder feedback matrix with mutually prime delay lines derived from the $\{7, 3\}$ hyperbolic heptagonal tessellation.
- **Standard MIDI File (SMF 0/1) Engine:**
  Zero-dependency binary parser and serializer for `.mid` files. Export and sequence scores directly into digital audio workstations (DAWs).
- **Live Real-Time Audio Hardware Streaming:**
  Low-latency hardware audio device streaming via `miniaudio` (WASAPI, CoreAudio, ALSA, PulseAudio, WebAudio).
- **Strict Real-Time Safety Guarantees:**
  - **Zero Dynamic Allocations (`malloc`/`new`)** inside the block processing loop.
  - **Lock-Free, Wait-Free SPSC Ring Buffer** with cache-line padding (`hardware_destructive_interference_size`) to prevent false sharing.
  - Header-only core architecture with `noexcept` specifications.
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

## Live Interactive Web Visualizer

Open [`web/index.html`](web/index.html) directly in any modern browser:

- **Poincaré Waveguide Canvas:** Click anywhere inside the unit disk to strike non-Euclidean space and observe real-time wavefront divergence.
- **Chladni Plate Sand Simulation:** 6,000 live sand particles move dynamically across the vibrating plate, kicked away by antinodes and gathering strictly onto geometric nodal lines.
- **3D Isometric Heightfield:** Live 3D perspective wireframe showing the physical deformation of the vibrating plate.
- **Web Audio Engine & FFT Scope:** Real-time modal and chaotic audio synthesis running natively via Web Audio API with a live oscilloscope and frequency spectrum analyzer.
- **Virtual Piano & Computer Hotkeys:** Play notes using computer keys (`A`, `W`, `S`, `E`, `D`, `F`, `T`, `G`, `Y`, `H`, `U`, `J`, `K`).

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

# Build targets (CLI synthesizer & test suite)
cmake --build build --config Release

# Run unit tests
ctest --test-dir build --output-on-failure
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
| `fibonacci_score.mid` | Standard MIDI Format 1 | Exported polyphonic score ready for DAW import (Ableton/FL Studio) |

---

## Architecture

```
aether-dsp/
├── include/aether/
│   ├── types.hpp                  # Sample representations, 2D/3D vectors, stereo frames
│   ├── ring_buffer.hpp            # Lock-free, wait-free SPSC ring buffer (cache-line aligned)
│   ├── wav_writer.hpp             # Lossless RIFF WAV exporter (32-bit float / 24-bit PCM)
│   ├── engine.hpp                 # Master engine orchestrator, voices & FDN reverb
│   ├── audio/
│   │   ├── miniaudio.h            # Single-header cross-platform audio hardware driver
│   │   └── playback_device.hpp    # Low-latency audio streaming wrapper
│   ├── midi/
│   │   └── midi_file.hpp          # Standard MIDI File (SMF 0/1) binary parser & exporter
│   ├── math/
│   │   ├── poincare.hpp           # Riemannian metric, conformal factors, geodesic distance
│   │   ├── runge_kutta.hpp        # 4th-order Runge-Kutta ODE numerical solver
│   │   └── algorithmic_composer.hpp # Xenakis Sieve & Fibonacci score generator
│   ├── dsp/
│   │   ├── biquad.hpp             # Direct Form II Transposed biquad filter
│   │   ├── envelope.hpp           # Sample-accurate exponential ADSR envelope
│   │   ├── tuning.hpp             # Microtonal tuning engine (Bohlen-Pierce & Scala .scl)
│   │   └── fx/
│   │       └── hyperbolic_reverb.hpp # 4-channel unitary Householder FDN reverb
│   └── engines/
│       ├── hyperbolic_waveguide.hpp # AVX2 SIMD Non-Euclidean FDTD hyperbolic wave solver
│       ├── chladni_plate.hpp        # Kirchhoff-Love 2D thin plate modal synthesizer
│       └── chaotic_oscillator.hpp   # Lorenz, Rössler & Clifford strange attractors
├── src/
│   ├── main.cpp                   # CLI benchmark, live hardware audio, MIDI exporter
│   └── audio_device.cpp           # Miniaudio hardware backend compilation unit
├── tests/
│   └── unit_tests.cpp             # 8 extended mathematical invariants & audio stability tests
├── web/
│   ├── index.html                 # Standalone interactive browser GUI & 3D wireframe
│   └── app.js                     # 3D Canvas visualizer, sand physics & Web Audio synth
└── CMakeLists.txt
```

---

## License

This project is licensed under the [MIT License](LICENSE) - Copyright (c) 2026 Emir (negg0s).
