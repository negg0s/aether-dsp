#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include "aether/math/fixed_point.hpp"

namespace aether::embedded {

// Eurorack Precision Voltage Controlled Analog Envelope/CV Slicer & Rhythm Chopper
// Slices continuous incoming audio or CV modulation into rhythmic gated patterns
// with envelope attack/decay smoothing on slice boundaries to prevent audible clicks.
// Zero-heap, zero-allocation bare-metal MCU implementation.
class EurorackCvSlicer {
public:
    EurorackCvSlicer() {
        reset();
    }

    void reset() noexcept {
        pattern_mask_ = 0xAAAA; // default alternating 16-step pattern (1010101010101010)
        current_step_ = 0;
        step_subtick_ = 0;
        smoothing_gain_ = 0;
        smoothing_rate_ = 4096; // fast anti-click crossfade
        gate_active_ = false;
    }

    void set_pattern(uint16_t mask) noexcept {
        pattern_mask_ = mask;
    }

    void set_smoothing_rate(uint16_t rate) noexcept {
        smoothing_rate_ = (rate == 0) ? 1024 : rate;
    }

    // Advance clock trigger edge
    void clock_step() noexcept {
        current_step_ = (current_step_ + 1) & 0x0F;
        gate_active_ = (pattern_mask_ & (1U << current_step_)) != 0;
    }

    // Process a single audio or CV sample through the slicer with de-clicking ramps
    math::FixedQ15 process(math::FixedQ15 input) noexcept {
        // Target amplitude ramp: 32767 if gate active, 0 if sliced off
        int32_t target_gain = gate_active_ ? 32767 : 0;

        if (smoothing_gain_ < target_gain) {
            smoothing_gain_ += smoothing_rate_;
            if (smoothing_gain_ > target_gain) smoothing_gain_ = target_gain;
        } else if (smoothing_gain_ > target_gain) {
            smoothing_gain_ -= smoothing_rate_;
            if (smoothing_gain_ < target_gain) smoothing_gain_ = target_gain;
        }

        int64_t sliced = (static_cast<int64_t>(input.raw) * smoothing_gain_) >> 15;
        return math::FixedQ15(static_cast<int16_t>(sliced));
    }

    [[nodiscard]] bool is_gate_active() const noexcept {
        return gate_active_;
    }

    [[nodiscard]] size_t current_step() const noexcept {
        return current_step_;
    }

private:
    uint16_t pattern_mask_{0xAAAA};
    size_t current_step_{0};
    uint32_t step_subtick_{0};
    int32_t smoothing_gain_{0};
    uint16_t smoothing_rate_{4096};
    bool gate_active_{false};
};

} // namespace aether::embedded
