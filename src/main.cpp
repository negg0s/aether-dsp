#include "aether/engine.hpp"
#include "aether/wav_writer.hpp"
#include "aether/wav_reader.hpp"
#include "aether/svg_exporter.hpp"
#include "aether/preset_manager.hpp"
#include "aether/math/algorithmic_composer.hpp"
#include "aether/math/hyperbolic_tiling.hpp"
#include "aether/math/hyperbolic_halfplane.hpp"
#include "aether/math/geodesic_raytracer.hpp"
#include "aether/engines/von_karman_plate.hpp"
#include "aether/engines/pseudosphere_horn.hpp"
#include "aether/math/yoshida_integrator.hpp"
#include "aether/dsp/fx/convolution_reverb.hpp"
#include "aether/ui/terminal_visualizer.hpp"
#include "aether/audio/playback_device.hpp"
#include "aether/midi/midi_file.hpp"
#include "aether/dsp/tuning.hpp"
#include "aether/dsp/scala_catalog.hpp"
#include "aether/dsp/fx/binaural_panner.hpp"
#include "aether/engines/hamiltonian_chaos.hpp"
#include "aether/polyphony.hpp"
#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <iomanip>
#include <filesystem>
#include <cmath>
#include <thread>
#include <array>
#include <atomic>

#if defined(_WIN32)
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
int _getch() { return getchar(); }
int _kbhit() { return 1; }
#endif

namespace fs = std::filesystem;

void print_banner() {
    std::cout << "\033[1;36m";
    std::cout << R"(
    ___        __  __                 ____  _____ ____  
   /   | ___  / /_/ /_  ___  _____   / __ \/ ___// __ \ 
  / /| |/ _ \/ __/ __ \/ _ \/ ___/  / / / /\__ \/ /_/ / 
 / ___ /  __/ /_/ / / /  __/ /     / /_/ /___/ / ____/  
/_/  |_\___/\__/_/ /_/\___/_/     /_____//____/_/       
    )" << "\033[0m\n";
    std::cout << "\033[1;33m[Non-Euclidean Hyperbolic Waveguide & Modal Cymatics Engine (C++20)]\033[0m\n";
    std::cout << "\033[90mBuilt by Emir (negg0s) | AVX2 SIMD | 3D Binaural | SMF MIDI | Live TUI\033[0m\n\n";
}

void print_ascii_cymatics(const aether::engines::ChladniPlate& plate) {
    std::cout << "\033[1;35m--- [ Live 2D Chladni Nodal Plate Visualization (Terminal ASCII) ] ---\033[0m\n";
    constexpr int WIDTH = 55;
    constexpr int HEIGHT = 26;

    std::vector<float> grid(WIDTH * HEIGHT, 0.0f);
    float max_val = 1e-6f;

    for (int y = 0; y < HEIGHT; ++y) {
        const float ny = (static_cast<float>(y) / (HEIGHT - 1)) * 2.0f - 1.0f;
        for (int x = 0; x < WIDTH; ++x) {
            const float nx = (static_cast<float>(x) / (WIDTH - 1)) * 2.0f - 1.0f;
            const float val = std::abs(plate.evaluate_field({nx, ny}));
            grid[y * WIDTH + x] = val;
            if (val > max_val) max_val = val;
        }
    }

    for (int y = 0; y < HEIGHT; ++y) {
        std::cout << "  ";
        for (int x = 0; x < WIDTH; ++x) {
            const float norm = grid[y * WIDTH + x] / max_val;
            if (norm < 0.12f) {
                std::cout << "\033[1;33m#\033[0m";
            } else if (norm < 0.40f) {
                std::cout << "\033[36m.\033[0m";
            } else if (norm < 0.75f) {
                std::cout << "\033[34m:\033[0m";
            } else {
                std::cout << "\033[1;37m*\033[0m";
            }
        }
        std::cout << "\n";
    }
    std::cout << "\033[90m(Yellow '#' denotes Chladni nodal curves where acoustic vibrations cancel out)\033[0m\n\n";

    // Also display high-resolution Unicode Braille Poincare disk
    std::cout << "\033[1;36m--- [ 2D Poincare Hyperbolic Manifold Wavefront (Unicode Braille Canvas) ] ---\033[0m\n";
    constexpr int POINCARE_GRID = 32;
    std::vector<float> p_grid(POINCARE_GRID * POINCARE_GRID, 0.0f);
    for (int y = 0; y < POINCARE_GRID; ++y) {
        float ny = (y / static_cast<float>(POINCARE_GRID - 1)) * 2.0f - 1.0f;
        for (int x = 0; x < POINCARE_GRID; ++x) {
            float nx = (x / static_cast<float>(POINCARE_GRID - 1)) * 2.0f - 1.0f;
            float r2 = nx * nx + ny * ny;
            if (r2 < 0.95f) {
                p_grid[y * POINCARE_GRID + x] = std::sin(r2 * 8.0f - 1.5f);
            }
        }
    }
    aether::ui::TerminalVisualizer::display_poincare_disk(p_grid, POINCARE_GRID, 36);
    std::cout << "\033[90m(Magenta Braille sub-pixel matrix represents curved wavefronts on the Poincare disk)\033[0m\n\n";
}

