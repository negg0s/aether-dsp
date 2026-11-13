#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include "aether/math/fixed_point.hpp"

namespace aether::embedded {

// Eurorack Precision Voltage Controlled Analog Gristle / Fuzz Diode Distortion
// Models Germanium / Silicon clipping diode forward-drop knees,
// asymmetrical crossover bias, and dynamic sag in fixed-point Q15.
// Zero-heap, zero-allocation bare-metal MCU implementation.
class EurorackDiodeFuzz {
public:
    EurorackDiodeFuzz() {
        reset();
    }

    void reset() noexcept {
        drive_ = math::FixedQ15::from_float(2.0f);
        bias_ = math::FixedQ15(0);
        germanium_mode_ = true;
        sag_state_ = 0;
    }

    // Configure drive gain (1.0 to 8.0) and DC asymmetry bias (-1.0 to +1.0)
    void configure(math::FixedQ15 drive, math::FixedQ15 bias, bool germanium = true) noexcept {
        drive_ = drive;
        bias_ = bias;
        germanium_mode_ = germanium;
    }

    // Process a single audio sample through the non-linear diode clipping stage
    math::FixedQ15 process(math::FixedQ15 input) noexcept {
        // Dynamic sag emulation (power rail sag under hot signals)
        int32_t abs_in = (input.raw < 0) ? -input.raw : input.raw;
        sag_state_ += ((abs_in - sag_state_) * 500) >> 15;
        int32_t sag_voltage = (sag_state_ * 6000) >> 15; // up to ~18% rail sag

        // Pre-amp gain stage
        int64_t x = (static_cast<int64_t>(input.raw) * drive_.raw) >> 14; // extra gain
        x += (bias_.raw >> 1);

        // Diode forward-voltage threshold: Germanium ~ 9800 (~0.3V), Silicon ~ 22000 (~0.7V)
        int32_t threshold = (germanium_mode_ ? 9800 : 22000) - sag_voltage;
        if (threshold < 3000) threshold = 3000;

        // Exponential diode knee transfer function:
        // For |x| <= threshold: linear with gentle warming
        // For |x| > threshold: asymptotic soft saturation
        int64_t out_sig = 0;
        if (x > threshold) {
            int64_t excess = x - threshold;
            int64_t compressed = (excess << 14) / (8192 + excess);
            out_sig = threshold + compressed;
        } else if (x < -threshold) {
            int64_t excess = -x - threshold;
            int64_t compressed = (excess << 14) / (8192 + excess);
            out_sig = -(threshold + compressed);
        } else {
            // Cubic warmth below diode conduction threshold: x - x^3 / 3
            int64_t x2 = (x * x) >> 15;
            int64_t x3 = (x2 * x) >> 15;
            out_sig = x - (x3 / 4);
        }

        if (out_sig > 32767) out_sig = 32767;
        if (out_sig < -32768) out_sig = -32768;

        return math::FixedQ15(static_cast<int16_t>(out_sig));
    }

private:
    math::FixedQ15 drive_{math::FixedQ15::from_float(2.0f)};
    math::FixedQ15 bias_{0};
    bool germanium_mode_{true};
    int32_t sag_state_{0};
};

} // namespace aether::embedded
