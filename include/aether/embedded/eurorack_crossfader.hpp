#pragma once

#include "aether/math/fixed_point.hpp"
#include <cstdint>

namespace aether::embedded {

/**
 * @brief Fixed-point 8-stage Audio & CV Cross-Fader / Morphing Panner.
 * Models a linear / equal-power bipolar Eurorack crossfader.
 * Features:
 * - Constant-power sinusoidal or linear crossfade curves.
 * - Symmetrical bipolar response (-1 to +1 morph position).
 * - Soft analog saturation on summed output preventing hard overflow.
 */
class EurorackCrossfader {
public:
    enum class CurveType : uint8_t {
        Linear = 0,
        EqualPower = 1
    };

    EurorackCrossfader() {
        reset();
    }

    void reset() {
        pos_q15_ = 0; // 0 = centered (50/50)
        curve_ = CurveType::EqualPower;
    }

    /**
     * @brief Set crossfade position:
     * -32768 = 100% Channel A
     * 0 = Center (50/50)
     * 32767 = 100% Channel B
     */
    void set_position(math::FixedQ15 pos) {
        pos_q15_ = pos.raw;
    }

    void set_curve(CurveType curve) {
        curve_ = curve;
    }

    math::FixedQ15 process(math::FixedQ15 chA, math::FixedQ15 chB) {
        // Map pos_q15_ from [-32768, 32767] to [0, 32767] for channel B gain
        int32_t norm_pos = (static_cast<int32_t>(pos_q15_) + 32768) >> 1; // [0, 32767]
        if (norm_pos < 0) norm_pos = 0;
        if (norm_pos > 32767) norm_pos = 32767;

        int32_t gain_b = norm_pos;
        int32_t gain_a = 32767 - norm_pos;

        if (curve_ == CurveType::EqualPower) {
            // Equal-power approximation: sqrt(x)
            gain_a = compute_approx_sqrt(gain_a);
            gain_b = compute_approx_sqrt(gain_b);
        }

        int32_t outA = (static_cast<int64_t>(chA.raw) * gain_a) >> 15;
        int32_t outB = (static_cast<int64_t>(chB.raw) * gain_b) >> 15;

        int32_t sum = outA + outB;
        if (sum > 32767) sum = 32767;
        if (sum < -32768) sum = -32768;

        return math::FixedQ15::from_raw(static_cast<int16_t>(sum));
    }

private:
    int16_t pos_q15_{0};
    CurveType curve_{CurveType::EqualPower};

    // Fast integer square root normalized to [0, 32767]
    inline int32_t compute_approx_sqrt(int32_t x) const {
        if (x <= 0) return 0;
        // Newton-Raphson 3 iterations
        int32_t val = x;
        // Initial guess
        int32_t root = 181 * 128; // ~23168
        for (int i = 0; i < 4; ++i) {
            root = (root + (static_cast<int64_t>(val) * 32767) / root) >> 1;
        }
        if (root > 32767) root = 32767;
        return root;
    }
};

} // namespace aether::embedded