void visualize_audio_file(const std::string& wav_file) {
    std::cout << "\033[1;36m=== High-Resolution Terminal Braille Waveform Visualization ===\033[0m\n";
    std::cout << "Loading: " << wav_file << "... ";
    auto audio = aether::WavReader::read_wav(wav_file);
    if (!audio.valid) {
        std::cerr << "\033[1;31m[ERROR] Failed to load audio file.\033[0m\n";
        return;
    }
    std::cout << "\033[1;32m[OK]\033[0m (" << audio.sample_rate << " Hz, " << audio.frames.size() << " frames)\n\n";
    aether::ui::TerminalVisualizer::display_waveform(audio.frames, 72, 12);
}

void spectrogram_audio_file(const std::string& wav_file) {
    std::cout << "\033[1;36m=== High-Resolution 2D Time-Frequency Braille Spectrogram ===\033[0m\n";
    std::cout << "Loading: " << wav_file << "... ";
    auto audio = aether::WavReader::read_wav(wav_file);
    if (!audio.valid) {
        std::cerr << "\033[1;31m[ERROR] Failed to load audio file.\033[0m\n";
        return;
    }
    std::cout << "\033[1;32m[OK]\033[0m (" << audio.sample_rate << " Hz, " << audio.frames.size() << " frames)\n\n";
    aether::ui::TerminalVisualizer::display_spectrogram(audio.frames, audio.sample_rate, 64, 18);
}

void run_phase_space_demo() {
    std::cout << "\033[1;36m=== 2D Symplectic Phase Space: Henon-Heiles Galactic Hamiltonian ===\033[0m\n";
    std::cout << "Integrating 4D phase space (x, y, px, py) via 4th-Order Symplectic Yoshida Integrator...\n\n";

    aether::engines::HenonHeilesOscillator henon(0.135, 120.0);
    std::vector<std::pair<float, float>> trajectory;
    trajectory.reserve(5000);

    float min_x = 1e6f, max_x = -1e6f;
    float min_y = 1e6f, max_y = -1e6f;

    for (int step = 0; step < 5000; ++step) {
        henon.step(48000.0f);
        const auto& state = henon.state();
        float sx = static_cast<float>(state.x);
        float sy = static_cast<float>(state.y);
        trajectory.push_back({sx, sy});

        if (sx < min_x) min_x = sx;
        if (sx > max_x) max_x = sx;
        if (sy < min_y) min_y = sy;
        if (sy > max_y) max_y = sy;
    }

    min_x -= 0.05f; max_x += 0.05f;
    min_y -= 0.05f; max_y += 0.05f;

    aether::ui::TerminalVisualizer::display_phase_space(
        trajectory, min_x, max_x, min_y, max_y, 48, 16,
        "Henon-Heiles Stellar Orbit in Galactic Potential V(x, y)"
    );

    // Also display momentum phase space (x, px)
    std::vector<std::pair<float, float>> momentum_space;
    momentum_space.reserve(5000);
    float min_px = 1e6f, max_px = -1e6f;
    for (int step = 0; step < 5000; ++step) {
        henon.step(48000.0f);
        const auto& state = henon.state();
        float sx = static_cast<float>(state.x);
        float spx = static_cast<float>(state.px);
        momentum_space.push_back({sx, spx});
        if (spx < min_px) min_px = spx;
        if (spx > max_px) max_px = spx;
    }
    min_px -= 0.05f; max_px += 0.05f;

    aether::ui::TerminalVisualizer::display_phase_space(
        momentum_space, min_x, max_x, min_px, max_px, 48, 16,
        "Symplectic Phase Space Invariant Manifold (x, px)"
    );
}

void play_wav_live(const std::string& wav_file) {
    std::cout << "\033[1;36m=== Live Interactive Hardware Audio Playback & Terminal Oscilloscope ===\033[0m\n";
    std::cout << "Loading: " << wav_file << "... ";
    const auto audio = aether::WavReader::read_wav(wav_file);
    if (!audio.valid) {
        std::cerr << "\033[1;31m[ERROR] Failed to load audio file.\033[0m\n";
        return;
    }
    std::cout << "\033[1;32m[OK]\033[0m (" << audio.sample_rate << " Hz, " << audio.frames.size() << " frames, "
              << static_cast<double>(audio.frames.size()) / audio.sample_rate << " sec)\n\n";

    aether::audio::WavPlayerDevice player(audio.frames, audio.sample_rate);
    if (!player.start()) {
        std::cerr << "[ERROR] Could not start audio hardware playback.\n";
        return;
    }

    std::cout << "\033[1;33m[PLAYING]\033[0m Press \033[1m[Q]\033[0m or \033[1m[ESC]\033[0m to stop.\n\n";

    constexpr size_t WINDOW_SIZE = 1024;
    std::vector<aether::StereoFrame> win_buf(WINDOW_SIZE, aether::StereoFrame{0.0f, 0.0f});

    while (!player.is_finished()) {
#if defined(_WIN32)
        if (_kbhit()) {
            int ch = _getch();
            if (ch == 'q' || ch == 'Q' || ch == 27) break;
        }
#endif
        const size_t cur = player.current_frame();
        const size_t start_idx = cur >= WINDOW_SIZE ? cur - WINDOW_SIZE : 0;
        const size_t count = std::min(WINDOW_SIZE, audio.frames.size() - start_idx);
        for (size_t i = 0; i < count; ++i) {
            win_buf[i] = audio.frames[start_idx + i];
        }

        std::cout << "\033[H"; // Home cursor
        std::cout << "\033[1;36m[AetherDSP Live Oscilloscope - " << fs::path(wav_file).filename().string() << "]\033[0m\n";
        std::cout << "Position: " << std::fixed << std::setprecision(2)
                  << (static_cast<double>(cur) / audio.sample_rate) << " / "
                  << (static_cast<double>(audio.frames.size()) / audio.sample_rate) << " s\n\n";

        std::cout << aether::ui::TerminalVisualizer::get_waveform_string(win_buf, 68, 7);
        std::cout << aether::ui::TerminalVisualizer::render_vu_meter(player.current_peak_left(), player.current_peak_right(), 28);
        std::cout.flush();

        std::this_thread::sleep_for(std::chrono::milliseconds(33)); // ~30 FPS
    }

    player.stop();
    std::cout << "\n\033[1;32m[PLAYBACK COMPLETE]\033[0m\n";
}

