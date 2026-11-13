#pragma once

#include "fixed_point.hpp"
#include <cstdint>
#include <array>
#include <algorithm>

namespace aether::math {

/**
 * FixedHarmonicExciter
 * Envelope-controlled dynamic auto-wah filter in Q15:
 * - Peak/RMS envelope follower tracking input dynamics
 * - Modulates Chamberlin 2nd-order bandpass filter cutoff
 * - Sensitivity, frequency range, and resonance Q control
 * - Zero dynamic heap allocation.
 */
class FixedAutoWah {
public:
    constexpr FixedAutoWah() noexcept = default;

    /**
     * configure
     * min_f: base frequency parameter in Q15 [500, 5000]
     * max_f: peak frequency parameter in Q15 [min_f, 16000]
     * sensitivity: envelope sensitivity in Q15 [1000, 32767]
     * q_inv: resonance damping factor in Q15 [1000, 16384] (smaller = higher peak)
     */
    void configure(FixedQ15 min_f, FixedQ15 max_f, FixedQ15 sensitivity, FixedQ15 q_inv) noexcept {
        min_f_ = std::clamp<int16_t>(min_f.raw, 200, 10000);
        max_f_ = std::clamp<int16_t>(max_f.raw, min_f_, 16000);
        sensitivity_ = std::clamp<int16_t>(sensitivity.raw, 100, 32767);
        q_inv_ = std::clamp<int16_t>(q_inv.raw, 500, 20000);
    }

    void reset() noexcept {
        env_ = 0;
        lp_ = 0;
        bp_ = 0;
    }

    [[nodiscard]] FixedQ15 process(FixedQ15 input) noexcept {
        int32_t in_abs = std::abs(static_cast<int32_t>(input.raw));

        // Fast attack, moderate decay envelope follower
        if (in_abs > env_) {
            env_ += ((in_abs - env_) * 16384) >> 15; // 0.5 attack
        } else {
            env_ -= (env_ * 1638) >> 15; // 0.05 release
        }
        env_ = std::clamp<int32_t>(env_, 0, 32767);

        // Modulate cutoff: f = min_f + (max_f - min_f) * (env * sensitivity)
        int32_t env_mod = (env_ * sensitivity_) >> 15;
        int32_t f_range = max_f_ - min_f_;
        int32_t current_f = min_f_ + ((f_range * env_mod) >> 15);
        current_f = std::clamp<int32_t>(current_f, 200, 16000);

        // Chamberlin SVF Bandpass stage
        int32_t in_val = input.raw;
        int32_t damp = (bp_ * q_inv_) >> 15;
        int32_t hp = in_val - lp_ - damp;
        hp = std::clamp<int32_t>(hp, -32768, 32767);

        int32_t bp_inc = (hp * current_f) >> 15;
        bp_ = std::clamp<int32_t>(bp_ + bp_inc, -32768, 32767);

        int32_t lp_inc = (bp_ * current_f) >> 15;
        lp_ = std::clamp<int32_t>(lp_ + lp_inc, -32768, 32767);

        // Wah output is resonant bandpass mixed with dry signal
        int32_t out = (in_val >> 1) + bp_;
        out = std::clamp<int32_t>(out, -32768, 32767);

        return FixedQ15::from_raw(static_cast<int16_t>(out));
    }

private:
    int16_t min_f_{1500};
    int16_t max_f_{12000};
    int16_t sensitivity_{24000};
    int16_t q_inv_{4000}; // high Q
    int32_t env_{0};
    int32_t lp_{0};
    int32_t bp_{0};
};

} // namespace aether::math
