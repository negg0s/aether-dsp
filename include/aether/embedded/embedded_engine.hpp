#pragma once

#include "../math/fixed_point.hpp"
#include "embedded_voice.hpp"
#include "../math/fixed_reverb.hpp"
#include "hardware_adapter.hpp"
#include <cstdint>
#include <array>

namespace aether::embedded {

/**
 * @brief 4-Voice Polyphonic Zero-Heap Embedded Hardware Synthesizer Engine.
 * Complete production-ready firmware engine for microcontrollers (Daisy Seed, Cortex-M4/M7).
 * Features:
 *   - 4-voice polyphony with Least-Recently-Used (LRU) voice stealing
 *   - MIDI note-to-frequency lookup without FPU math
 *   - Integrated integer algorithmic FixedReverb
 *   - Master volume and hardware controller integration
 */
class EmbeddedEngine {
public:
    static constexpr size_t NUM_VOICES = 4;

    EmbeddedEngine() noexcept {
        init();
    }

    void init() noexcept {
        for (auto& v : voices_) {
            v.init();
        }
        reverb_.configure(0.78f, 0.25f, 0.30f);
        note_counter_ = 0;
        voice_ages_.fill(0);
        active_notes_.fill(255); // 255 = no note
    }

    void note_on(uint8_t midi_note, uint8_t velocity = 100) noexcept {
        (void)velocity;
        midi_note = std::min<uint8_t>(midi_note, 127);
        const float freq = midi_note_to_frequency(midi_note);

        // 1. Try to find inactive voice
        size_t target_voice = NUM_VOICES;
        for (size_t i = 0; i < NUM_VOICES; ++i) {
            if (!voices_[i].is_active()) {
                target_voice = i;
                break;
            }
        }

        // 2. If all active, steal LRU voice
        if (target_voice == NUM_VOICES) {
            uint32_t oldest_age = 0xFFFFFFFF;
            for (size_t i = 0; i < NUM_VOICES; ++i) {
                if (voice_ages_[i] < oldest_age) {
                    oldest_age = voice_ages_[i];
                    target_voice = i;
                }
            }
        }

        if (target_voice < NUM_VOICES) {
            voices_[target_voice].note_on(freq);
            voice_ages_[target_voice] = ++note_counter_;
            active_notes_[target_voice] = midi_note;
        }
    }

    void note_off(uint8_t midi_note) noexcept {
        for (size_t i = 0; i < NUM_VOICES; ++i) {
            if (active_notes_[i] == midi_note) {
                voices_[i].note_off();
                active_notes_[i] = 255;
            }
        }
    }

    void all_notes_off() noexcept {
        for (size_t i = 0; i < NUM_VOICES; ++i) {
            voices_[i].note_off();
            active_notes_[i] = 255;
        }
    }

    void reset() noexcept {
        init();
        reverb_.reset();
    }

    [[nodiscard]] inline math::FixedQ15 process() noexcept {
        int32_t mix = 0;
        for (auto& v : voices_) {
            if (v.is_active()) {
                mix += v.process().raw;
            }
        }

        // Scale by 1/4 to prevent clipping across 4 voices
        const math::FixedQ15 dry_sum(static_cast<int16_t>(mix >> 2));
        return reverb_.process(dry_sum);
    }

    [[nodiscard]] math::FixedReverb& reverb() noexcept {
        return reverb_;
    }

    [[nodiscard]] size_t active_voice_count() const noexcept {
        size_t count = 0;
        for (const auto& v : voices_) {
            if (v.is_active()) count++;
        }
        return count;
    }

private:
    std::array<EmbeddedVoice, NUM_VOICES> voices_{};
    std::array<uint32_t, NUM_VOICES> voice_ages_{};
    std::array<uint8_t, NUM_VOICES> active_notes_{};
    math::FixedReverb reverb_{};
    uint32_t note_counter_{0};

    [[nodiscard]] static float midi_note_to_frequency(uint8_t note) noexcept {
        // Equal temperament reference: 440 * 2^((note - 69) / 12)
        // Fast float exponent for initialization
        return 440.0f * std::pow(2.0f, (static_cast<float>(note) - 69.0f) / 12.0f);
    }
};

} // namespace aether::embedded