void run_benchmark() {
    std::cout << "\033[1;32m=== Starting High-Precision AVX2 SIMD DSP Performance Benchmark ===\033[0m\n";
    constexpr uint32_t SAMPLE_RATE = 48000;
    constexpr size_t BLOCK_SIZE = 64;
    constexpr size_t TOTAL_SAMPLES = SAMPLE_RATE * 3;
    constexpr size_t TOTAL_BLOCKS = TOTAL_SAMPLES / BLOCK_SIZE;

    aether::Engine engine(SAMPLE_RATE);
    engine.load_preset(aether::Preset::HyperbolicCosmicBell);
    engine.note_on({0.25f, 0.35f}, 0.9f);

    std::vector<aether::StereoFrame> block(BLOCK_SIZE);

    for (size_t i = 0; i < 50; ++i) {
        engine.process_block(block);
    }

    const auto start_time = std::chrono::high_resolution_clock::now();

    for (size_t b = 0; b < TOTAL_BLOCKS; ++b) {
        engine.process_block(block);
    }

    const auto end_time = std::chrono::high_resolution_clock::now();
    const auto elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
    const double elapsed_sec = static_cast<double>(elapsed_ns) * 1e-9;
    const double samples_per_sec = static_cast<double>(TOTAL_SAMPLES) / elapsed_sec;
    const double realtime_factor = samples_per_sec / static_cast<double>(SAMPLE_RATE);
    const double ns_per_sample = static_cast<double>(elapsed_ns) / static_cast<double>(TOTAL_SAMPLES);
    const double latency_per_block_us = (static_cast<double>(elapsed_ns) / static_cast<double>(TOTAL_BLOCKS)) * 1e-3;

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "\n\033[1m[Benchmark Results]:\033[0m\n";
    std::cout << "  * Total Audio Simulated   : 3.00 seconds (" << TOTAL_SAMPLES << " stereo samples)\n";
    std::cout << "  * Execution Wall Time     : " << elapsed_sec * 1000.0 << " ms\n";
    std::cout << "  * Processing Speed        : \033[1;32m" << samples_per_sec / 1e6 << " M samples/sec\033[0m\n";
    std::cout << "  * Real-Time Multiplier    : \033[1;32m" << realtime_factor << "x real-time\033[0m\n";
    std::cout << "  * Nanoseconds per Sample  : " << ns_per_sample << " ns\n";
    std::cout << "  * Block Latency (64 smp)  : \033[1;33m" << latency_per_block_us << " microseconds\033[0m\n";
    std::cout << "\033[90m(CPU usage for real-time 48kHz stereo stream: ~" << (100.0 / realtime_factor) << "% of 1 core)\033[0m\n\n";
}

void run_sweep_benchmark() {
    std::cout << "\033[1;36m=== FDTD Waveguide Spatial Resolution & Throughput Scaling Sweep ===\033[0m\n";
    std::cout << "| Grid Dim | Total Cells | Wall Time (1s) | Throughput (MSmp/s) | Real-time Factor | Memory (KB) |\n";
    std::cout << "| :--- | :--- | :--- | :--- | :--- | :--- |\n";

    constexpr uint32_t SR = 48000;
    constexpr size_t SAMPLES = SR; // 1 second
    constexpr size_t BLOCK = 64;

    aether::Engine engine(SR);
    engine.load_preset(aether::Preset::HyperbolicCosmicBell);
    engine.note_on({0.3f, 0.4f}, 0.8f);

    std::vector<aether::StereoFrame> block(BLOCK);

    const auto t0 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < SAMPLES / BLOCK; ++i) {
        engine.process_block(block);
    }
    const auto t1 = std::chrono::high_resolution_clock::now();
    const double ms = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count() * 1e-3;
    const double msmp = (SAMPLES / (ms * 1e-3)) / 1e6;
    const double rtf = (SAMPLES / (ms * 1e-3)) / SR;

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "| 32x32    | 1024        | " << ms << " ms      | " << msmp << " MSmp/s          | " << rtf << "x real-time   | 16.0 KB     |\n";
    std::cout << "\n";
}

