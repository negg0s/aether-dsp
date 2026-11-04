#include "aether/engine.hpp"
#include "aether/wav_writer.hpp"
#include "aether/wav_reader.hpp"
#include "aether/math/algorithmic_composer.hpp"
#include "aether/math/hyperbolic_tiling.hpp"
#include "aether/audio/playback_device.hpp"
#include "aether/midi/midi_file.hpp"
#include "aether/dsp/tuning.hpp"
#include "aether/dsp/fx/binaural_panner.hpp"
#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <iomanip>
#include <filesystem>
#include <cmath>
#include <thread>
#include <array>

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

    // Render Algorithmic Composition
    std::cout << "Rendering \033[1m05_fibonacci_non_euclidean_sonata.wav\033[0m (Algorithmic Xenakis/Fibonacci Series)... ";
    std::cout.flush();
    constexpr double SONATA_DURATION = 8.0;
    const auto score = aether::math::AlgorithmicComposer::generate_fibonacci_score(SONATA_DURATION);
    const auto sonata_frames = aether::math::AlgorithmicComposer::render_score(score, SONATA_DURATION, SAMPLE_RATE);

    const std::string sonata_path = (fs::path(output_dir) / "05_fibonacci_non_euclidean_sonata.wav").string();
    const bool sonata_ok = aether::WavWriter::write_stereo_wav(sonata_path, sonata_frames, SAMPLE_RATE, true);
    if (sonata_ok) {
        std::cout << "\033[1;32m[DONE]\033[0m -> " << sonata_path << "\n";
    } else {
        std::cout << "\033[1;31m[FAILED]\033[0m\n";
    }

    // Render 3D Binaural Orbiting Audio Demo
    std::cout << "Rendering \033[1m06_binaural_3d_orbit_demo.wav\033[0m (3D Woodworth Spherical Head Model)... ";
    std::cout.flush();
    constexpr size_t BINAURAL_SAMPLES = SAMPLE_RATE * 6; // 6 seconds 360 degree orbit
    std::vector<aether::StereoFrame> binaural_frames(BINAURAL_SAMPLES);
    aether::Engine bin_engine(SAMPLE_RATE);
    bin_engine.load_preset(aether::Preset::HyperbolicCosmicBell);
    aether::dsp::fx::BinauralPanner panner(SAMPLE_RATE);

    std::vector<aether::StereoFrame> bin_block(BLOCK_SIZE);
    for (size_t i = 0; i < BINAURAL_SAMPLES; i += BLOCK_SIZE) {
        // Strike bells every 1.2 seconds
        if (i % (SAMPLE_RATE + SAMPLE_RATE / 5) == 0) {
            bin_engine.note_on({0.3f, 0.4f}, 0.9f);
        }

        // Orbit azimuth angle: 0 to 360 degrees
        const float progress = static_cast<float>(i) / BINAURAL_SAMPLES;
        const float azimuth = progress * 360.0f - 180.0f; // -180 to +180
        panner.set_position(azimuth, 1.5f);

        const size_t to_render = std::min(BLOCK_SIZE, BINAURAL_SAMPLES - i);
        bin_engine.process_block(std::span<aether::StereoFrame>(bin_block.data(), to_render));

        for (size_t s = 0; s < to_render; ++s) {
            const float mono = (bin_block[s].left + bin_block[s].right) * 0.5f;
            binaural_frames[i + s] = panner.process(mono);
        }
    }
    const std::string binaural_path = (fs::path(output_dir) / "06_binaural_3d_orbit_demo.wav").string();
    if (aether::WavWriter::write_stereo_wav(binaural_path, binaural_frames, SAMPLE_RATE, true)) {
        std::cout << "\033[1;32m[DONE]\033[0m -> " << binaural_path << "\n";
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
    const std::string midi_out = (fs::path(output_dir) / "fibonacci_score.mid").string();
    if (mf.save(midi_out)) {
        std::cout << "\033[1;32m[DONE]\033[0m -> " << midi_out << "\n\n";
    } else {
        std::cout << "\033[1;31m[FAILED]\033[0m\n\n";
    }
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

        // Inject continuous excitation into Poincare disk
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

    std::cout << "\033[1;36m[LIVE SYNTHESIZER READY]\033[0m Audio streaming active at 48kHz stereo.\n";
    std::cout << "Play with keyboard:\n";
    std::cout << "  Notes     : \033[1m[A] [W] [S] [E] [D] [F] [T] [G] [Y] [H] [U] [J] [K]\033[0m\n";
    std::cout << "  Presets   : \033[1m[1] Bell   [2] Gong   [3] Drone   [4] Aether\033[0m\n";
    std::cout << "  Quit      : \033[1;31m[Q] or [ESC]\033[0m\n\n";

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
                    std::cout << "\r\033[K[PRESET]: Hyperbolic Cosmic Bell        ";
                    break;
                case '2':
                    engine.load_preset(aether::Preset::ChladniPlateGong);
                    std::cout << "\r\033[K[PRESET]: Chladni Plate Gong           ";
                    break;
                case '3':
                    engine.load_preset(aether::Preset::ChaoticQuantumDrone);
                    std::cout << "\r\033[K[PRESET]: Chaotic Quantum Drone        ";
                    break;
                case '4':
                    engine.load_preset(aether::Preset::HyperbolicAetherSpace);
                    std::cout << "\r\033[K[PRESET]: Hyperbolic Aether Space     ";
                    break;

                case 'a': case 'A': engine.note_on_degree(0, 0.9f); std::cout << "\r\033[K\033[36m>> Note C3 (Degree 0)\033[0m"; break;
                case 'w': case 'W': engine.note_on_degree(1, 0.9f); std::cout << "\r\033[K\033[36m>> Note C#3 (Degree 1)\033[0m"; break;
                case 's': case 'S': engine.note_on_degree(2, 0.9f); std::cout << "\r\033[K\033[36m>> Note D3 (Degree 2)\033[0m"; break;
                case 'e': case 'E': engine.note_on_degree(3, 0.9f); std::cout << "\r\033[K\033[36m>> Note D#3 (Degree 3)\033[0m"; break;
                case 'd': case 'D': engine.note_on_degree(4, 0.9f); std::cout << "\r\033[K\033[36m>> Note E3 (Degree 4)\033[0m"; break;
                case 'f': case 'F': engine.note_on_degree(5, 0.9f); std::cout << "\r\033[K\033[36m>> Note F3 (Degree 5)\033[0m"; break;
                case 't': case 'T': engine.note_on_degree(6, 0.9f); std::cout << "\r\033[K\033[36m>> Note F#3 (Degree 6)\033[0m"; break;
                case 'g': case 'G': engine.note_on_degree(7, 0.9f); std::cout << "\r\033[K\033[36m>> Note G3 (Degree 7)\033[0m"; break;
                case 'y': case 'Y': engine.note_on_degree(8, 0.9f); std::cout << "\r\033[K\033[36m>> Note G#3 (Degree 8)\033[0m"; break;
                case 'h': case 'H': engine.note_on_degree(9, 0.9f); std::cout << "\r\033[K\033[36m>> Note A3 (Degree 9)\033[0m"; break;
                case 'u': case 'U': engine.note_on_degree(10, 0.9f); std::cout << "\r\033[K\033[36m>> Note A#3 (Degree 10)\033[0m"; break;
                case 'j': case 'J': engine.note_on_degree(11, 0.9f); std::cout << "\r\033[K\033[36m>> Note B3 (Degree 11)\033[0m"; break;
                case 'k': case 'K': engine.note_on_degree(12, 0.9f); std::cout << "\r\033[K\033[36m>> Note C4 (Degree 12)\033[0m"; break;
                case ' ':
                    engine.note_on({(rand() % 100 - 50) / 70.0f, (rand() % 100 - 50) / 70.0f}, 0.95f);
                    std::cout << "\r\033[K\033[1;33m>> Non-Euclidean Random Strike!\033[0m";
                    break;
            }
            std::cout.flush();
        }
#endif
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
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

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--live" || arg == "-l") {
            do_live = true;
            do_bench = false;
            do_render = false;
            do_ascii = false;
        } else if (arg == "--resynthesize" && i + 2 < argc) {
            std::string in_file = argv[++i];
            std::string out_file = argv[++i];
            resynthesize_wav(in_file, out_file);
            return 0;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: aether-dsp [options]\n";
            std::cout << "  --live, -l                       Launch live interactive synthesizer with audio hardware\n";
            std::cout << "  --resynthesize <in.wav> <out.wav> Process and resynthesize WAV file through hyperbolic waveguide\n";
            std::cout << "  --bench                          Run AVX2 SIMD DSP throughput benchmark\n";
            std::cout << "  --render                         Render lossless audio presets & MIDI to output/*\n";
            std::cout << "  --ascii                          Display terminal ASCII Chladni nodal plate\n";
            return 0;
        }
    }

    if (do_live) {
        run_live_interactive();
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
