# AetherDSP

[![CI](https://github.com/negg0s/aether-dsp/actions/workflows/ci.yml/badge.svg)](https://github.com/negg0s/aether-dsp/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++20](https://img.shields.io/badge/Language-C%2B%2B20-00599C?logo=cplusplus)](https://en.cppreference.com/w/cpp/20)
[![Platform](https://img.shields.io/badge/Platform-Cross--Platform%20%7C%20WebAssembly-brightgreen)](#)
[![Zero-AI](https://img.shields.io/badge/Engine-100%25%20Pure%20Math%20%26%20DSP%20(No%20AI)-orange)](#)

```
    ___        __  __                 ____  _____ ____  
   /   | ___  / /_/ /_  ___  _____   / __ \/ ___// __ \ 
  / /| |/ _ \/ __/ __ \/ _ \/ ___/  / / / /\__ \/ /_/ / 
 / ___ /  __/ /_/ / / /  __/ /     / /_/ /___/ / ____/  
/_/  |_\___/\__/_/ /_/\___/_/     /_____//____/_/       
```

**AetherDSP** is a high-performance, real-time physical modeling sound synthesis engine built in modern C++20. It combines **2D Non-Euclidean Hyperbolic Waveguide Acoustics** (Poincaré conformal disk manifolds) and **2D Kirchhoff-Love Thin Plate Modal Resonators** (Chladni cymatics) into a unified, zero-allocation audio architecture.

Unlike standard physical modeling synthesizers constrained to 1D strings (Karplus-Strong) or rectangular Euclidean rooms, AetherDSP solves acoustic wave propagation on curved Riemannian geometries ($K = -1$) and thin plate biharmonic surfaces, generating shimmering, inharmonic timbres that cannot exist in standard 3D Euclidean space.

---

## Key Features

- **2D Non-Euclidean Hyperbolic Waveguide (Poincaré Disk):**
  Solves the 2D wave equation on a Riemannian manifold with constant negative Gaussian curvature $K = -1$ using Finite-Difference Time-Domain (FDTD). Wavefronts expand exponentially towards the horocycle boundary without modal flutter.
- **2D Modal Cymatics Plate (Kirchhoff-Love Thin Elastic Plate):**
  Calculates the analytical biharmonic eigenmode lattice of vibrating thin plates with customizable stiffness and Poisson ratios. Computes exact nodal lines ($u(x,y) = 0$) where sand accumulates in real-time.
- **Continuous Strange Attractor Modulators:**
  Integrates non-linear 3D dynamical systems (Lorenz, Rössler, Clifford) using 4th-order Runge-Kutta (RK4) to generate evolving organic harmonic perturbations without random number generators or neural networks.
- **Strict Real-Time Safety Guarantees:**
  - **Zero Dynamic Allocations (`malloc`/`new`)** inside the block processing loop.
  - **Lock-Free, Wait-Free SPSC Ring Buffer** with cache-line padding (`hardware_destructive_interference_size`) to prevent false sharing.
  - Header-only core architecture with `noexcept` specifications.
- **Cross-Platform & Web-Ready:**
  Zero OS-specific dependencies (no Win32 or Linux kernel requirements). Compiles on Windows, macOS, Linux, and WebAssembly (WASM). Includes a standalone interactive HTML5/Canvas visualizer.

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

Open [`web/index.html`](web/index.html) directly in any modern browser (or serve via GitHub Pages):

- **Poincaré Waveguide Canvas:** Click anywhere inside the unit disk to strike non-Euclidean space and observe real-time wavefront divergence.
- **Chladni Plate Sand Simulation:** 5,000 live sand particles move dynamically across the vibrating plate, kicked away by antinodes and gathering strictly onto geometric nodal lines.
- **Web Audio Engine:** Real-time modal and chaotic audio synthesis running natively via the Web Audio API.

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

## CLI & Benchmark Usage

Run the compiled executable to display terminal ASCII cymatics, benchmark engine throughput, and render 32-bit float lossless WAV presets:

```bash
./build/aether-dsp
```

### Options
```text
Usage: aether-dsp [options]
  --bench          Run nanosecond-resolution DSP throughput benchmark
  --render         Render lossless audio presets to output/*.wav
  --ascii          Display live terminal ASCII Chladni nodal plate
```

### Rendered Presets (Saved to `output/`)
| Preset | Waveform / Algorithm | Character |
| :--- | :--- | :--- |
| `01_hyperbolic_cosmic_bell.wav` | 2D Poincaré FDTD Waveguide | Ethereal metallic chime with exponential dispersion |
| `02_chladni_plate_gong.wav` | Kirchhoff-Love Thin Plate Modal Lattice | Heavy acoustic gong with rich inharmonic partials |
| `03_chaotic_quantum_drone.wav` | Lorenz Strange Attractor State-Space | Evolving deterministic sub-harmonic drone |
| `04_hyperbolic_aether_space.wav` | Hyperbolic Waveguide + Chaotic Perturbation | Shimmering cosmic acoustic field |

---

## Architecture

```
aether-dsp/
├── include/aether/
│   ├── types.hpp                  # Sample representations, 2D/3D vectors, stereo frames
│   ├── ring_buffer.hpp            # Lock-free, wait-free SPSC ring buffer (cache-line aligned)
│   ├── wav_writer.hpp             # Lossless RIFF WAV exporter (32-bit float / 24-bit PCM)
│   ├── engine.hpp                 # Master engine orchestrator & voice allocator
│   ├── math/
│   │   ├── poincare.hpp           # Riemannian metric, conformal factors, geodesic distance
│   │   └── runge_kutta.hpp        # 4th-order Runge-Kutta ODE numerical solver
│   ├── dsp/
│   │   ├── biquad.hpp             # Direct Form II Transposed biquad filter
│   │   └── envelope.hpp           # Sample-accurate exponential ADSR envelope
│   └── engines/
│       ├── hyperbolic_waveguide.hpp # 2D Non-Euclidean FDTD hyperbolic wave solver
│       ├── chladni_plate.hpp        # Kirchhoff-Love 2D thin plate modal synthesizer
│       └── chaotic_oscillator.hpp   # Lorenz, Rössler & Clifford strange attractors
├── src/
│   └── main.cpp                   # CLI benchmark, terminal cymatics, audio generator
├── tests/
│   └── unit_tests.cpp             # Mathematical invariants and stability test suite
├── web/
│   ├── index.html                 # Standalone interactive browser GUI
│   └── app.js                     # HTML5 Canvas visualizer & Web Audio synthesizer
└── CMakeLists.txt
```

---

## License

This project is licensed under the [MIT License](LICENSE) - Copyright (c) 2026 Emir (negg0s).