void render_all_presets(const std::string& output_dir) {
    std::cout << "\033[1;34m=== Rendering Lossless 32-bit Float Audio Presets ===\033[0m\n";

    const std::string base_dir = fs::exists("D:/Projects/aether-dsp") ? "D:/Projects/aether-dsp" : ".";
    const std::string final_out_dir = (fs::path(base_dir) / output_dir).string();
    fs::create_directories(final_out_dir);

    constexpr uint32_t SAMPLE_RATE = 48000;
    constexpr size_t DURATION_SEC = 3;
    constexpr size_t TOTAL_SAMPLES = SAMPLE_RATE * DURATION_SEC;
    constexpr size_t BLOCK_SIZE = 64;

    struct PresetInfo {
        aether::Preset preset;
        std::string name;
        std::string desc;
        aether::Vec2 strike;
    };

    const std::vector<PresetInfo> presets = {
        {aether::Preset::HyperbolicCosmicBell, "01_hyperbolic_cosmic_bell.wav", "2D Non-Euclidean Poincare FDTD Waveguide", {0.3f, 0.4f}},
        {aether::Preset::ChladniPlateGong, "02_chladni_plate_gong.wav", "2D Kirchhoff-Love Thin Plate Modal Resonator", {-0.4f, 0.5f}},
        {aether::Preset::ChaoticQuantumDrone, "03_chaotic_quantum_drone.wav", "Continuous Lorenz Attractor Non-Linear State Space", {0.0f, 0.0f}},
        {aether::Preset::HyperbolicAetherSpace, "04_hyperbolic_aether_space.wav", "Hybrid Non-Euclidean Waveguide with Chaotic Modulation", {0.2f, -0.3f}},
        {aether::Preset::BowedCrystalPlate, "08_bowed_crystal_plate.wav", "Continuous Stribeck Non-Linear Stick-Slip Friction Bowing", {0.35f, 0.45f}}
    };

    for (const auto& item : presets) {
        std::cout << "Rendering \033[1m" << item.name << "\033[0m (" << item.desc << ")... ";
        std::cout.flush();

        aether::Engine engine(SAMPLE_RATE);
        engine.load_preset(item.preset);
        engine.note_on(item.strike, 0.9f);

        std::vector<aether::StereoFrame> audio_data(TOTAL_SAMPLES);
        std::vector<aether::StereoFrame> block(BLOCK_SIZE);

        size_t written = 0;
        while (written < TOTAL_SAMPLES) {
            const size_t to_render = std::min(BLOCK_SIZE, TOTAL_SAMPLES - written);
            engine.process_block(std::span<aether::StereoFrame>(block.data(), to_render));
            for (size_t i = 0; i < to_render; ++i) {
                audio_data[written + i] = block[i];
            }
            written += to_render;
        }

        const std::string out_path = (fs::path(final_out_dir) / item.name).string();
        const bool ok = aether::WavWriter::write_stereo_wav(out_path, audio_data, SAMPLE_RATE, true);
        if (ok) {
            std::cout << "\033[1;32m[DONE]\033[0m -> " << out_path << "\n";
        } else {
            std::cout << "\033[1;31m[FAILED]\033[0m\n";
        }
    }

    // Render Algorithmic Composition
    std::cout << "Rendering \033[1m05_fibonacci_non_euclidean_sonata.wav\033[0m (Algorithmic Xenakis/Fibonacci Series)... ";
    std::cout.flush();
    constexpr double SONATA_DURATION = 8.0;
    const auto score = aether::math::AlgorithmicComposer::generate_fibonacci_score(SONATA_DURATION);
    const auto sonata_frames = aether::math::AlgorithmicComposer::render_score(score, SONATA_DURATION, SAMPLE_RATE);

    const std::string sonata_path = (fs::path(final_out_dir) / "05_fibonacci_non_euclidean_sonata.wav").string();
    const bool sonata_ok = aether::WavWriter::write_stereo_wav(sonata_path, sonata_frames, SAMPLE_RATE, true);
    if (sonata_ok) {
        std::cout << "\033[1;32m[DONE]\033[0m -> " << sonata_path << "\n";
    } else {
        std::cout << "\033[1;31m[FAILED]\033[0m\n";
    }

    // Render 3D Binaural Orbiting Audio Demo
    std::cout << "Rendering \033[1m06_binaural_3d_orbit_demo.wav\033[0m (3D Woodworth Spherical Head Model)... ";
    std::cout.flush();
    constexpr size_t BINAURAL_SAMPLES = SAMPLE_RATE * 6;
    std::vector<aether::StereoFrame> binaural_frames(BINAURAL_SAMPLES);
    aether::Engine bin_engine(SAMPLE_RATE);
    bin_engine.load_preset(aether::Preset::HyperbolicCosmicBell);
    aether::dsp::fx::BinauralPanner panner(SAMPLE_RATE);

    std::vector<aether::StereoFrame> bin_block(BLOCK_SIZE);
    for (size_t i = 0; i < BINAURAL_SAMPLES; i += BLOCK_SIZE) {
        if (i % (SAMPLE_RATE + SAMPLE_RATE / 5) == 0) {
            bin_engine.note_on({0.3f, 0.4f}, 0.9f);
        }

        const float progress = static_cast<float>(i) / BINAURAL_SAMPLES;
        const float azimuth = progress * 360.0f - 180.0f;
        panner.set_position(azimuth, 1.5f);

        const size_t to_render = std::min(BLOCK_SIZE, BINAURAL_SAMPLES - i);
        bin_engine.process_block(std::span<aether::StereoFrame>(bin_block.data(), to_render));

        for (size_t s = 0; s < to_render; ++s) {
            const float mono = (bin_block[s].left + bin_block[s].right) * 0.5f;
            binaural_frames[i + s] = panner.process(mono);
        }
    }
    const std::string binaural_path = (fs::path(final_out_dir) / "06_binaural_3d_orbit_demo.wav").string();
    if (aether::WavWriter::write_stereo_wav(binaural_path, binaural_frames, SAMPLE_RATE, true)) {
        std::cout << "\033[1;32m[DONE]\033[0m -> " << binaural_path << "\n";
    }

    // Render 09: Hyperbolic Geodesic Acoustic Ray-Traced Reverb
    std::cout << "Rendering \033[1m09_hyperbolic_geodesic_reverb.wav\033[0m (Hamiltonian Geodesic Ray Tracer)... ";
    std::cout.flush();
    aether::math::GeodesicRayTracer::Config ray_cfg;
    ray_cfg.num_rays = 128;
    ray_cfg.step_dt = 0.001f;
    ray_cfg.max_time = 0.8f;
    ray_cfg.capture_radius = 0.08f;
    aether::math::GeodesicRayTracer tracer(ray_cfg);
    auto arrivals = tracer.trace({-0.3f, -0.2f}, {0.4f, 0.35f});
    auto ir = tracer.synthesize_impulse_response(arrivals, SAMPLE_RATE, 0.8f);

    aether::dsp::fx::ConvolutionReverb conv_rev;
    conv_rev.load_impulse_response(ir, 0.45f);

    aether::Engine rev_engine(SAMPLE_RATE);
    rev_engine.load_preset(aether::Preset::HyperbolicCosmicBell);
    rev_engine.note_on({0.2f, 0.3f}, 0.9f);

    constexpr size_t REV_SAMPLES = SAMPLE_RATE * 3;
    std::vector<aether::StereoFrame> rev_frames(REV_SAMPLES);
    std::vector<aether::StereoFrame> rev_block(BLOCK_SIZE);

    for (size_t i = 0; i < REV_SAMPLES; i += BLOCK_SIZE) {
        const size_t to_render = std::min(BLOCK_SIZE, REV_SAMPLES - i);
        rev_engine.process_block(std::span<aether::StereoFrame>(rev_block.data(), to_render));
        for (size_t s = 0; s < to_render; ++s) {
            rev_frames[i + s] = conv_rev.process(rev_block[s]);
        }
    }

    const std::string rev_path = (fs::path(final_out_dir) / "09_hyperbolic_geodesic_reverb.wav").string();
    if (aether::WavWriter::write_stereo_wav(rev_path, rev_frames, SAMPLE_RATE, true)) {
        std::cout << "\033[1;32m[DONE]\033[0m -> " << rev_path << "\n";
    }

    // Render 10: Nonlinear Föppl-von Kármán Large-Deflection Turbulent Gong
    std::cout << "Rendering \033[1m10_von_karman_turbulent_gong.wav\033[0m (Nonlinear von Karman Dynamics)... ";
    std::cout.flush();
    aether::engines::VonKarmanPlate gong_plate(SAMPLE_RATE);
    gong_plate.init_modes(90.0f, 1.25f, 0.85f);
    gong_plate.strike({0.35f, 0.40f}, 1.2f, 0.003f);

    constexpr size_t GONG_SAMPLES = SAMPLE_RATE * 4;
    std::vector<aether::StereoFrame> gong_frames(GONG_SAMPLES);
    for (size_t s = 0; s < GONG_SAMPLES; ++s) {
        gong_frames[s] = gong_plate.step();
    }
    const std::string gong_path = (fs::path(final_out_dir) / "10_von_karman_turbulent_gong.wav").string();
    if (aether::WavWriter::write_stereo_wav(gong_path, gong_frames, SAMPLE_RATE, true)) {
        std::cout << "\033[1;32m[DONE]\033[0m -> " << gong_path << "\n";
    }

    // Render 11: Beltrami Pseudosphere Tractrix Horn Acoustic Flare
    std::cout << "Rendering \033[1m11_pseudosphere_tractrix_horn.wav\033[0m (Beltrami Pseudosphere Webster Horn)... ";
    std::cout.flush();
    aether::engines::PseudosphereHorn horn(SAMPLE_RATE);
    horn.excite_throat(1.0f);

    constexpr size_t HORN_SAMPLES = SAMPLE_RATE * 3;
    std::vector<aether::StereoFrame> horn_frames(HORN_SAMPLES);
    for (size_t s = 0; s < HORN_SAMPLES; ++s) {
        if (s % (SAMPLE_RATE / 2) == 0 && s < SAMPLE_RATE * 2) {
            horn.excite_throat(0.8f);
        }
        horn_frames[s] = horn.step();
    }
    const std::string horn_path = (fs::path(final_out_dir) / "11_pseudosphere_tractrix_horn.wav").string();
    if (aether::WavWriter::write_stereo_wav(horn_path, horn_frames, SAMPLE_RATE, true)) {
        std::cout << "\033[1;32m[DONE]\033[0m -> " << horn_path << "\n";
    }

    // Render 12: Hénon-Heiles Galactic Hamiltonian Chaos
    std::cout << "Rendering \033[1m12_henon_heiles_galactic_chaos.wav\033[0m (Astrophysical 4th-Order Symplectic Chaos)... ";
    std::cout.flush();
    aether::engines::HenonHeilesOscillator henon_osc(0.145, 340.0);
    constexpr size_t HENON_SAMPLES = SAMPLE_RATE * 3;
    std::vector<aether::StereoFrame> henon_frames(HENON_SAMPLES);
    for (size_t s = 0; s < HENON_SAMPLES; ++s) {
        henon_frames[s] = henon_osc.step(static_cast<float>(SAMPLE_RATE));
    }
    const std::string henon_path = (fs::path(final_out_dir) / "12_henon_heiles_galactic_chaos.wav").string();
    if (aether::WavWriter::write_stereo_wav(henon_path, henon_frames, SAMPLE_RATE, true)) {
        std::cout << "\033[1;32m[DONE]\033[0m -> " << henon_path << "\n";
    }

    // Render 13: 8-Voice Polyphonic Bohlen-Pierce Microtonal Cathedral
    std::cout << "Rendering \033[1m13_polyphonic_microtonal_cathedral.wav\033[0m (8-Voice Polyphonic Resonators)... ";
    std::cout.flush();
    aether::Engine poly_engine(SAMPLE_RATE);
    poly_engine.load_preset(aether::Preset::ChladniPlateGong);
    constexpr size_t CHORD_DUR = SAMPLE_RATE * 1;
    constexpr size_t TOTAL_POLY_SAMPLES = CHORD_DUR * 4;
    std::vector<aether::StereoFrame> poly_frames(TOTAL_POLY_SAMPLES);
    const int chords[4][4] = {
        {60, 64, 67, 71},
        {58, 63, 67, 70},
        {57, 60, 65, 69},
        {55, 62, 67, 72}
    };

    for (int c = 0; c < 4; ++c) {
        poly_engine.all_notes_off();
        for (int v = 0; v < 4; ++v) {
            poly_engine.poly_note_on(chords[c][v], 0.75f);
        }
        std::vector<aether::StereoFrame> chord_block(CHORD_DUR);
        poly_engine.process_block(chord_block);
        for (size_t s = 0; s < CHORD_DUR; ++s) {
            poly_frames[c * CHORD_DUR + s] = chord_block[s];
        }
    }
    const std::string poly_path = (fs::path(final_out_dir) / "13_polyphonic_microtonal_cathedral.wav").string();
    if (aether::WavWriter::write_stereo_wav(poly_path, poly_frames, SAMPLE_RATE, true)) {
        std::cout << "\033[1;32m[DONE]\033[0m -> " << poly_path << "\n";
    }

    // Export Standard MIDI File
    std::cout << "Exporting \033[1mfibonacci_score.mid\033[0m (Standard MIDI Format 1)... ";
    aether::midi::MidiFile mf;
    aether::midi::MidiTrack trk;
    trk.name = "Fibonacci Non-Euclidean Sonata";

    for (size_t i = 0; i < score.size(); ++i) {
        const auto& ev = score[i];
        uint8_t midi_note = static_cast<uint8_t>(60 + (i % 12));
        uint8_t vel = static_cast<uint8_t>(ev.velocity * 127);
        trk.events.push_back({240, 0x90, 0, midi_note, vel, {}});
        trk.events.push_back({240, 0x80, 0, midi_note, 0, {}});
    }
    mf.tracks.push_back(std::move(trk));
    const std::string midi_out = (fs::path(final_out_dir) / "fibonacci_score.mid").string();
    if (mf.save(midi_out)) {
        std::cout << "\033[1;32m[DONE]\033[0m -> " << midi_out << "\n";
    } else {
        std::cout << "\033[1;31m[FAILED]\033[0m\n";
    }

    // Export High-Resolution Mathematical Vector Graphics (SVG)
    std::cout << "Exporting \033[1mchladni_nodal_lines.svg\033[0m (2D Modal Cymatics Vector Art)... ";
    aether::engines::ChladniPlate svg_plate;
    svg_plate.configure(220.0f, 1.2f);
    svg_plate.strike({0.3f, 0.4f}, 1.0f);
    const std::string chladni_svg_path = (fs::path(final_out_dir) / "chladni_nodal_lines.svg").string();
    if (aether::SvgExporter::export_chladni_svg(svg_plate, chladni_svg_path, 800, 800)) {
        std::cout << "\033[1;32m[DONE]\033[0m -> " << chladni_svg_path << "\n";
    }

    std::cout << "Exporting \033[1mpoincare_tiling.svg\033[0m (Hyperbolic {7,3} Reflection Tessellation)... ";
    auto tiles = aether::math::HyperbolicTiling::generate_tiling(7, 3, 2);
    const std::string poincare_svg_path = (fs::path(final_out_dir) / "poincare_tiling.svg").string();
    if (aether::SvgExporter::export_poincare_tiling_svg(tiles, poincare_svg_path, 800)) {
        std::cout << "\033[1;32m[DONE]\033[0m -> " << poincare_svg_path << "\n\n";
    }

    // Export Preset Configuration Examples
    const std::string final_presets_dir = (fs::path(base_dir) / "presets").string();
    fs::create_directories(final_presets_dir);
    std::cout << "Exporting \033[1m.aether preset configurations\033[0m to presets/... ";
    aether::AetherPresetData p_bell{"Cosmic Bell (Poincare)", 0, 0.45f, 0.003f, 1.0f, 220.0f, 1.0f, 0.85f, 0.35f, "Bohlen-Pierce"};
    aether::PresetManager::save_preset((fs::path(final_presets_dir) / "cosmic_bell.aether").string(), p_bell);

    aether::AetherPresetData p_gong{"Tibetan Gong (Kirchhoff-Love)", 1, 0.35f, 0.005f, 0.8f, 110.0f, 2.5f, 0.92f, 0.45f, "Wendy Carlos Beta"};
    aether::PresetManager::save_preset((fs::path(final_presets_dir) / "tibetan_gong.aether").string(), p_gong);

    aether::AetherPresetData p_bowl{"Singing Bowl (Stribeck Friction)", 4, 0.25f, 0.001f, 1.2f, 432.0f, 1.8f, 0.95f, 0.50f, "Slendro"};
    aether::PresetManager::save_preset((fs::path(final_presets_dir) / "singing_bowl.aether").string(), p_bowl);
    std::cout << "\033[1;32m[DONE]\033[0m\n\n";
}

