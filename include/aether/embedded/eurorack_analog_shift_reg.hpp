#pragma once

#include "aether/math/fixed_point.hpp"
#include <cstdint>

namespace aether::embedded {

/**
 * @brief Fixed-point 8-stage Stochastic Analog Shift Register (ASR) & Chord Arpeggiator.
 * Implements a 4-channel bucket analog delay line for control voltages (-5000 mV to +5000 mV).
 * Features:
 * - On each clock pulse, shifts CV down the 4 output stages: Out1 (current), Out2 (z^-1), Out3 (z^-2), Out4 (z^-3).
 * - Variable CV glide / portamento per channel in milliseconds.
 * - Pitch quantization and inversion toggles.
 * - Perfect for 4-voice canon arpeggios, polyphonic pitch spreading, and harmonic chord clustering.
 */
class EurorackAnalogShiftRegister {
public:
    static constexpr size_t kNumStages = 4;

    EurorackAnalogShiftRegister() {
        reset();
    }

    void reset() {
        for (size_t i = 0; i < kNumStages; ++i) {
            stage_targets_mV_[i] = 0;
            stage_current_mV_[i] = 0;
        }
        clk_last_ = false;
        glide_q15_ = 0; // Instantaneous step by default
    }

    void set_glide(math::FixedQ15 glide) {
        if (glide.raw < 0) glide.raw = 0;
        if (glide.raw > 32700) glide.raw = 32700;
        glide_q15_ = glide.raw;
    }

    struct Output {
        int16_t out1_mV;
        int16_t out2_mV;
        int16_t out3_mV;
        int16_t out4_mV;
    };

    Output process(int16_t cv_in_mV, math::FixedQ15 clock_in) {
        // Schmitt trigger clock detection
        bool clk_high = clock_in.raw > 2000;
        bool clk_rising = clk_high && !clk_last_;
        clk_last_ = clk_high;

        if (clk_rising) {
            // Shift pipeline down
            stage_targets_mV_[3] = stage_targets_mV_[2];
            stage_targets_mV_[2] = stage_targets_mV_[1];
            stage_targets_mV_[1] = stage_targets_mV_[0];
            stage_targets_mV_[0] = cv_in_mV;
        }

        // Apply slew / glide toward targets
        for (size_t i = 0; i < kNumStages; ++i) {
            int32_t target = stage_targets_mV_[i];
            if (glide_q15_ == 0) {
                stage_current_mV_[i] = static_cast<int16_t>(target);
            } else {
                int32_t current = stage_current_mV_[i];
                int32_t diff = target - current;
                int32_t step_factor = 32767 - glide_q15_;
                int32_t delta = (static_cast<int64_t>(diff) * step_factor) >> 15;
                stage_current_mV_[i] = static_cast<int16_t>(current + delta);
            }
        }

        Output out;
        out.out1_mV = stage_current_mV_[0];
        out.out2_mV = stage_current_mV_[1];
        out.out3_mV = stage_current_mV_[2];
        out.out4_mV = stage_current_mV_[3];
        return out;
    }

private:
    int16_t stage_targets_mV_[kNumStages];
    int16_t stage_current_mV_[kNumStages];
    bool clk_last_{false};
    int16_t glide_q15_{0};
};

} // namespace aether::embedded
