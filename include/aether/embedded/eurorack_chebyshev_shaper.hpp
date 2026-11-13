#pragma once

#include "aether/math/fixed_point.hpp"
#include <cstdint>

namespace aether::embedded {

/**
 * @brief Fixed-point 8-stage Wave Shaper & Multi-Segment Chebychev Distortion Core.
 * Evaluates Chebyshev polynomials (T1 through T5) using integer fixed-point recurrence:
 * T0(x) = 1
 * T1(x) = x
 * T2(x) = 2*x^2 - 1
 * T3(x) = 4*x^3 - 3*x
 * T4(x) = 8*x^4 - 8*x^2 + 1
 * T5(x) = 16*x^5 - 20*x^3 + 5*x
 * 
 * Provides precise, surgical generation of individual even and odd musical harmonics.
 * Pure integer arithmetic, zero dynamic allocation, designed for STM32 / Daisy Patch.
 */
class EurorackChebyshevShaper {
public:
    EurorackChebyshevShaper() {
        reset();
    }

    void reset() {
        h1_q15_ = 32767; // Fundamental
        h2_q15_ = 0;     // 2nd harmonic (octave)
        h3_q15_ = 0;     // 3rd harmonic (octave + fifth)
        h4_q15_ = 0;     // 4th harmonic (2 octaves)
        h5_q15_ = 0;     // 5th harmonic (2 octaves + major third)
        drive_q15_ = 32767; // 1.0x drive
    }

    void set_harmonics(math::FixedQ15 h1, math::FixedQ15 h2, math::FixedQ15 h3, 
                       math::FixedQ15 h4, math::FixedQ15 h5) {
        h1_q15_ = h1.raw;
        h2_q15_ = h2.raw;
        h3_q15_ = h3.raw;
        h4_q15_ = h4.raw;
        h5_q15_ = h5.raw;
    }

    void set_drive(math::FixedQ15 drive) {
        if (drive.raw < 0) drive.raw = 0;
        drive_q15_ = drive.raw;
    }

    math::FixedQ15 process(math::FixedQ15 input) {
        // Apply drive to normalized [-1, 1] range in Q15
        int32_t x = (static_cast<int64_t>(input.raw) * drive_q15_) >> 15;
        if (x > 32767) x = 32767;
        if (x < -32768) x = -32768;

        // Chebyshev recurrence evaluation:
        // T0 = 32767 (1.0)
        // T1 = x
        // T_{n+1} = 2*x*T_n - T_{n-1}
        int32_t t0 = 32767;
        int32_t t1 = x;

        // T2 = (2 * x * t1) - t0
        int32_t t2 = ((static_cast<int64_t>(x) * t1) >> 14) - t0;
        if (t2 > 32767) t2 = 32767;
        if (t2 < -32768) t2 = -32768;

        // T3 = (2 * x * t2) - t1
        int32_t t3 = ((static_cast<int64_t>(x) * t2) >> 14) - t1;
        if (t3 > 32767) t3 = 32767;
        if (t3 < -32768) t3 = -32768;

        // T4 = (2 * x * t3) - t2
        int32_t t4 = ((static_cast<int64_t>(x) * t3) >> 14) - t2;
        if (t4 > 32767) t4 = 32767;
        if (t4 < -32768) t4 = -32768;

        // T5 = (2 * x * t4) - t3
        int32_t t5 = ((static_cast<int64_t>(x) * t4) >> 14) - t3;
        if (t5 > 32767) t5 = 32767;
        if (t5 < -32768) t5 = -32768;

        // Sum weighted harmonics
        int64_t sum = 0;
        sum += (static_cast<int64_t>(t1) * h1_q15_) >> 15;
        sum += (static_cast<int64_t>(t2) * h2_q15_) >> 15;
        sum += (static_cast<int64_t>(t3) * h3_q15_) >> 15;
        sum += (static_cast<int64_t>(t4) * h4_q15_) >> 15;
        sum += (static_cast<int64_t>(t5) * h5_q15_) >> 15;

        // Normalize sum by total weights
        int32_t total_w = std::abs(h1_q15_) + std::abs(h2_q15_) + std::abs(h3_q15_) + 
                          std::abs(h4_q15_) + std::abs(h5_q15_);
        if (total_w == 0) total_w = 32767;

        int32_t out = static_cast<int32_t>((sum * 32767) / total_w);
        if (out > 32767) out = 32767;
        if (out < -32768) out = -32768;

        return math::FixedQ15::from_raw(static_cast<int16_t>(out));
    }

private:
    int16_t h1_q15_{32767};
    int16_t h2_q15_{0};
    int16_t h3_q15_{0};
    int16_t h4_q15_{0};
    int16_t h5_q15_{0};
    int16_t drive_q15_{32767};
};

} // namespace aether::embedded