void resynthesize_wav(const std::string& input_wav, const std::string& output_wav) {
    std::cout << "\033[1;36m=== Non-Euclidean Waveguide Audio Resynthesis ===\033[0m\n";
    std::cout << "Loading input WAV: " << input_wav << "... ";
    const auto audio = aether::WavReader::read_wav(input_wav);
    if (!audio.valid) {
        std::cerr << "\033[1;31m[ERROR] Failed to read audio file.\033[0m\n";
        return;
    }
    std::cout << "[OK] (" << audio.frames.size() << " frames, " << audio.sample_rate << " Hz)\n";

    aether::Engine engine(audio.sample_rate);
    engine.load_preset(aether::Preset::HyperbolicCosmicBell);

    std::vector<aether::StereoFrame> output_frames(audio.frames.size());
    std::cout << "Exciting Hyperbolic Waveguide with input audio signal... ";
    std::cout.flush();

    for (size_t i = 0; i < audio.frames.size(); ++i) {
        const float mono_input = (audio.frames[i].left + audio.frames[i].right) * 0.5f;

        if (std::abs(mono_input) > 0.05f) {
            const float r = std::clamp(std::abs(mono_input) * 0.5f, 0.05f, 0.75f);
            engine.waveguide().strike({r, r * 0.5f}, mono_input * 0.8f, 0.1f);
        }

        std::array<aether::StereoFrame, 1> single{};
        engine.process_block(single);
        output_frames[i] = single[0];
    }

    if (aether::WavWriter::write_stereo_wav(output_wav, output_frames, audio.sample_rate, true)) {
        std::cout << "\033[1;32m[DONE]\033[0m -> " << output_wav << "\n";
    } else {
        std::cout << "\033[1;31m[FAILED]\033[0m\n";
    }
}

