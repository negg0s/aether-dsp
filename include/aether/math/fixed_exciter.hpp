#pragma once

#include "fixed_point.hpp"
#include <cstdint>
#include <array>
#include <algorithm>

namespace aether::math {

/**
 * FixedExciter
 * High-frequency psychoacoustic exciter / harmonic enhancer in Q15:
 * - Highpass sidechain to isolate brilliance band
 * - Asymmetric quadratic harmonic distortion (even harmonics)
 * - Mix enhancement back into dry signal for sparkle and presence
 * Zero heap, zero FPU.
 */
class FixedExciter {
public:
    constexpr FixedExciter() noexcept = default;

    /**
     * configure
     * hp_cutoff: Q15 cutoff for highpass sidechain [5000, 25000]
     * drive: Q15 distortion drive [32768, 163840]
     * blend: Q15 wet harmonic presence mix [0, 32767]
     */
    void configure(FixedQ15 hp_cutoff, int32_t drive, FixedQ15 blend) noexcept {
        hp_cutoff_ = std::clamp<int16_t>(hp_cutoff.raw, 2000, 30000);
        drive_ = std::clamp<int32_t>(drive, 1000, 262144);
        blend_ = std::clamp<int16_t>(blend.raw, 0, 32767);
    }

    void reset() noexcept {
        lp_state_ = 0;
    }

    [[nodiscard]] FixedQ15 process(FixedQ15 input) noexcept {
        int32_t in_val = input.raw;

        // 1-pole lowpass to derive highpass = in - lp
        lp_state_ += ((in_val - lp_state_) * hp_cutoff_) >> 15;
        lp_state_ = std::clamp<int32_t>(lp_state_, -32768, 32767);
        int32_t hp = in_val - lp_state_;

        // Quadratic harmonic generator: y = (hp * hp) >> 15 (doubles frequency -> octaves up)
        int32_t driven_hp = (hp * drive_) >> 15;
        driven_hp = std::clamp<int32_t>(driven_hp, -32768, 32767);
        int32_t harmonics = (driven_hp * std::abs(driven_hp)) >> 15;

        // Add harmonic brilliance back to input
        int32_t wet_contrib = (harmonics * blend_) >> 15;
        int32_t out = in_val + wet_contrib;
        out = std::clamp<int32_t>(out, -32768, 32767);

        return FixedQ15::from_raw(static_cast<int16_t>(out));
    }

private:
    int16_t hp_cutoff_{16000};
    int32_t drive_{65536}; // 2.0x
    int16_t blend_{8192};  // 25% wet
    int32_t lp_state_{0};
};

} // namespace aether::math
