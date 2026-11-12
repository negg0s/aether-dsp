#pragma once

#include "fixed_point.hpp"
#include <cstdint>
#include <array>
#include <algorithm>

namespace aether::math {

/**
 * FixedTransientShaper
 * Fast attack and sustain transient designer in Q15:
 * - Dual-envelope follower (fast attack envelope vs slow sustain envelope)
 * - Attack boost/cut (transient punch, percussive click accentuation)
 * - Sustain boost/cut (tail compression, room decay control)
 * - Zero dynamic heap allocation.
 */
class FixedTransientShaper {
public:
    constexpr FixedTransientShaper() noexcept = default;

    /**
     * configure
     * attack_gain: Q15 multiplier for transient attack [-32768, 32767] (0 = neutral, positive = boost, negative = cut)
     * sustain_gain: Q15 multiplier for sustain tail [-32768, 32767]
     */
    void configure(FixedQ15 attack_gain, FixedQ15 sustain_gain) noexcept {
        attack_gain_ = attack_gain;
        sustain_gain_ = sustain_gain;
    }

    void reset() noexcept {
        env_fast_ = 0;
        env_slow_ = 0;
    }

    [[nodiscard]] FixedQ15 process(FixedQ15 input) noexcept {
        int32_t in_abs = std::abs(static_cast<int32_t>(input.raw));

        // Fast envelope follower (attack tracking): attack coeff = 0.8, release = 0.05
        if (in_abs > env_fast_) {
            env_fast_ += ((in_abs - env_fast_) * 26214) >> 15;
        } else {
            env_fast_ -= (env_fast_ * 1638) >> 15;
        }
        env_fast_ = std::clamp<int32_t>(env_fast_, 0, 32767);

        // Slow envelope follower (sustain tracking): attack coeff = 0.1, release = 0.005
        if (in_abs > env_slow_) {
            env_slow_ += ((in_abs - env_slow_) * 3276) >> 15;
        } else {
            env_slow_ -= (env_slow_ * 164) >> 15;
        }
        env_slow_ = std::clamp<int32_t>(env_slow_, 0, 32767);

        // Differential transient detection: diff = fast - slow
        int32_t trans_diff = env_fast_ - env_slow_; // positive on drum hits

        // Compute gain modulations
        // Attack modulation: (trans_diff * attack_gain) >> 15
        int32_t attack_mod = (trans_diff * attack_gain_.raw) >> 15;

        // Sustain modulation: (env_slow * sustain_gain) >> 15
        int32_t sustain_mod = (env_slow_ * sustain_gain_.raw) >> 15;

        // Total scale = 1.0 (32768) + attack_mod + sustain_mod
        int32_t total_scale = 32768 + attack_mod + sustain_mod;
        total_scale = std::clamp<int32_t>(total_scale, 0, 131072); // up to 4.0x gain

        int32_t out = (static_cast<int32_t>(input.raw) * total_scale) >> 15;
        out = std::clamp<int32_t>(out, -32768, 32767);

        return FixedQ15::from_raw(static_cast<int16_t>(out));
    }

private:
    FixedQ15 attack_gain_{FixedQ15::from_raw(0)};
    FixedQ15 sustain_gain_{FixedQ15::from_raw(0)};
    int32_t env_fast_{0};
    int32_t env_slow_{0};
};

} // namespace aether::math
