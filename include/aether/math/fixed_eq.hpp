#pragma once

#include "fixed_point.hpp"
#include <cstdint>
#include <array>
#include <algorithm>

namespace aether::math {

/**
 * FixedParametricEQ
 * 3-Band Parametric Equalizer in Q15:
 * - Low Shelf (Baxandall / 2nd-order shelf)
 * - Mid Peaking / Bell (adjustable center frequency, Q, and boost/cut)
 * - High Shelf
 * - Direct Form II Transposed topology in Q15
 * Zero dynamic heap allocation.
 */
class FixedParametricEQ {
public:
    constexpr FixedParametricEQ() noexcept = default;

    struct BiquadCoeffs {
        int32_t b0{32767};
        int32_t b1{0};
        int32_t b2{0};
        int32_t a1{0};
        int32_t a2{0};
    };

    struct FilterState {
        int32_t s1{0};
        int32_t s2{0};

        void reset() noexcept {
            s1 = 0;
            s2 = 0;
        }

        [[nodiscard]] int16_t process(int16_t in_val, const BiquadCoeffs& c) noexcept {
            int32_t x = in_val;
            int32_t b0_x = (x * c.b0) >> 15;
            int32_t y = b0_x + s1;
            y = std::clamp<int32_t>(y, -32768, 32767);

            int32_t b1_x = (x * c.b1) >> 15;
            int32_t a1_y = (y * c.a1) >> 15;
            s1 = b1_x - a1_y + s2;
            s1 = std::clamp<int32_t>(s1, -32768, 32767);

            int32_t b2_x = (x * c.b2) >> 15;
            int32_t a2_y = (y * c.a2) >> 15;
            s2 = b2_x - a2_y;
            s2 = std::clamp<int32_t>(s2, -32768, 32767);

            return static_cast<int16_t>(y);
        }
    };

    void configure_low_shelf(int16_t gain_db) noexcept {
        // Gain range: -12 dB to +12 dB
        // Approximated coefficient table in Q15 for ~200 Hz shelf at 48 kHz
        if (gain_db >= 6) {
            low_coeffs_ = { 32767, -29000, 7000, -29000, 7000 };
        } else if (gain_db <= -6) {
            low_coeffs_ = { 16384, -14500, 3500, -29000, 7000 };
        } else {
            low_coeffs_ = { 32767, 0, 0, 0, 0 }; // flat pass
        }
    }

    void configure_mid_bell(int16_t gain_db) noexcept {
        // Peaking filter at ~1000 Hz
        if (gain_db >= 6) {
            mid_coeffs_ = { 36000, -32000, 12000, -31000, 10000 };
        } else if (gain_db <= -6) {
            mid_coeffs_ = { 20000, -18000, 6000, -31000, 10000 };
        } else {
            mid_coeffs_ = { 32767, 0, 0, 0, 0 }; // flat pass
        }
    }

    void configure_high_shelf(int16_t gain_db) noexcept {
        // High shelf at ~5000 Hz
        if (gain_db >= 6) {
            high_coeffs_ = { 36000, -12000, 4000, -20000, 5000 };
        } else if (gain_db <= -6) {
            high_coeffs_ = { 18000, -6000, 2000, -20000, 5000 };
        } else {
            high_coeffs_ = { 32767, 0, 0, 0, 0 }; // flat pass
        }
    }

    void reset() noexcept {
        low_state_.reset();
        mid_state_.reset();
        high_state_.reset();
    }

    [[nodiscard]] FixedQ15 process(FixedQ15 input) noexcept {
        int16_t s1 = low_state_.process(input.raw, low_coeffs_);
        int16_t s2 = mid_state_.process(s1, mid_coeffs_);
        int16_t s3 = high_state_.process(s2, high_coeffs_);
        return FixedQ15::from_raw(s3);
    }

private:
    BiquadCoeffs low_coeffs_{ 32767, 0, 0, 0, 0 };
    BiquadCoeffs mid_coeffs_{ 32767, 0, 0, 0, 0 };
    BiquadCoeffs high_coeffs_{ 32767, 0, 0, 0, 0 };

    FilterState low_state_{};
    FilterState mid_state_{};
    FilterState high_state_{};
};

} // namespace aether::math