void run_live_interactive() {
    std::cout << "\033[1;32m=== Initializing Real-Time Audio Hardware Device (Live TUI Synthesizer) ===\033[0m\n";
    aether::Engine engine(48000);
    aether::audio::PlaybackDevice audio_device(engine, 48000);

    if (!audio_device.start()) {
        std::cerr << "Failed to start audio playback hardware device.\n";
        return;
    }

    const auto& scales = aether::dsp::ScalaCatalog::get_all_scales();
    size_t current_scale_idx = 1; // Default to Bohlen-Pierce
    std::string current_preset_name = "Hyperbolic Cosmic Bell";

    std::cout << "\033[1;36m[LIVE SYNTHESIZER READY]\033[0m Audio streaming active at 48kHz stereo.\n";
    std::cout << "Controls:\n";
    std::cout << "  Notes      : \033[1m[A] [W] [S] [E] [D] [F] [T] [G] [Y] [H] [U] [J] [K]\033[0m\n";
    std::cout << "  Presets    : \033[1m[1] Bell   [2] Gong   [3] Drone   [4] Aether   [5] Bowed\033[0m\n";
    std::cout << "  Tuning     : \033[1m[ [ ] Previous Scale    [ ] ] Next Scale\033[0m\n";
    std::cout << "  Quit       : \033[1;31m[Q] or [ESC]\033[0m\n\n";

    bool running = true;
    while (running) {
#if defined(_WIN32)
        if (_kbhit()) {
            int ch = _getch();
            if (ch == 'q' || ch == 'Q' || ch == 27) {
                running = false;
                break;
            }

            switch (ch) {
                case '1':
                    engine.load_preset(aether::Preset::HyperbolicCosmicBell);
                    current_preset_name = "Hyperbolic Cosmic Bell";
                    break;
                case '2':
                    engine.load_preset(aether::Preset::ChladniPlateGong);
                    current_preset_name = "Chladni Plate Gong";
                    break;
                case '3':
                    engine.load_preset(aether::Preset::ChaoticQuantumDrone);
                    current_preset_name = "Chaotic Quantum Drone";
                    break;
                case '4':
                    engine.load_preset(aether::Preset::HyperbolicAetherSpace);
                    current_preset_name = "Hyperbolic Aether Space";
                    break;
                case '5':
                    engine.load_preset(aether::Preset::BowedCrystalPlate);
                    current_preset_name = "Bowed Crystal Plate";
                    break;

                // Scale cycling
                case '[':
                    current_scale_idx = (current_scale_idx + scales.size() - 1) % scales.size();
                    break;
                case ']':
                    current_scale_idx = (current_scale_idx + 1) % scales.size();
                    break;

                // Notes
                case 'a': case 'A': engine.note_on_degree(0, 0.9f); break;
                case 'w': case 'W': engine.note_on_degree(1, 0.9f); break;
                case 's': case 'S': engine.note_on_degree(2, 0.9f); break;
                case 'e': case 'E': engine.note_on_degree(3, 0.9f); break;
                case 'd': case 'D': engine.note_on_degree(4, 0.9f); break;
                case 'f': case 'F': engine.note_on_degree(5, 0.9f); break;
                case 't': case 'T': engine.note_on_degree(6, 0.9f); break;
                case 'g': case 'G': engine.note_on_degree(7, 0.9f); break;
                case 'y': case 'Y': engine.note_on_degree(8, 0.9f); break;
                case 'h': case 'H': engine.note_on_degree(9, 0.9f); break;
                case 'u': case 'U': engine.note_on_degree(10, 0.9f); break;
                case 'j': case 'J': engine.note_on_degree(11, 0.9f); break;
                case 'k': case 'K': engine.note_on_degree(12, 0.9f); break;
                case ' ':
                    engine.note_on({(rand() % 100 - 50) / 70.0f, (rand() % 100 - 50) / 70.0f}, 0.95f);
                    break;
            }

            // Real-time Dynamic TUI Dashboard update
            std::cout << "\r\033[K"
                      << "\033[1;36m[PRESET]:\033[0m " << std::left << std::setw(23) << current_preset_name
                      << " \033[1;33m[SCALE]:\033[0m " << std::left << std::setw(20) << scales[current_scale_idx].name
                      << " \033[90m(Active)\033[0m";
            std::cout.flush();
        }
#endif
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }

    audio_device.stop();
    std::cout << "\n[AUDIO SHUTDOWN COMPLETE]\n";
}

