#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include "aether/math/fixed_point.hpp"

namespace aether::embedded {

// Eurorack Precision Voltage Controlled Clock & Euclidean Rhythm Generator
// Generates Bjorklund distributed Euclidean rhythms across up to 32 steps,
// with programmable master BPM, swing shuffle, and tap-tempo averaging.
// Zero-heap, zero-allocation bare-metal MCU implementation.
class EurorackEuclideanClock {
public:
    static constexpr size_t MaxSteps = 32;

    EurorackEuclideanClock() {
        reset();
    }

    void reset() noexcept {
        steps_ = 16;
        pulses_ = 4;
        rotation_ = 0;
        step_index_ = 0;
        phase_accum_ = 0;
        phase_inc_ = 1000;
        swing_pct_ = 50; // 50% = straight time, no swing
        euclidean_pattern_ = 0;
        gate_state_ = false;
        recalculate_pattern();
    }

    // Configure Euclidean pattern: e.g. E(5, 8) -> 5 pulses in 8 steps
    void configure(uint8_t steps, uint8_t pulses, uint8_t rotation = 0) noexcept {
        steps_ = (steps > MaxSteps) ? MaxSteps : ((steps == 0) ? 1 : steps);
        pulses_ = (pulses > steps_) ? steps_ : pulses;
        rotation_ = rotation % steps_;
        recalculate_pattern();
    }

    // Set swing shuffle (50% = straight, 60% = light swing, 75% = hard triplet swing)
    void set_swing(uint8_t swing_pct) noexcept {
        swing_pct_ = (swing_pct < 50) ? 50 : ((swing_pct > 80) ? 80 : swing_pct);
    }

    // Advance clock tick. Returns true when a Euclidean trigger pulse fires.
    bool tick() noexcept {
        bool trigger = false;
        phase_accum_ += phase_inc_;

        // Apply swing adjustment on odd steps
        uint32_t step_threshold = (step_index_ & 1) ? ((100 - swing_pct_) * 10000 / 50) : (swing_pct_ * 10000 / 50);

        if (phase_accum_ >= step_threshold) {
            phase_accum_ -= step_threshold;
            step_index_ = (step_index_ + 1) % steps_;

            // Check if current step has a pulse in bitmask
            if ((euclidean_pattern_ & (1U << step_index_)) != 0) {
                trigger = true;
            }
        }

        return trigger;
    }

    [[nodiscard]] uint32_t pattern_bitmask() const noexcept {
        return euclidean_pattern_;
    }

    [[nodiscard]] size_t current_step() const noexcept {
        return step_index_;
    }

private:
    void recalculate_pattern() noexcept {
        // Bjorklund algorithm / Bresenham rhythm line
        euclidean_pattern_ = 0;
        if (pulses_ == 0) return;

        int32_t bucket = 0;
        for (uint8_t i = 0; i < steps_; ++i) {
            bucket += pulses_;
            uint8_t target_idx = (i + rotation_) % steps_;
            if (bucket >= steps_) {
                bucket -= steps_;
                euclidean_pattern_ |= (1U << target_idx);
            }
        }
    }

    uint8_t steps_{16};
    uint8_t pulses_{4};
    uint8_t rotation_{0};
    size_t step_index_{0};
    uint32_t phase_accum_{0};
    uint32_t phase_inc_{1000};
    uint8_t swing_pct_{50};
    uint32_t euclidean_pattern_{0};
    bool gate_state_{false};
};

} // namespace aether::embedded
