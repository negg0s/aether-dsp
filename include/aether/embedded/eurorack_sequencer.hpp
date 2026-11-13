#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include "aether/math/fixed_point.hpp"

namespace aether::embedded {

// Eurorack Quad CV/Gate Step Sequencer
// 16 steps per track, 4 tracks (CV pitch, velocity, gate length, probability)
// Zero-heap, zero-allocation deterministic tracker for bare-metal MCU.
class EurorackStepSequencer {
public:
    static constexpr size_t MaxSteps = 16;
    static constexpr size_t NumTracks = 4;

    struct Step {
        int16_t cv_pitch_millivolts{0}; // 0 to 5000 mV (0 - 5 Octaves)
        math::FixedQ15 velocity{math::FixedQ15::from_float(0.8f)};
        uint8_t gate_length_percent{50}; // 1% to 100% of clock step
        uint8_t probability{100};        // 0% to 100% chance to trigger
        bool active{true};
    };

    struct StepOutput {
        int16_t cv_millivolts{0};
        math::FixedQ15 velocity{0};
        bool gate{false};
    };

    EurorackStepSequencer() {
        reset();
    }

    void reset() noexcept {
        current_step_ = 0;
        step_subtick_ = 0;
        gate_state_ = false;
        prng_state_ = 0x12345678;
        for (auto& track : tracks_) {
            for (auto& step : track) {
                step.cv_pitch_millivolts = 0;
                step.velocity = math::FixedQ15::from_float(0.8f);
                step.gate_length_percent = 50;
                step.probability = 100;
                step.active = true;
            }
        }
    }

    void set_step(size_t track, size_t step_idx, int16_t cv_mv, math::FixedQ15 vel, uint8_t gate_pct, uint8_t prob) noexcept {
        if (track >= NumTracks || step_idx >= MaxSteps) return;
        tracks_[track][step_idx].cv_pitch_millivolts = cv_mv;
        tracks_[track][step_idx].velocity = vel;
        tracks_[track][step_idx].gate_length_percent = (gate_pct > 100) ? 100 : gate_pct;
        tracks_[track][step_idx].probability = (prob > 100) ? 100 : prob;
        tracks_[track][step_idx].active = true;
    }

    // Advance clock tick (external clock edge or internal timer).
    // subticks_per_step defines gate resolution (e.g., 24 ticks per quarter note).
    StepOutput tick(bool clock_edge, size_t track = 0, uint32_t subticks_per_step = 24) noexcept {
        if (track >= NumTracks) return {};

        if (clock_edge) {
            current_step_ = (current_step_ + 1) % sequence_length_;
            step_subtick_ = 0;

            const auto& step = tracks_[track][current_step_];
            if (step.active && (step.probability >= 100 || (next_random() % 100) < step.probability)) {
                gate_state_ = true;
                current_pitch_mv_ = step.cv_pitch_millivolts;
                current_vel_ = step.velocity;
            } else {
                gate_state_ = false;
            }
        } else {
            step_subtick_++;
            const auto& step = tracks_[track][current_step_];
            uint32_t gate_off_tick = (subticks_per_step * step.gate_length_percent) / 100;
            if (step_subtick_ >= gate_off_tick) {
                gate_state_ = false;
            }
        }

        return {current_pitch_mv_, current_vel_, gate_state_};
    }

    void set_sequence_length(size_t length) noexcept {
        if (length > 0 && length <= MaxSteps) {
            sequence_length_ = length;
            if (current_step_ >= sequence_length_) {
                current_step_ = 0;
            }
        }
    }

    [[nodiscard]] size_t current_step() const noexcept {
        return current_step_;
    }

private:
    uint32_t next_random() noexcept {
        prng_state_ ^= (prng_state_ << 13);
        prng_state_ ^= (prng_state_ >> 17);
        prng_state_ ^= (prng_state_ << 5);
        return prng_state_;
    }

    std::array<std::array<Step, MaxSteps>, NumTracks> tracks_{};
    size_t sequence_length_{16};
    size_t current_step_{0};
    uint32_t step_subtick_{0};
    bool gate_state_{false};
    int16_t current_pitch_mv_{0};
    math::FixedQ15 current_vel_{0};
    uint32_t prng_state_{0x12345678};
};

} // namespace aether::embedded