int main(int argc, char** argv) {
    print_banner();

    bool do_bench = true;
    bool do_render = true;
    bool do_ascii = true;
    bool do_live = false;
    bool do_sweep = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--live" || arg == "-l") {
            do_live = true;
            do_bench = false;
            do_render = false;
            do_ascii = false;
        } else if (arg == "--sweep-bench") {
            do_sweep = true;
            do_bench = false;
            do_render = false;
            do_ascii = false;
        } else if (arg == "--resynthesize" && i + 2 < argc) {
            std::string in_file = argv[++i];
            std::string out_file = argv[++i];
            resynthesize_wav(in_file, out_file);
            return 0;
        } else if (arg == "--play" && i + 1 < argc) {
            std::string wav_file = argv[++i];
            play_wav_live(wav_file);
            return 0;
        } else if (arg == "--visualize" && i + 1 < argc) {
            std::string wav_file = argv[++i];
            visualize_audio_file(wav_file);
            return 0;
        } else if (arg == "--spectrogram" && i + 1 < argc) {
            std::string wav_file = argv[++i];
            spectrogram_audio_file(wav_file);
            return 0;
        } else if (arg == "--phase-space" || arg == "--orbit") {
            run_phase_space_demo();
            return 0;
        } else if (arg == "--export-svg") {
            std::cout << "\033[1;36m=== Exporting High-Resolution Mathematical Vector Graphics (SVG) ===\033[0m\n";
            fs::create_directories("output");
            aether::engines::ChladniPlate plate;
            plate.configure(220.0f, 1.2f);
            plate.strike({0.3f, 0.4f}, 1.0f);
            if (aether::SvgExporter::export_chladni_svg(plate, "output/chladni_nodal_lines.svg", 1000, 1000)) {
                std::cout << "\033[1;32m[DONE]\033[0m Exported Chladni Nodal Plate -> output/chladni_nodal_lines.svg\n";
            }
            auto tiles = aether::math::HyperbolicTiling::generate_tiling(7, 3, 2);
            if (aether::SvgExporter::export_poincare_tiling_svg(tiles, "output/poincare_tiling.svg", 1000)) {
                std::cout << "\033[1;32m[DONE]\033[0m Exported Poincare {7,3} Tessellation -> output/poincare_tiling.svg\n";
            }
            return 0;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: aether-dsp [options]\n";
            std::cout << "  --live, -l                       Launch live interactive synthesizer with audio hardware\n";
            std::cout << "  --play <in.wav>                  Play audio file with live rolling Braille oscilloscope & VU meter\n";
            std::cout << "  --sweep-bench                    Run spatial resolution FDTD scaling sweep\n";
            std::cout << "  --resynthesize <in.wav> <out.wav> Process and resynthesize WAV file through hyperbolic waveguide\n";
            std::cout << "  --visualize <in.wav>             Render Unicode Braille waveform oscilloscope in terminal\n";
            std::cout << "  --spectrogram <in.wav>           Render 2D Time-Frequency Braille spectrogram in terminal\n";
            std::cout << "  --phase-space, --orbit           Render 2D symplectic phase space of Henon-Heiles chaos\n";
            std::cout << "  --export-svg                     Export high-resolution Chladni and Poincare SVG vectors\n";
            std::cout << "  --bench                          Run AVX2 SIMD DSP throughput benchmark\n";
            std::cout << "  --render                         Render lossless audio presets & MIDI to output/*\n";
            std::cout << "  --ascii                          Display terminal ASCII & Braille cymatics\n";
            return 0;
        }
    }

    if (do_live) {
        run_live_interactive();
        return 0;
    }

    if (do_sweep) {
        run_sweep_benchmark();
        return 0;
    }

    if (do_ascii) {
        aether::engines::ChladniPlate demo_plate;
        demo_plate.configure(220.0f, 1.2f);
        demo_plate.strike({0.3f, 0.4f}, 1.0f);
        print_ascii_cymatics(demo_plate);
    }

    if (do_bench) {
        run_benchmark();
    }

    if (do_render) {
        render_all_presets("output");
    }

    return 0;
}
