#pragma once

#include "fixed_point.hpp"
#include <cstdint>
#include <cmath>
#include <algorithm>

namespace aether::math {

/**
 * @brief Fixed-point Q15 Biquad Filter Direct Form II Transposed.
 * Zero-heap, zero-FPU execution optimized for embedded ARM Cortex-M DSPs.
 */
class FixedBiquadFilter {
public:
    constexpr FixedBiquadFilter() noexcept = default;

    void reset() noexcept {
        s1_.raw = 0;
        s2_.raw = 0;
    }

    /**
     * @brief Configure lowpass filter coefficients in Q15 format.
     */
    void configure_lowpass(float cutoff_norm, float q = 0.707f) noexcept {
        cutoff_norm = std::clamp(cutoff_norm, 0.001f, 0.49f);
        q = std::clamp(q, 0.1f, 10.0f);

        const float omega = 6.2831853f * cutoff_norm;
        const float sin_w = std::sin(omega);
        const float cos_w = std::cos(omega);
        const float alpha = sin_w / (2.0f * q);

        const float b0 = (1.0f - cos_w) * 0.5f;
        const float b1 = 1.0f - cos_w;
        const float b2 = (1.0f - cos_w) * 0.5f;
        const float a0 = 1.0f + alpha;
        const float a1 = -2.0f * cos_w;
        const float a2 = 1.0f - alpha;

        const float inv_a0 = 1.0f / a0;
        b0_ = FixedQ15::from_float(b0 * inv_a0);
        b1_ = FixedQ15::from_float(b1 * inv_a0);
        b2_ = FixedQ15::from_float(b2 * inv_a0);
        a1_ = FixedQ15::from_float(a1 * inv_a0 * 0.5f); // Scaled by 0.5 to fit [-1, 1]
        a2_ = FixedQ15::from_float(a2 * inv_a0);
    }

    [[nodiscard]] inline FixedQ15 process(FixedQ15 in) noexcept {
        // Direct Form II Transposed in Q15
        const FixedQ15 out = (in * b0_) + s1_;
        const FixedQ15 a1_term = out * a1_;
        // Multiply by 2 to compensate scaling
        const int32_t a1_double = static_cast<int32_t>(a1_term.raw) << 1;
        const FixedQ15 a1_eff(static_cast<int16_t>(std::clamp<int32_t>(a1_double, -32768, 32767)));

        s1_ = (in * b1_) - a1_eff + s2_;
        s2_ = (in * b2_) - (out * a2_);

        return out;
    }

private:
    FixedQ15 b0_{FixedQ15::from_float(1.0f)};
    FixedQ15 b1_{FixedQ15::from_float(0.0f)};
    FixedQ15 b2_{FixedQ15::from_float(0.0f)};
    FixedQ15 a1_{FixedQ15::from_float(0.0f)};
    FixedQ15 a2_{FixedQ15::from_float(0.0f)};
    FixedQ15 s1_{};
    FixedQ15 s2_{};
};

} // namespace aether::math
