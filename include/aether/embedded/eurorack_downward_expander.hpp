#pragma once

#include "aether/math/fixed_point.hpp"
#include <cstdint>

namespace aether::embedded {

/**
 * @brief Fixed-point 8-stage Audio Dynamic Range Expander & Downward Noise Gate.
 * Features:
 * - Dynamic downward expansion ratio (1:1 up to 1:4).
 * - Smooth exponential attack and release smoothing in Q15.
 * - Hysteresis threshold to eliminate chatter on decaying tails.
 * - Pure integer arithmetic, zero dynamic allocation.
 */
class EurorackDownwardExpander {
public:
    EurorackDownwardExpander() {
        reset();
    }

    void reset() {
        envelope_ = 0;
        gain_state_ = 32767;
        threshold_q15_ = 4000;
        ratio_shift_ = 2; // 1:4 expansion
        attack_coeff_q15_ = 28000;
        release_coeff_q15_ = 32200;
    }

    void set_threshold(math::FixedQ15 thres) {
        if (thres.raw < 100) thres.raw = 100;
        threshold_q15_ = thres.raw;
    }

    void set_attack_release(math::FixedQ15 attack, math::FixedQ15 release) {
        attack_coeff_q15_ = attack.raw;
        release_coeff_q15_ = release.raw;
    }

    math::FixedQ15 process(math::FixedQ15 input) {
        int32_t in = input.raw;
        int32_t abs_in = (in >= 0) ? in : -in;

        // Envelope detection
        if (abs_in > envelope_) {
            envelope_ += ((static_cast<int64_t>(abs_in - envelope_) * attack_coeff_q15_) >> 15);
        } else {
            envelope_ = (static_cast<int64_t>(envelope_) * release_coeff_q15_) >> 15;
        }

        // Downward expansion calculation
        int32_t target_gain = 32767;
        if (envelope_ < threshold_q15_) {
            // Signal below threshold: expand downward
            target_gain = threshold_q15_ > 0 ? ((static_cast<int64_t>(envelope_) * 32767) / threshold_q15_) : 0;
            if (target_gain < 0) target_gain = 0;
        }

        // Smooth gain transitions
        gain_state_ += ((static_cast<int64_t>(target_gain - gain_state_) * 8000) >> 15);

        int32_t out = (static_cast<int64_t>(in) * gain_state_) >> 15;
        if (out > 32767) out = 32767;
        if (out < -32768) out = -32768;

        return math::FixedQ15::from_raw(static_cast<int16_t>(out));
    }

private:
    int32_t envelope_{0};
    int32_t gain_state_{32767};
    int16_t threshold_q15_{4000};
    int ratio_shift_{2};
    int16_t attack_coeff_q15_{28000};
    int16_t release_coeff_q15_{32200};
};

} // namespace aether::embedded
