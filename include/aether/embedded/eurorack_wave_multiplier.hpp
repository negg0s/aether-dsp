#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include "aether/math/fixed_point.hpp"

namespace aether::embedded {

// Eurorack Precision Voltage Controlled Analog Wave Multiplier / West Coast Serge Triple Wave Multiplier
// Generates rich acoustic timbres from simple sine or triangle waves via series of
// biased non-linear differential diode folding cells (3 harmonic stages).
// Zero-heap, zero-allocation bare-metal MCU implementation.
class EurorackWaveMultiplier {
public:
    EurorackWaveMultiplier() {
        reset();
    }

    void reset() noexcept {
        drive_ = math::FixedQ15::from_float(1.5f);
        symmetry_ = math::FixedQ15(0);
        odd_even_mix_ = math::FixedQ15::from_float(0.5f);
    }

    void configure(math::FixedQ15 drive, math::FixedQ15 symmetry, math::FixedQ15 odd_even) noexcept {
        drive_ = drive;
        symmetry_ = symmetry;
        odd_even_mix_ = odd_even;
    }

    // Process a single audio sample through the 3-stage Serge wave multiplier core
    math::FixedQ15 process(math::FixedQ15 input) noexcept {
        // Stage 1: Linear amplification with DC symmetry bias
        int64_t x = (static_cast<int64_t>(input.raw) * drive_.raw) >> 15;
        x += (symmetry_.raw >> 1);

        // Stage 2: Full-wave rectification cell (doubles fundamental frequency)
        int64_t rectified = (x < 0) ? -x : x;
        // Normalize rectified wave to bipolar centered signal
        int64_t even_harmonic = (rectified << 1) - 32767;

        // Stage 3: Differential diode folding cell (cubic polynomial fold: 3x - 4x^3)
        // Odd harmonic generation
        int64_t x_clamped = x;
        if (x_clamped > 32767) x_clamped = 32767;
        if (x_clamped < -32768) x_clamped = -32768;

        int64_t x2 = (x_clamped * x_clamped) >> 15;
        int64_t x3 = (x2 * x_clamped) >> 15;
        int64_t odd_harmonic = (3 * x_clamped) - (4 * x3);

        // Blend even (rectified) and odd (folded) harmonic stages
        int64_t out_sig = ((even_harmonic * odd_even_mix_.raw) >> 15) +
                          ((odd_harmonic * (32767 - odd_even_mix_.raw)) >> 15);

        if (out_sig > 32767) out_sig = 32767;
        if (out_sig < -32768) out_sig = -32768;

        return math::FixedQ15(static_cast<int16_t>(out_sig));
    }

private:
    math::FixedQ15 drive_{math::FixedQ15::from_float(1.5f)};
    math::FixedQ15 symmetry_{0};
    math::FixedQ15 odd_even_mix_{math::FixedQ15::from_float(0.5f)};
};

} // namespace aether::embedded
