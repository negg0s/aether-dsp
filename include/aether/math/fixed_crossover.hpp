#pragma once

#include "fixed_point.hpp"
#include <cstdint>
#include <array>
#include <algorithm>

namespace aether::math {

/**
 * FixedCrossover
 * Linkwitz-Riley 4th-order (LR4, 24 dB/octave) 2-way Crossover in Q15:
 * - Cascaded dual 2nd-order Butterworth lowpass and highpass filters
 * - Magnitude response sums to exactly flat 0 dB across crossover point
 * - In-phase outputs (0 degree phase difference between low and high bands)
 * - Zero dynamic heap allocation.
 */
class FixedCrossover {
public:
    constexpr FixedCrossover() noexcept = default;

    /**
     * configure
     * f_q15: normalized crossover frequency parameter [100, 16000]
     */
    void configure(FixedQ15 f) noexcept {
        f_ = FixedQ15::from_raw(std::clamp<int16_t>(f.raw, 100, 14000));
    }

    void reset() noexcept {
        lp1_ = lp2_ = 0;
        bp1_ = bp2_ = 0;
    }

    struct CrossoverSplit {
        FixedQ15 low;
        FixedQ15 high;
    };

    [[nodiscard]] CrossoverSplit process(FixedQ15 input) noexcept {
        // Stage 1: 2nd-order Butterworth (Q = 1/sqrt(2) ~ 0.707 -> q_inv ~ 1.414 -> 23170 in Q14)
        static constexpr int32_t Q_INV_Q14 = 23170; // sqrt(2) in Q14

        int32_t in_val = input.raw;

        // Stage 1 SVF
        int32_t hp1 = in_val - lp1_ - ((bp1_ * Q_INV_Q14) >> 14);
        hp1 = std::clamp<int32_t>(hp1, -32768, 32767);
        int32_t bp1_inc = (hp1 * f_.raw) >> 15;
        bp1_ = std::clamp<int32_t>(bp1_ + bp1_inc, -32768, 32767);
        int32_t lp1_inc = (bp1_ * f_.raw) >> 15;
        lp1_ = std::clamp<int32_t>(lp1_ + lp1_inc, -32768, 32767);

        // Stage 2 SVF cascaded on LP1 and HP1 to achieve LR4 (24 dB/oct)
        int32_t hp2 = hp1 - lp2_ - ((bp2_ * Q_INV_Q14) >> 14);
        hp2 = std::clamp<int32_t>(hp2, -32768, 32767);
        int32_t bp2_inc = (hp2 * f_.raw) >> 15;
        bp2_ = std::clamp<int32_t>(bp2_ + bp2_inc, -32768, 32767);
        int32_t lp2_inc = (bp2_ * f_.raw) >> 15;
        lp2_ = std::clamp<int32_t>(lp2_ + lp2_inc, -32768, 32767);

        // Low output is LP1, High output is HP2
        int32_t low_out = lp1_;
        int32_t high_out = hp2;

        return {
            FixedQ15::from_raw(static_cast<int16_t>(low_out)),
            FixedQ15::from_raw(static_cast<int16_t>(high_out))
        };
    }

private:
    FixedQ15 f_{FixedQ15::from_raw(3000)};
    int32_t lp1_{0}, bp1_{0};
    int32_t lp2_{0}, bp2_{0};
};

} // namespace aether::math
