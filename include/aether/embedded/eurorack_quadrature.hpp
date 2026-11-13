#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include "aether/math/fixed_point.hpp"

namespace aether::embedded {

// Eurorack Precision Voltage Controlled Analog Phase Shifter & Quadrature Generator
// Generates pair of Sine/Cosine quadrature audio or CV signals (0 deg and 90 deg out of phase)
// via coupled 2nd-order harmonic fixed-point rotation matrix (Givens rotation).
// Zero-heap, zero-allocation bare-metal MCU quadrature oscillator.
class EurorackQuadratureOscillator {
public:
    EurorackQuadratureOscillator() {
        reset();
    }

    void reset() noexcept {
        cos_val_ = 32767; // cos(0) = 1.0 in Q15
        sin_val_ = 0;     // sin(0) = 0.0 in Q15
        epsilon_q15_ = math::FixedQ15::from_float(0.05f); // frequency step ~ 380 Hz at 48kHz
    }

    // Set oscillation frequency coefficient: epsilon ~ 2 * sin(pi * f / fs) in Q15
    void set_frequency(math::FixedQ15 epsilon) noexcept {
        epsilon_q15_ = epsilon;
    }

    struct QuadratureOutput {
        math::FixedQ15 cosine{0};
        math::FixedQ15 sine{0};
    };

    // Advance 1 sample using coupled conservative integrator:
    // sin[n+1] = sin[n] + eps * cos[n]
    // cos[n+1] = cos[n] - eps * sin[n+1]  (symplectic Euler step preserves amplitude)
    QuadratureOutput process() noexcept {
        int64_t sin_step = (static_cast<int64_t>(cos_val_) * epsilon_q15_.raw) >> 15;
        sin_val_ += static_cast<int32_t>(sin_step);

        int64_t cos_step = (static_cast<int64_t>(sin_val_) * epsilon_q15_.raw) >> 15;
        cos_val_ -= static_cast<int32_t>(cos_step);

        // Amplitude stabilization clamp to prevent numerical energy drift
        if (sin_val_ > 32767) sin_val_ = 32767;
        if (sin_val_ < -32768) sin_val_ = -32768;
        if (cos_val_ > 32767) cos_val_ = 32767;
        if (cos_val_ < -32768) cos_val_ = -32768;

        return {
            math::FixedQ15(static_cast<int16_t>(cos_val_)),
            math::FixedQ15(static_cast<int16_t>(sin_val_))
        };
    }

private:
    int32_t cos_val_{32767};
    int32_t sin_val_{0};
    math::FixedQ15 epsilon_q15_{math::FixedQ15::from_float(0.05f)};
};

} // namespace aether::embedded
