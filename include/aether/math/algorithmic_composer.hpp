#pragma once

#include "../types.hpp"
#include "../engine.hpp"
#include <vector>
#include <array>
#include <algorithm>
#include <cmath>
#include <numbers>

namespace aether::math {

struct NoteEvent {
    double timestamp_sec{0.0};
    Preset preset{Preset::HyperbolicCosmicBell};
    Vec2 strike_coord{0.0f, 0.0f};
    float velocity{0.8f};
    float fundamental_freq{220.0f};
};

/**
 * @brief Algorithmic Generative Counterpoint Engine (Zero AI, Pure Math).
 *
 * Utilizes Iannis Xenakis Sieve Theory, Fibonacci modulo arithmetic,
 * and Golden Ratio (phi) temporal spacing to generate deterministic polyphonic scores.
 */
class AlgorithmicComposer {
public:
    static constexpr double PHI = 1.6180339887498948482; // Golden Ratio

    /**
     * @brief Generate a deterministic algorithmic score based on Fibonacci & Golden Ratio series.
     * @param duration_sec Length of composition in seconds
     * @return List of timestamped polyphonic note events
     */
    static std::vector<NoteEvent> generate_fibonacci_score(double duration_sec = 8.0) {
        std::vector<NoteEvent> score;

        // Base Fibonacci sequence
        constexpr int FIB[] = {1, 2, 3, 5, 8, 13, 21, 34, 55, 89};

        // Pentatonic / Inharmonic non-Euclidean scale based on Golden Ratio powers
        // f_k = 110 * phi^(k / 5)
        auto get_scale_freq = [](int step) -> float {
            return static_cast<float>(110.0 * std::pow(PHI, (step % 12) * 0.4));
        };

        double current_time = 0.0;
        int step = 0;

        while (current_time < duration_sec) {
            const int fib_val = FIB[step % 10];

            // Xenakis Sieve: (3k + 1) U (5k + 2) selection
            const bool is_hyperbolic = ((step % 3 == 1) || (step % 5 == 2));
            const Preset preset = is_hyperbolic ? Preset::HyperbolicCosmicBell : Preset::ChladniPlateGong;

            // Spiral coordinate inside unit disk: r = sqrt(step / N), theta = 2 * pi * phi * step
            const float r = std::min(0.85f, 0.15f + 0.06f * (step % 10));
            const float theta = static_cast<float>(constants::TWO_PI * (step * PHI));
            const Vec2 coord{r * std::cos(theta), r * std::sin(theta)};

            const float vel = 0.65f + 0.3f * std::sin(static_cast<float>(step * 0.7));
            const float freq = get_scale_freq(fib_val + step);

            score.push_back(NoteEvent{
                .timestamp_sec = current_time,
                .preset = preset,
                .strike_coord = coord,
                .velocity = vel,
                .fundamental_freq = freq
            });

            // Golden ratio rhythmic subdivision
            const double delta = 0.25 + 0.5 * std::abs(std::fmod(step * PHI, 1.0));
            current_time += delta;
            step++;
        }

        // Add periodic deep chaotic drone pulses in the background
        for (double t = 0.0; t < duration_sec; t += 2.5) {
            score.push_back(NoteEvent{
                .timestamp_sec = t,
                .preset = Preset::ChaoticQuantumDrone,
                .strike_coord = {0.0f, 0.0f},
                .velocity = 0.5f,
                .fundamental_freq = 55.0f
            });
        }

        std::stable_sort(score.begin(), score.end(), [](const NoteEvent& a, const NoteEvent& b) {
            return a.timestamp_sec < b.timestamp_sec;
        });

        return score;
    }

    /**
     * @brief Render an algorithmic score directly to a stereo frame buffer.
     */
    static std::vector<StereoFrame> render_score(const std::vector<NoteEvent>& events,
                                                 double duration_sec,
                                                 uint32_t sample_rate = DEFAULT_SAMPLE_RATE) {
        if (duration_sec <= 0.0) return {};
        const size_t total_samples = static_cast<size_t>(duration_sec * sample_rate);
        std::vector<StereoFrame> output(total_samples, StereoFrame{0.0f, 0.0f});

        std::vector<NoteEvent> sorted_events = events;
        std::stable_sort(sorted_events.begin(), sorted_events.end(), [](const NoteEvent& a, const NoteEvent& b) {
            return a.timestamp_sec < b.timestamp_sec;
        });

        Engine engine(sample_rate);
        constexpr size_t BLOCK_SIZE = 64;
        std::array<StereoFrame, BLOCK_SIZE> block{};

        size_t event_idx = 0;
        const size_t total_events = sorted_events.size();

        for (size_t sample_pos = 0; sample_pos < total_samples; sample_pos += BLOCK_SIZE) {
            const double current_sec = static_cast<double>(sample_pos) / sample_rate;

            // Trigger events scheduled in this block
            while (event_idx < total_events && sorted_events[event_idx].timestamp_sec <= current_sec) {
                const auto& ev = sorted_events[event_idx];
                engine.load_preset(ev.preset);
                if (ev.preset == Preset::ChladniPlateGong) {
                    engine.chladni().configure(ev.fundamental_freq, 1.1f, static_cast<float>(sample_rate));
                }
                engine.note_on(ev.strike_coord, ev.velocity);
                event_idx++;
            }

            const size_t to_render = std::min(BLOCK_SIZE, total_samples - sample_pos);
            engine.process_block(std::span<StereoFrame>(block.data(), to_render));

            for (size_t i = 0; i < to_render; ++i) {
                output[sample_pos + i] = block[i];
            }
        }

        return output;
    }
};

} // namespace aether::math
