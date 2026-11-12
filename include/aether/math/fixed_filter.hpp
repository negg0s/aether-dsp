#pragma once

#include "fixed_point.hpp"
#include <cstdint>
#include <array>
#include <algorithm>

namespace aether::math {

/**
 * FixedNonlinearFilter (Virtual Analog Ladder / Diode Clipper)
 * Implements a 2nd-order nonlinear fixed-point resonant filter in Q15.
 * Uses rational polynomial saturator (approx tanh) in feedback loop
 * to emulate warm analog saturation without transcendental FPU calls.
 * 100% deterministic, zero-heap, suitable for Cortex-M3/M4/M7 and DSP MCUs.
 */
class FixedNonlinearFilter {
public:
    constexpr FixedNonlinearFilter() noexcept = default;

    /**
     * Configure cutoff and resonance:
     * cutoff: Q15 in range [0, 32767] (normalized cutoff freq)
     * resonance: Q15 in range [0, 32767] (Q factor feedback intensity)
     */
    void configure(FixedQ15 cutoff, FixedQ15 resonance) noexcept {
        // Clamp cutoff to prevent instability at Nyquist
        int16_t c_raw = std::clamp<int16_t>(cutoff.raw, 10, 30000);
        cutoff_ = FixedQ15::from_raw(c_raw);

        // Feedback resonance limit: up to ~0.95 to keep filter self-oscillation bounded
        int16_t r_raw = std::clamp<int16_t>(resonance.raw, 0, 31000);
        resonance_ = FixedQ15::from_raw(r_raw);
    }

    void reset() noexcept {
        s1_ = 0;
        s2_ = 0;
    }

    /**
     * Process a single audio sample in Q15 with nonlinear feedback clipping.
     */
    [[nodiscard]] FixedQ15 process(FixedQ15 input) noexcept {
        // Rational saturator: f(x) = x * (27 + x^2) / (27 + 9 * x^2) approx tanh
        // For fast fixed-point MCU DSP: cubic soft-clipper x - (x^3)/3
        int64_t in_val = input.raw;

        // Feedback from state 2 with resonance
        int64_t fb = (s2_ * resonance_.raw) >> 15;
        // Saturate feedback to analog drive level
        fb = saturate_soft(fb);

        int64_t drive = in_val - fb;

        // Stage 1 low-pass integrator
        int64_t d1 = ((drive - s1_) * cutoff_.raw) >> 15;
        s1_ += d1;
        s1_ = saturate_soft(s1_);

        // Stage 2 low-pass integrator
        int64_t d2 = ((s1_ - s2_) * cutoff_.raw) >> 15;
        s2_ += d2;
        s2_ = saturate_soft(s2_);

        int32_t out = static_cast<int32_t>(std::clamp<int64_t>(s2_, -32768, 32767));
        return FixedQ15::from_raw(static_cast<int16_t>(out));
    }

private:
    // Soft cubic saturator in fixed-point: y = x - (x * x * x) / (3 * 32768^2)
    [[nodiscard]] static constexpr int64_t saturate_soft(int64_t x) noexcept {
        if (x > 32767) return 32767;
        if (x < -32768) return -32768;

        // (x * x) >> 15 -> Q15 squared
        int64_t x2 = (x * x) >> 15;
        // (x2 * x) >> 15 -> Q15 cubed
        int64_t x3 = (x2 * x) >> 15;
        // x - x3 / 3
        int64_t y = x - (x3 / 3);
        return std::clamp<int64_t>(y, -32768, 32767);
    }

    FixedQ15 cutoff_{FixedQ15::from_raw(4000)};
    FixedQ15 resonance_{FixedQ15::from_raw(0)};
    int64_t s1_{0};
    int64_t s2_{0};
};

} // namespace aether::math
