#pragma once

#include "fixed_point.hpp"
#include <cstdint>
#include <array>
#include <algorithm>

namespace aether::math {

/**
 * FixedGate
 * Downward noise gate and expander in Q15 for embedded targets:
 * - Hysteresis thresholding to prevent chattering on noisy signals
 * - Attack, hold, and release time constants
 * - Floor attenuation level in Q15
 * Zero dynamic memory, zero floating point.
 */
class FixedGate {
public:
    constexpr FixedGate() noexcept = default;

    /**
     * configure
     * open_threshold: Q15 level to open gate [0, 32767]
     * close_threshold: Q15 level to close gate [0, open_threshold]
     * attack_coeff: Q15 envelope speed when opening [0, 32767]
     * release_coeff: Q15 envelope speed when closing [0, 32767]
     * floor_q15: attenuation when closed [0, 32767] (0 = total silence)
     */
    void configure(FixedQ15 open_th, FixedQ15 close_th, FixedQ15 attack, FixedQ15 release, FixedQ15 floor) noexcept {
        open_th_ = std::clamp<int16_t>(open_th.raw, 100, 32767);
        close_th_ = std::clamp<int16_t>(close_th.raw, 0, open_th_);
        attack_ = std::clamp<int16_t>(attack.raw, 100, 32767);
        release_ = std::clamp<int16_t>(release.raw, 10, 32767);
        floor_ = std::clamp<int16_t>(floor.raw, 0, 32767);
    }

    void reset() noexcept {
        gain_state_ = 0;
        is_open_ = false;
    }

    [[nodiscard]] FixedQ15 process(FixedQ15 input) noexcept {
        int32_t in_abs = std::abs(static_cast<int32_t>(input.raw));

        // State machine with hysteresis
        if (is_open_) {
            if (in_abs < close_th_) {
                is_open_ = false;
            }
        } else {
            if (in_abs >= open_th_) {
                is_open_ = true;
            }
        }

        // Target gain: 1.0 (32767) if open, floor if closed
        int32_t target_gain = is_open_ ? 32767 : floor_;

        // Smooth gain transition
        if (target_gain > gain_state_) {
            // Opening (attack)
            gain_state_ += ((target_gain - gain_state_) * attack_) >> 15;
        } else {
            // Closing (release)
            gain_state_ -= ((gain_state_ - target_gain) * release_) >> 15;
        }
        gain_state_ = std::clamp<int32_t>(gain_state_, 0, 32767);

        // Apply gain to input
        int32_t out = (static_cast<int32_t>(input.raw) * gain_state_) >> 15;
        out = std::clamp<int32_t>(out, -32768, 32767);

        return FixedQ15::from_raw(static_cast<int16_t>(out));
    }

    [[nodiscard]] bool is_open() const noexcept {
        return is_open_;
    }

private:
    int16_t open_th_{4000};
    int16_t close_th_{2000};
    int16_t attack_{16384};
    int16_t release_{1000};
    int16_t floor_{0};
    int32_t gain_state_{0};
    bool is_open_{false};
};

} // namespace aether::math
