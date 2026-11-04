#include "aether/engine.hpp"
#include "aether/wav_writer.hpp"
#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <iomanip>
#include <filesystem>

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
    std::cout << "\033[90mBuilt by Emir (negg0s) | Zero-OS-lock-in | Zero-Allocation Real-Time Core\033[0m\n\n";
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
                // Nodal line: zero vibration, sand gathers here!
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
}

void run_benchmark() {
    std::cout << "\033[1;32m=== Starting High-Precision DSP Performance Benchmark ===\033[0m\n";
    constexpr uint32_t SAMPLE_RATE = 48000;
    constexpr size_t BLOCK_SIZE = 64;
    constexpr size_t TOTAL_SAMPLES = SAMPLE_RATE * 2; // 2 seconds of high-precision audio
    constexpr size_t TOTAL_BLOCKS = TOTAL_SAMPLES / BLOCK_SIZE;

    aether::Engine engine(SAMPLE_RATE);
    engine.load_preset(aether::Preset::HyperbolicCosmicBell);
    engine.note_on({0.25f, 0.35f}, 0.9f);

    std::vector<aether::StereoFrame> block(BLOCK_SIZE);

    // Warm-up cache
    for (size_t i = 0; i < 50; ++i) {
        engine.process_block(block);
    }

    // Benchmark loop
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
    std::cout << "  * Total Audio Simulated   : 2.00 seconds (" << TOTAL_SAMPLES << " stereo samples)\n";
    std::cout << "  * Execution Wall Time     : " << elapsed_sec * 1000.0 << " ms\n";
    std::cout << "  * Processing Speed        : \033[1;32m" << samples_per_sec / 1e6 << " M samples/sec\033[0m\n";
    std::cout << "  * Real-Time Multiplier    : \033[1;32m" << realtime_factor << "x real-time\033[0m\n";
    std::cout << "  * Nanoseconds per Sample  : " << ns_per_sample << " ns\n";
    std::cout << "  * Block Latency (64 smp)  : \033[1;33m" << latency_per_block_us << " microseconds\033[0m\n";
    std::cout << "\033[90m(CPU usage for real-time 48kHz stereo stream: ~" << (100.0 / realtime_factor) << "% of 1 core)\033[0m\n\n";
}

void render_all_presets(const std::string& output_dir) {
    std::cout << "\033[1;34m=== Rendering Lossless 32-bit Float Audio Presets ===\033[0m\n";

    fs::create_directories(output_dir);
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
        {aether::Preset::HyperbolicAetherSpace, "04_hyperbolic_aether_space.wav", "Hybrid Non-Euclidean Waveguide with Chaotic Modulation", {0.2f, -0.3f}}
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

        const std::string out_path = (fs::path(output_dir) / item.name).string();
        const bool ok = aether::WavWriter::write_stereo_wav(out_path, audio_data, SAMPLE_RATE, true);
        if (ok) {
            std::cout << "\033[1;32m[DONE]\033[0m -> " << out_path << "\n";
        } else {
            std::cout << "\033[1;31m[FAILED]\033[0m\n";
        }
    }
    std::cout << "\n";
}

int main(int argc, char** argv) {
    print_banner();

    bool do_bench = true;
    bool do_render = true;
    bool do_ascii = true;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: aether-dsp [options]\n";
            std::cout << "  --bench          Run nanosecond resolution DSP throughput benchmark\n";
            std::cout << "  --render         Render lossless audio presets to output/*.wav\n";
            std::cout << "  --ascii          Display terminal ASCII Chladni nodal plate\n";
            return 0;
        }
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
