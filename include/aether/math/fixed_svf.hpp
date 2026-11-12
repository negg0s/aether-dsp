#pragma once

#include "fixed_point.hpp"
#include <cstdint>
#include <array>
#include <algorithm>

namespace aether::math {

/**
 * FixedStateVariableFilter (Chamberlin SVF)
 * Zero-heap 2nd-order State Variable Filter in Q15.
 * Simultaneous outputs from a single core:
 * - Lowpass
 * - Highpass
 * - Bandpass
 * - Notch
 * Ultra-stable digital integrator with resonance control and Q15 saturation.
 * Zero dynamic memory, zero floating point.
 */
class FixedStateVariableFilter {
public:
    constexpr FixedStateVariableFilter() noexcept = default;

    /**
     * configure
     * f_q15: frequency parameter in Q15 [100, 16384] (f = 2 * sin(pi * fc / fs))
     * q_inv_q15: inverse Q / damping factor in Q15 [100, 32767] (1/Q, where smaller = higher resonance)
     */
    void configure(FixedQ15 f, FixedQ15 q_inv) noexcept {
        f_ = FixedQ15::from_raw(std::clamp<int16_t>(f.raw, 100, 16000));
        q_inv_ = FixedQ15::from_raw(std::clamp<int16_t>(q_inv.raw, 100, 32767));
    }

    void reset() noexcept {
        s_lp_ = 0;
        s_bp_ = 0;
    }

    struct FilterOutputs {
        FixedQ15 lowpass;
        FixedQ15 highpass;
        FixedQ15 bandpass;
        FixedQ15 notch;
    };

    [[nodiscard]] FilterOutputs process(FixedQ15 input) noexcept {
        // Highpass = input - s_lp - q_inv * s_bp
        int32_t damp_contrib = (s_bp_ * q_inv_.raw) >> 15;
        int32_t hp = static_cast<int32_t>(input.raw) - s_lp_ - damp_contrib;
        hp = std::clamp<int32_t>(hp, -32768, 32767);

        // Bandpass = s_bp + f * hp
        int32_t bp_inc = (hp * f_.raw) >> 15;
        int32_t bp = s_bp_ + bp_inc;
        bp = std::clamp<int32_t>(bp, -32768, 32767);

        // Lowpass = s_lp + f * bp
        int32_t lp_inc = (bp * f_.raw) >> 15;
        int32_t lp = s_lp_ + lp_inc;
        lp = std::clamp<int32_t>(lp, -32768, 32767);

        // Notch = hp + lp
        int32_t notch = hp + lp;
        notch = std::clamp<int32_t>(notch, -32768, 32767);

        // Update states
        s_bp_ = bp;
        s_lp_ = lp;

        return {
            FixedQ15::from_raw(static_cast<int16_t>(lp)),
            FixedQ15::from_raw(static_cast<int16_t>(hp)),
            FixedQ15::from_raw(static_cast<int16_t>(bp)),
            FixedQ15::from_raw(static_cast<int16_t>(notch))
        };
    }

private:
    FixedQ15 f_{FixedQ15::from_raw(2000)};
    FixedQ15 q_inv_{FixedQ15::from_raw(16384)};
    int32_t s_lp_{0};
    int32_t s_bp_{0};
};

} // namespace aether::math
