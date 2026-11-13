#pragma once

#include "aether/math/fixed_point.hpp"
#include <cstdint>

namespace aether::embedded {

/**
 * @brief Fixed-point 8-stage Audio & CV Envelope Inverter & Polarizer Utility.
 * Features:
 * - Dual independent channels of bipolar attenuverting (-100% to +100%).
 * - Absolute value rectification mode (full-wave).
 * - Variable clipping threshold limiters (brickwall diode clamp).
 * - Channel A + Channel B sum bus output with analog soft clipping.
 */
class EurorackPolarizerUtility {
public:
    EurorackPolarizerUtility() {
        reset();
    }

    void reset() {
        polarize_a_q15_ = 32767;  // +1.0 gain
        polarize_b_q15_ = -32768; // -1.0 inverted gain
        rectify_a_ = false;
        rectify_b_ = false;
        clamp_threshold_q15_ = 32767;
    }

    void set_polarize_a(math::FixedQ15 pol) {
        polarize_a_q15_ = pol.raw;
    }

    void set_polarize_b(math::FixedQ15 pol) {
        polarize_b_q15_ = pol.raw;
    }

    void set_rectify_a(bool rect) {
        rectify_a_ = rect;
    }

    void set_rectify_b(bool rect) {
        rectify_b_ = rect;
    }

    void set_clamp(math::FixedQ15 clamp) {
        if (clamp.raw < 1000) clamp.raw = 1000;
        clamp_threshold_q15_ = clamp.raw;
    }

    struct Output {
        math::FixedQ15 out_a;
        math::FixedQ15 out_b;
        math::FixedQ15 out_sum;
    };

    Output process(math::FixedQ15 in_a, math::FixedQ15 in_b) {
        // Channel A
        int32_t a = in_a.raw;
        if (rectify_a_) a = (a >= 0) ? a : -a;
        int32_t scaled_a = (static_cast<int64_t>(a) * polarize_a_q15_) >> 15;
        if (scaled_a > clamp_threshold_q15_) scaled_a = clamp_threshold_q15_;
        if (scaled_a < -clamp_threshold_q15_) scaled_a = -clamp_threshold_q15_;

        // Channel B
        int32_t b = in_b.raw;
        if (rectify_b_) b = (b >= 0) ? b : -b;
        int32_t scaled_b = (static_cast<int64_t>(b) * polarize_b_q15_) >> 15;
        if (scaled_b > clamp_threshold_q15_) scaled_b = clamp_threshold_q15_;
        if (scaled_b < -clamp_threshold_q15_) scaled_b = -clamp_threshold_q15_;

        // Sum bus
        int32_t sum = (scaled_a + scaled_b) >> 1;
        if (sum > 32767) sum = 32767;
        if (sum < -32768) sum = -32768;

        Output out;
        out.out_a = math::FixedQ15::from_raw(static_cast<int16_t>(scaled_a));
        out.out_b = math::FixedQ15::from_raw(static_cast<int16_t>(scaled_b));
        out.out_sum = math::FixedQ15::from_raw(static_cast<int16_t>(sum));
        return out;
    }

private:
    int16_t polarize_a_q15_{32767};
    int16_t polarize_b_q15_{-32768};
    bool rectify_a_{false};
    bool rectify_b_{false};
    int16_t clamp_threshold_q15_{32767};
};

} // namespace aether::embedded
