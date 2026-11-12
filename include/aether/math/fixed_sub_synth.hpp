#pragma once

#include "fixed_point.hpp"
#include <cstdint>
#include <array>
#include <algorithm>

namespace aether::math {

/**
 * FixedHarmonicExciter
 * Dual-Band Sub-Harmonic Synthesizer (Sub-Octave Generator) in Q15:
 * - Edge-triggered flip-flop sub-octave divider (/2) for bass generation
 * - 2nd-order resonant lowpass tracking the fundamental
 * - Sub-bass drive and blend control
 * - Zero dynamic heap allocation.
 */
class FixedSubHarmonicSynth {
public:
    constexpr FixedSubHarmonicSynth() noexcept = default;

    /**
     * configure
     * sub_gain: Q15 volume of generated sub-bass [0, 32767]
     * dry_gain: Q15 volume of original signal [0, 32767]
     * filter_cutoff: Q15 smoothing lowpass cutoff [1000, 16000]
     */
    void configure(FixedQ15 sub_gain, FixedQ15 dry_gain, FixedQ15 filter_cutoff) noexcept {
        sub_gain_ = std::clamp<int16_t>(sub_gain.raw, 0, 32767);
        dry_gain_ = std::clamp<int16_t>(dry_gain.raw, 0, 32767);
        filter_cutoff_ = std::clamp<int16_t>(filter_cutoff.raw, 500, 20000);
    }

    void reset() noexcept {
        flip_flop_ = false;
        prev_sign_ = false;
        lp_state1_ = 0;
        lp_state2_ = 0;
    }

    [[nodiscard]] FixedQ15 process(FixedQ15 input) noexcept {
        int32_t in_val = input.raw;

        // Zero-crossing detection with positive edge trigger
        bool current_sign = in_val >= 0;
        if (current_sign && !prev_sign_) {
            // Rising zero-crossing: toggle sub-octave flip-flop
            flip_flop_ = !flip_flop_;
        }
        prev_sign_ = current_sign;

        // Raw sub-octave square wave scaled to input envelope magnitude
        int32_t env = std::abs(in_val);
        int32_t raw_sub = flip_flop_ ? env : -env;

        // 2nd-order lowpass filter to smooth square wave into warm sine sub-bass
        lp_state1_ += ((raw_sub - lp_state1_) * filter_cutoff_) >> 15;
        lp_state1_ = std::clamp<int32_t>(lp_state1_, -32768, 32767);

        lp_state2_ += ((lp_state1_ - lp_state2_) * filter_cutoff_) >> 15;
        lp_state2_ = std::clamp<int32_t>(lp_state2_, -32768, 32767);

        // Mix dry + sub
        int32_t dry_contrib = (in_val * dry_gain_) >> 15;
        int32_t sub_contrib = (lp_state2_ * sub_gain_) >> 15;
        int32_t mixed = dry_contrib + sub_contrib;
        mixed = std::clamp<int32_t>(mixed, -32768, 32767);

        return FixedQ15::from_raw(static_cast<int16_t>(mixed));
    }

private:
    int16_t sub_gain_{16384};      // 0.5x
    int16_t dry_gain_{32767};      // 1.0x
    int16_t filter_cutoff_{4000};  // Low cutoff for sub bass
    bool flip_flop_{false};
    bool prev_sign_{false};
    int32_t lp_state1_{0};
    int32_t lp_state2_{0};
};

} // namespace aether::math
