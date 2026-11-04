#pragma once

#include "types.hpp"
#include "dsp/envelope.hpp"
#include "dsp/tuning.hpp"
#include "engines/chladni_plate.hpp"
#include "engines/pseudosphere_horn.hpp"
#include <array>
#include <vector>
#include <span>
#include <algorithm>

namespace aether {

/**
 * @brief Polyphonic Physical Resonator Voice.
 */
struct PolyVoice {
    int midi_note{-1};
    float frequency{220.0f};
    float velocity{0.0f};
    uint64_t age{0};
    bool active{false};

    engines::ChladniPlate resonator;
    dsp::ADSREnvelope envelope;

    void note_on(int note, float freq, float vel, float sample_rate, uint64_t current_age) noexcept {
        midi_note = note;
        frequency = freq;
        velocity = vel;
        age = current_age;
        active = true;

        resonator.configure(frequency, 1.15f, sample_rate);
        resonator.strike({0.35f, 0.40f}, velocity);

        envelope.configure(0.005f, 0.4f, 0.2f, 1.2f, sample_rate);
        envelope.reset();
        envelope.gate(true);
    }

    void note_off() noexcept {
        envelope.gate(false);
    }

    StereoFrame step() noexcept {
        if (!active) return StereoFrame{0.0f, 0.0f};

        const float env = envelope.process();
        if (!envelope.is_active()) {
            active = false;
            midi_note = -1;
            return StereoFrame{0.0f, 0.0f};
        }

        const auto res = resonator.step({-0.4f, -0.4f}, {0.4f, 0.4f});
        return res * (env * velocity);
    }
};

/**
 * @brief Zero-Allocation Polyphonic Voice Manager (8-Voice Pool with LRU Stealing).
 */
template <size_t NUM_VOICES = 8>
class PolyphonyManager {
public:
    explicit PolyphonyManager(uint32_t sample_rate = DEFAULT_SAMPLE_RATE)
        : sample_rate_(sample_rate) {}

    void note_on(int note, float freq, float velocity) noexcept {
        ++age_counter_;

        // 1. Check if same note is already sounding (re-trigger)
        for (auto& v : voices_) {
            if (v.active && v.midi_note == note) {
                v.note_on(note, freq, velocity, static_cast<float>(sample_rate_), age_counter_);
                return;
            }
        }

        // 2. Find free voice
        for (auto& v : voices_) {
            if (!v.active) {
                v.note_on(note, freq, velocity, static_cast<float>(sample_rate_), age_counter_);
                return;
            }
        }

        // 3. Voice stealing: steal oldest voice (smallest age)
        size_t oldest_idx = 0;
        uint64_t min_age = UINT64_MAX;
        for (size_t i = 0; i < NUM_VOICES; ++i) {
            if (voices_[i].age < min_age) {
                min_age = voices_[i].age;
                oldest_idx = i;
            }
        }

        voices_[oldest_idx].note_on(note, freq, velocity, static_cast<float>(sample_rate_), age_counter_);
    }

    void note_off(int note) noexcept {
        for (auto& v : voices_) {
            if (v.active && v.midi_note == note) {
                v.note_off();
            }
        }
    }

    void all_notes_off() noexcept {
        for (auto& v : voices_) {
            v.note_off();
        }
    }

    StereoFrame step() noexcept {
        StereoFrame sum{0.0f, 0.0f};
        int active_count = 0;

        for (auto& v : voices_) {
            if (v.active) {
                sum = sum + v.step();
                ++active_count;
            }
        }

        if (active_count > 1) {
            // Soft normalization scaling to prevent clipping on dense chords
            const float scale = 1.0f / std::sqrt(static_cast<float>(active_count));
            sum = sum * scale;
        }

        return sum;
    }

    [[nodiscard]] size_t active_voices() const noexcept {
        size_t count = 0;
        for (const auto& v : voices_) {
            if (v.active) ++count;
        }
        return count;
    }

private:
    uint32_t sample_rate_{DEFAULT_SAMPLE_RATE};
    uint64_t age_counter_{0};
    std::array<PolyVoice, NUM_VOICES> voices_{};
};

} // namespace aether
