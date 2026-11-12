#pragma once

#include "fixed_point.hpp"
#include <cstdint>
#include <array>
#include <algorithm>

namespace aether::math {

/**
 * FixedClipper
 * Asymmetric soft-saturation and Germanium/Silicon diode transfer curve in Q15:
 * - Emulates guitar overdrive and tube preamp saturation.
 * - Positive and negative threshold asymmetry (even-harmonic generation).
 * - Zero FPU, zero transcendental functions.
 */
class FixedClipper {
public:
    constexpr FixedClipper() noexcept = default;

    /**
     * configure
     * drive_q15: pre-gain in Q15 (32768 = 1.0x, up to 163840 = 5.0x)
     * asymmetry: positive bias shift in Q15 [-16384, 16384] for rich 2nd-order harmonics
     */
    void configure(int32_t drive_q15, int16_t asymmetry) noexcept {
        drive_ = std::clamp<int32_t>(drive_q15, 1000, 262144);
        asymmetry_ = std::clamp<int16_t>(asymmetry, -16384, 16384);
    }

    [[nodiscard]] FixedQ15 process(FixedQ15 input) const noexcept {
        // Apply drive and asymmetric offset
        int64_t driven = ((static_cast<int64_t>(input.raw) * drive_) >> 15) + asymmetry_;

        // Soft rational transfer function: f(x) = x / (1 + |x|)
        // In Q15: y = (x * 32768) / (32768 + |x|)
        int64_t abs_x = std::abs(driven);
        int64_t denom = 32768 + abs_x;
        int64_t num = driven * 32768;
        int32_t saturated = static_cast<int32_t>(num / (denom > 0 ? denom : 1));

        // Compensate for DC asymmetry offset slightly to keep audio centered
        saturated -= (asymmetry_ >> 1);
        saturated = std::clamp<int32_t>(saturated, -32768, 32767);

        return FixedQ15::from_raw(static_cast<int16_t>(saturated));
    }

private:
    int32_t drive_{32768}; // 1.0x
    int16_t asymmetry_{0};
};

} // namespace aether::math
